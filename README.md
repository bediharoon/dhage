# dhage
A 'go' at implementing CSP-style preemptive concurrency in C.

###Todo
- [x] Green Threads + Cooperative Scheduler (Function Ends == Yield) using ucontext.h.
- [x] Create stack cleanup + scheduler routines using uc\_link.
- [x] Allow functions to take arguments.
- [ ] Use pthreads to achieve pre-emptive concurrency with variable quanta of execution.
- [ ] Make blocking channels to use CSP style concurrency.
- [ ] Support multiple producers and consumers, different data types, and buffered channels.
- [ ] Simplify thread creation API.
- [ ] Write tests and analytics.
- [ ] Evaluate using other kinds of multi-threading.
- [ ] Switch from ucontext.h to using assembly.
- [ ] Policy and Event Driven Scheduling
