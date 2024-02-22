#include <ucontext.h>

struct GreenThread {
    ucontext_t context;
    void (*function)(void *);
    int active;
    struct GreenThread *next;
    struct GreenThread *prev;
    void *stack;
    void *args;
};

void orchestrate();
void initialise();
int create_thread(void (*function)(void *), void *args);
