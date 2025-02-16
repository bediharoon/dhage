#include <pthread.h>
#include <stdlib.h>

// Linked list for the tasks (maybe prio queues, eventually?)
typedef struct Task {
    void (*function)(void*);
    void* argument;
    struct Task* next;
} Task;

typedef struct ThreadPool {
    pthread_t* threads;
    int num_threads;
    volatile int shutdown; 
    pthread_mutex_t lock;
    pthread_cond_t cond;
    Task* task_head;
    Task* task_tail;
} ThreadPool;

// Cooperative Round-Robin (I know... I know...)
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
        pthread_mutex_unlock(&pool->lock);
        
        (task->function)(task->argument);
        free(task);
    }
    return NULL;
}

ThreadPool* threadpool_create(int num_threads) {
    if (num_threads <= 0) return NULL;

    ThreadPool* pool = malloc(sizeof(ThreadPool));
    if (!pool) return NULL;

    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (!pool->threads) {
        free(pool);
        return NULL;
    }

    pool->num_threads = num_threads;
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

int threadpool_enqueue(ThreadPool* pool, void (*function)(void*), void* arg) {
    if (!pool || !function) return -1;

    Task* task = malloc(sizeof(Task));
    if (!task) return -1;

    task->function = function;
    task->argument = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->lock);
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        free(task);
        return -1;
    }

    if (!pool->task_tail) {
        pool->task_head = pool->task_tail = task;
    } else {
        pool->task_tail->next = task;
        pool->task_tail = task;
    }

    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->lock);
    return 0;
}
