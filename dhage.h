#ifndef DHAGE_H
#define DHAGE_H

#include <pthread.h>
#include <stdlib.h>


/* Thread Pool API */
typedef struct ThreadPool ThreadPool;

ThreadPool* threadpool_init();
ThreadPool* threadpool_create(int num_threads, int max_queue_size);
int threadpool_enqueue(ThreadPool* pool, void (*function)(void*), void* arg);
int csp_spawn(ThreadPool* pool, void (*func)(void*), void* arg);
void threadpool_destroy(ThreadPool* pool);


/* Channel API */
typedef struct Channel Channel;

Channel* channel_create(void);
int channel_send(Channel* chan, void* data);
int channel_receive(Channel* chan, void** buffer);
void channel_close(Channel* chan);
void channel_destroy(Channel* chan);


#endif // DHAGE_H
