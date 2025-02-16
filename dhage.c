#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct Task {
    void (*function)(void*);
    void* argument;
    struct Task* next;
} Task;

typedef struct ThreadPool {
    pthread_t* threads;
    int num_threads;
    int max_queue_size;
    volatile int current_queue_size;
    volatile int shutdown;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    Task* task_head;
    Task* task_tail;
} ThreadPool;

static void* worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while (1) {
        pthread_mutex_lock(&pool->lock);
        while (pool->task_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        Task* task = pool->task_head;
        pool->task_head = task->next;
        if (pool->task_head == NULL) {
            pool->task_tail = NULL;
        }
        pool->current_queue_size--;
        pthread_mutex_unlock(&pool->lock);
        
        (task->function)(task->argument);
        free(task);
    }
    return NULL;
}

ThreadPool* threadpool_create(int num_threads, int max_queue_size) {
    if (num_threads <= 0) return NULL;

    ThreadPool* pool = malloc(sizeof(ThreadPool));
    if (!pool) return NULL;

    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (!pool->threads) {
        free(pool);
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->max_queue_size = max_queue_size;
    pool->current_queue_size = 0;
    pool->shutdown = 0;
    pool->task_head = NULL;
    pool->task_tail = NULL;

    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->cond, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker, pool) != 0) {
            pool->shutdown = 1;
            pthread_cond_broadcast(&pool->cond);
            for (int j = 0; j < i; j++) pthread_join(pool->threads[j], NULL);
            free(pool->threads);
            pthread_mutex_destroy(&pool->lock);
            pthread_cond_destroy(&pool->cond);
            free(pool);
            return NULL;
        }
    }
    return pool;
}

int threadpool_enqueue(ThreadPool* pool, void (*function)(void*), void* arg) {
    if (!pool || !function) return -1;

    pthread_mutex_lock(&pool->lock);
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    if (pool->max_queue_size > 0 && 
        pool->current_queue_size >= pool->max_queue_size) {
        pthread_mutex_unlock(&pool->lock);
        return -1; // Queue full
    }

    Task* task = malloc(sizeof(Task));
    if (!task) {
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    task->function = function;
    task->argument = arg;
    task->next = NULL;

    if (!pool->task_tail) {
        pool->task_head = pool->task_tail = task;
    } else {
        pool->task_tail->next = task;
        pool->task_tail = task;
    }
    pool->current_queue_size++;

    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->lock);
    return 0;
}

void threadpool_destroy(ThreadPool* pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->lock);

    pthread_cond_broadcast(&pool->cond);
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    Task* current = pool->task_head;
    while (current) {
        Task* next = current->next;
        free(current);
        current = next;
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    free(pool);
}

int csp_spawn(ThreadPool* pool, void (*func)(void*), void* arg) {
    return threadpool_enqueue(pool, func, arg);
}

// Bad Defaults; IK.
ThreadPool *threadpool_init() { // Just use threadpool_create if this fails
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN); // If you need a long do not use this library
    if (num_cores == -1) {
        return NULL;
    }

    int num_threads = 1.2 * num_cores + 2;

    return threadpool_create(num_threads, 4096);
}

typedef struct Channel {
    pthread_mutex_t lock;
    pthread_cond_t send_cond;
    pthread_cond_t recv_cond;
    void* data;
    void** recv_buffer;
    int closed;
    int has_sender;
    int has_receiver;
} Channel;

Channel* channel_create() {
    Channel* chan = malloc(sizeof(Channel));
    if (!chan) return NULL;

    pthread_mutex_init(&chan->lock, NULL);
    pthread_cond_init(&chan->send_cond, NULL);
    pthread_cond_init(&chan->recv_cond, NULL);
    chan->data = NULL;
    chan->recv_buffer = NULL;
    chan->closed = 0;
    chan->has_sender = 0;
    chan->has_receiver = 0;
    return chan;
}

int channel_send(Channel* chan, void* data) {
    if (!chan) return -1;

    pthread_mutex_lock(&chan->lock);
    if (chan->closed) {
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    while (!chan->has_receiver && !chan->closed) {
        pthread_cond_wait(&chan->send_cond, &chan->lock);
    }

    if (chan->closed) {
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    *chan->recv_buffer = data;
    chan->has_sender = 1;
    pthread_cond_signal(&chan->recv_cond);

    while (chan->has_sender) {
        pthread_cond_wait(&chan->send_cond, &chan->lock);
    }

    pthread_mutex_unlock(&chan->lock);
    return 0;
}

int channel_receive(Channel* chan, void** buffer) {
    if (!chan || !buffer) return -1;

    pthread_mutex_lock(&chan->lock);
    if (chan->closed && !chan->has_sender) {
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    chan->has_receiver = 1;
    chan->recv_buffer = buffer;
    pthread_cond_signal(&chan->send_cond);

    while (!chan->has_sender && !chan->closed) {
        pthread_cond_wait(&chan->recv_cond, &chan->lock);
    }

    if (chan->closed && !chan->has_sender) {
        chan->has_receiver = 0;
        pthread_mutex_unlock(&chan->lock);
        return -1;
    }

    chan->has_sender = 0;
    chan->has_receiver = 0;
    pthread_cond_signal(&chan->send_cond);
    pthread_mutex_unlock(&chan->lock);
    return 0;
}

void channel_close(Channel* chan) {
    if (!chan) return;

    pthread_mutex_lock(&chan->lock);
    chan->closed = 1;
    pthread_cond_broadcast(&chan->send_cond);
    pthread_cond_broadcast(&chan->recv_cond);
    pthread_mutex_unlock(&chan->lock);
}

void channel_destroy(Channel* chan) {
    if (!chan) return;
    
    pthread_mutex_destroy(&chan->lock);
    pthread_cond_destroy(&chan->send_cond);
    pthread_cond_destroy(&chan->recv_cond);
    free(chan);
}
