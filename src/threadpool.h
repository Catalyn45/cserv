#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_
#include <stdint.h>

typedef int (*async_task_t)(int fd, uint32_t what, void* args);

enum TASK_RESULT {
    TASK_SUCCESS,
    TASK_FAILED,
    TASK_CLOSE
};

struct event_context {
    int fd;
    uint32_t what;
    async_task_t task;
    void* args;
};

struct thread_pool* thread_pool(uint32_t threads);

int thread_pool_start(struct thread_pool* pool);

void thread_pool_free(struct thread_pool* pool);

int thread_pool_enqueue(struct thread_pool* pool, struct event_context* context);

#endif
