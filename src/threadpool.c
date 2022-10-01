#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include "logging.h"
#include "threadpool.h"

struct thread_context {
    pthread_t thread_handle;
    uint32_t clients;
    int sock;
};

struct thread_pool {
    uint32_t n_threads;
    struct thread_context* threads;
};

struct pool_args {
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

    struct pool_args task;

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

    if (threads == 0)
        threads = get_nprocs();

    log_info("found %d cores available", threads);
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

        int socks[2];
        int res = socketpair(AF_UNIX, SOCK_STREAM, 0, socks);
        if (res != 0) {
            free(args);
            log_errno("error creating socketpair");
            return -1;
        }

        pool->threads[i].sock = socks[0];
        args->sock = socks[1];
        args->clients = &pool->threads[i].clients;

        res = pthread_create (
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

static int exit_task(void* args) {
    (void)args;
    pthread_exit(NULL);
}

void thread_pool_free(struct thread_pool* pool) {
    for (uint32_t i = 0; i < pool->n_threads; ++i) {
        log_debug("freeing thread %d", i);
        thread_pool_enqueue(pool, exit_task, NULL);
        pthread_join(pool->threads[i].thread_handle,  NULL);
    }

    free(pool->threads);
    free(pool);
}

int thread_pool_enqueue(struct thread_pool* pool, thread_pool_task_t task, void* args) {
    uint32_t min_worker = 0;
    uint32_t min_worker_value = pool->threads[0].clients;

    for (uint32_t i = 1; i < pool->n_threads; ++i) {
        uint32_t clients = pool->threads[i].clients;
        if (clients < min_worker_value) {
            min_worker = i;
            min_worker_value = clients;
        }
    }

    struct thread_context* context = &pool->threads[min_worker];

    struct pool_args p_args = {
        .task = task,
        .args = args
    };
    log_debug("sending task to thread: %d", min_worker);
    int res = send(context->sock, &p_args, sizeof p_args, 0);
    if (res <= 0) {
        log_errno("error sending to thread");
        return -1;
    }

    return 0;
}
