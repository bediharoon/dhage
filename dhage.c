#include <ucontext.h>
#include <stdlib.h>

#include "dhage.h"

#define STACK_SIZE (1024 * 2)

void error();
void cleanup_thread();
void switch_threads();
void thread_exit();
void thread_wrapper();

struct GreenThread *current_thread;
ucontext_t cleanup_context;

void
error()
{
    exit(-1);
}

void
initialise()
{
    current_thread = NULL;

    getcontext(&cleanup_context);

    cleanup_context.uc_stack.ss_sp = malloc(STACK_SIZE);
    if (cleanup_context.uc_stack.ss_sp == NULL)
        error();

    cleanup_context.uc_stack.ss_size = STACK_SIZE;
    cleanup_context.uc_link = NULL;
    makecontext(&cleanup_context, cleanup_thread, 0);
}

void
cleanup_thread()
{
    if (current_thread->stack != NULL)
        free(current_thread->stack);

    current_thread->stack = NULL;
}

void
switch_threads()
{
    struct GreenThread *prev_thread = current_thread;
    current_thread = current_thread->next;

    /* Next thread being active should be guaranteed by the achitecture */
    while (!(current_thread->active)) {
        current_thread = current_thread->next;

        if (current_thread == prev_thread)
            exit(0);
    }

    if (prev_thread->active)
        swapcontext(&prev_thread->context, &current_thread->context);

    setcontext(&current_thread->context);
}

void
thread_exit()
{
    current_thread->active = 0;

    current_thread->next->prev = current_thread->prev;
    current_thread->prev->next = current_thread->next;
    free(current_thread);

    switch_threads();
}

void
thread_wrapper()
{
    current_thread->function(current_thread->args);
    thread_exit();
}

int
create_thread(void (*function)(void *), void *args)
{
    struct GreenThread *gt = malloc(sizeof(struct GreenThread));
    if (gt == NULL) {
        error();
    }

    gt->function = function;
    gt->active = 1;
    gt->args = args;

    if (current_thread == NULL) {
        current_thread = gt;
        current_thread->next = current_thread;
        current_thread->prev = current_thread;
    } else {
        current_thread->next->prev = gt;
        gt->next = current_thread->next;
        current_thread->next = gt;
        gt->prev = current_thread;
    }

    getcontext(&gt->context);
    gt->stack = malloc(STACK_SIZE);
    if (gt->stack == NULL) {
        free(gt);
        error();
    }

    gt->context.uc_stack.ss_sp = gt->stack;
    gt->context.uc_stack.ss_size = STACK_SIZE;
    gt->context.uc_link = &cleanup_context;

    makecontext(&gt->context, thread_wrapper, 0);

    return 0;
}

void
orchestrate()
{
    setcontext(&current_thread->context);
}
