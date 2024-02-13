#include <ucontext.h>
#include <stdlib.h>

#include "dhage.h"

#define STACK_SIZE 1024 * 1024 * 8

struct GreenThread {
    ucontext_t context;
    void (*function)(void);
    int active;
    int tid;
    struct GreenThread *next;
};

void switch_threads();
void thread_exit();
void thread_wrapper();
int create_thread(void (*function)(void));

struct GreenThread *current_thread = NULL;

void
switch_threads()
{
    struct GreenThread *prev_thread = current_thread;
    current_thread = current_thread->next;

    if (prev_thread->active)
        swapcontext(&prev_thread->context, &current_thread->context);

    while (current_thread != prev_thread) {
        if (current_thread->active)
            setcontext(&current_thread->context);

        current_thread = current_thread->next;
    }
    
    exit(0);
}

void
thread_exit()
{
    current_thread->active = 0;
    switch_threads();
}

void
thread_wrapper()
{
    current_thread->function();
    thread_exit();
}

int
create_thread(void (*function)(void))
{
    static int tid = 0;

    struct GreenThread *conductor;
    struct GreenThread *gt = malloc(sizeof(struct GreenThread));
    gt->function = function;
    gt->active = 1;

    if (current_thread == NULL) {
        gt->tid = tid+1;
        current_thread = gt;
        current_thread->next = current_thread;
    } else {
        gt->tid = tid+1;
        conductor = current_thread;
        while (conductor->tid != tid)
            conductor = conductor->next;
    
        gt->next = conductor->next;
        conductor->next = gt;
    }

    getcontext(&gt->context);
    gt->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    gt->context.uc_stack.ss_size = STACK_SIZE;
    gt->context.uc_link = 0;

    makecontext(&gt->context, thread_wrapper, 0);

    tid = tid + 1;
    return tid;
}
