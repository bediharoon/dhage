# dhage
A 'go' at implementing CSP-style preemptive concurrency in C.
The goal is make multi-threading in C about as easy and efficient as it in Golang.

## Docs
---
### Thread Pool API

#### `typedef struct ThreadPool ThreadPool;`
A structure representing a thread pool.

#### `ThreadPool* threadpool_init();`
Initializes a new thread pool with default settings.
This might fail on esoteric systems (if so, please use `threadpool\_create`).

**Returns:**
- Pointer to the created `ThreadPool` instance.

#### `ThreadPool* threadpool_create(int num_threads, int max_queue_size);`
Creates a thread pool with a specified number of threads and a maximum queue size for tasks.

**Parameters:**
- `num_threads`: Number of threads in the pool.
- `max_queue_size`: Maximum number of tasks that can be queued.

**Returns:**
- Pointer to the created `ThreadPool` instance.

#### `int csp_spawn(ThreadPool* pool, void (*func)(void*), void* arg);`
A wrapper around `threadpool_enqueue` for goroutine-style task spawning.

**Parameters:**
- `pool`: Pointer to the `ThreadPool` instance.
- `func`: Function pointer representing the task to execute.
- `arg`: Argument to be passed to the function.

**Returns:**
- `0` on success, nonzero on failure.

#### `void threadpool_destroy(ThreadPool* pool);`
Gracefully shuts down and destroys the thread pool.

**Parameters:**
- `pool`: Pointer to the `ThreadPool` instance.

---

### Channel API

#### `typedef struct Channel Channel;`
A structure representing a communication channel.

#### `Channel* channel_create(void);`
Creates a new channel.

**Returns:**
- Pointer to the created `Channel` instance.

#### `int channel_send(Channel* chan, void* data);`
Sends data through the channel. This function blocks until a receiver is ready.

**Parameters:**
- `chan`: Pointer to the `Channel` instance.
- `data`: Pointer to the data to be sent.

**Returns:**
- `0` on success, nonzero on failure.

#### `int channel_receive(Channel* chan, void** buffer);`
Receives data from the channel. This function blocks until a sender is ready.

**Parameters:**
- `chan`: Pointer to the `Channel` instance.
- `buffer`: Pointer to store the received data.

**Returns:**
- `0` on success, nonzero on failure.

#### `void channel_close(Channel* chan);`
Closes the channel and wakes up blocked threads.

**Parameters:**
- `chan`: Pointer to the `Channel` instance.

#### `void channel_destroy(Channel* chan);`
Destroys the channel and frees associated resources.

**Parameters:**
- `chan`: Pointer to the `Channel` instance.


