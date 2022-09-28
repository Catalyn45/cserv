#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include "logging.h"
#include "threadpool.h"

struct thread_context {
    pthread_t thread_handle;
    int clients;
    int sock;
};

struct thread_pool {
    uint32_t n_threads;
    struct thread_context* threads;
};

struct thread_task {
    thread_pool_task_t task;
    void* args;
};

struct worker_args {
    int sock;
    uint32_t* clients;
};

static void* worker(void* args) {
    struct worker_args* wargs = args;
    int sock = wargs->sock;
    uint32_t* clients = wargs->clients;
    free(args);

    struct thread_task task;

    int res;
    while((res = recv(sock, &task, sizeof task, 0)) > 0) {
        ++*clients;
        task.task(task.args);
        --*clients;
    }

    return NULL;
}

struct thread_pool* thread_pool(uint32_t threads) {
    struct thread_pool* pool = malloc(sizeof *pool);
    if(!pool) {
        log_error("error allocating memory");
        return NULL;
    }

    *pool = (struct thread_pool) {
        .n_threads = threads
    };

    pool->threads = malloc(threads * sizeof *pool->threads);
    if (!pool->threads) {
        log_error("error alocating memory");
        goto free_pool; 
    }

    return pool;

free_pool:
    free(pool);

    return NULL;
}

int thread_pool_start(struct thread_pool* pool) {
    for (uint32_t i = 0; i < pool->n_threads; ++i) {
        struct worker_args* args = malloc(sizeof *args);
        if (!args) {
            log_error("failed to allocate memory");
            return -1;
        }

        int res = pthread_create (
            &pool->threads[i].thread_handle,
            NULL,
            worker,
            args
        );

        if (res != 0) {
            free(args);
            log_error("error creating threads");
            return -1;
        }
    }

    return 0;
}

void thread_pool_free(struct thread_pool* pool) {
    for (uint32_t i = 0; i < pool->n_threads; ++i) {
        pthread_exit(&pool->threads[i].thread_handle);
        pthread_join(pool->threads[i].thread_handle,  NULL);
    }

    free(pool);
}

int thread_pool_enqueue(thread_pool_task_t task, void* args) {
    return 0;
}
