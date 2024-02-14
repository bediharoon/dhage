#include <ucontext.h>

struct GreenThread {
    ucontext_t context;
    void (*function)(void *);
    int active;
    int tid;
    struct GreenThread *next;
    void *stack;
    void *args;
};

void orchestrate();
void initialise();
int create_thread(void (*function)(void *), void *args);
