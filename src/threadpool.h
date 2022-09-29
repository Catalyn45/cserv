#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_
#include <stdint.h>

typedef int (*thread_pool_task_t)(void* args);

struct thread_pool* thread_pool(uint32_t threads);

int thread_pool_start(struct thread_pool* pool);

void thread_pool_free(struct thread_pool* pool);

int thread_pool_enqueue(struct thread_pool* pool, thread_pool_task_t task, void* args);

#endif
