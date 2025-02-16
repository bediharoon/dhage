#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "dhage.h"

/* BENCHMARKS FOR OVERHEAD, mostly ;)
 * $ gcc -o benchmark tests.c channels.c threads.c -pthread -O3
 */

// Shared counter and mutex for benchmarking
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int task_counter = 0;

// Simple task that just increments the counter
void benchmark_task(void* arg) {
    pthread_mutex_lock(&counter_mutex);
    task_counter++;
    pthread_mutex_unlock(&counter_mutex);
}

// Benchmark function measuring task throughput
void run_benchmark() {
    const int NUM_TASKS = 100000;
    clock_t start, end;

    ThreadPool* pool = threadpool_init();
    if (!pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        exit(1);
    }

    printf("Starting benchmark with %d tasks...\n", NUM_TASKS);
    start = clock();

    // Submit all tasks
    for(int i = 0; i < NUM_TASKS; i++) {
        if(csp_spawn(pool, benchmark_task, NULL) != 0) {
            fprintf(stderr, "Failed to enqueue task %d\n", i);
            exit(1);
        }
    }

    // Wait for completion
    while(1) {
        pthread_mutex_lock(&counter_mutex);
        int current = task_counter;
        pthread_mutex_unlock(&counter_mutex);
        if(current >= NUM_TASKS) break;
        usleep(1000); // Prevent busy waiting
    }

    end = clock();
    double duration = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Completed %d tasks in %.2f seconds\n", NUM_TASKS, duration);
    printf("Throughput: %.2f tasks/second\n", NUM_TASKS / duration);

    threadpool_destroy(pool);
}

int main() {
    run_benchmark();
    return 0;
}
