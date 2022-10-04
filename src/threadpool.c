#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <sys/epoll.h>
#include "logging.h"
#include "threadpool.h"

struct thread_pool {
    uint32_t n_threads;
    struct thread_context* threads;
};

struct thread_context {
    pthread_t thread_handle;
    uint32_t clients;
    int sock;
};

struct worker_args {
    int sock;
    int epoll;
    uint32_t* clients;
};

static int socketpair_task(int fd, uint32_t what, void* args) {
    (void)what;

    struct worker_args* wargs = args;
    int epoll = wargs->sock;
    uint32_t* clients = wargs->clients;

    struct event_context ctx;
    ssize_t res = recv(fd, &ctx, sizeof ctx, 0);
    if (res <= 0 || ctx.fd == -1) {
        goto del_fd;
    }

    struct epoll_event client_event = {
        .data.ptr = &ctx,
        .events = ctx.what
    };

    res = epoll_ctl(epoll, EPOLL_CTL_ADD, ctx.fd, &client_event);
    if (res == -1) {
        log_errno("error adding client socket to epoll");
        goto del_fd;
    }

    ++*clients;

    return TASK_SUCCESS;

del_fd:
    epoll_ctl(epoll, EPOLL_CTL_DEL, fd, NULL);

    close(fd);

    close(epoll);

    pthread_exit(NULL);

    return -1;

}

static inline void handle_res(int epoll, struct event_context* context, uint32_t* clients, int res) {
    switch(res) {
    case TASK_CLOSE:
        epoll_ctl(epoll, EPOLL_CTL_DEL, context->fd, NULL);
        close(context->fd);
        --*clients;
        break;
    }
}

static void* worker(void* args) {
    struct worker_args* wargs = args;
    int sock = wargs->sock;
    uint32_t* clients = wargs->clients;
    free(wargs);

    int epoll = epoll_create1(0);
    if (!epoll) {
        log_error("error creating epoll");
        goto close_socket;
    }

    struct worker_args socketpair_args = {
        .sock = epoll,
        .clients = clients
    };

    struct event_context socketpair_ctx = {
        .fd = sock,
        .what = EPOLLIN,
        .task = socketpair_task,
        .args = &socketpair_args
    };

    struct epoll_event socketpair_event = {
        .data.ptr = &socketpair_ctx,
        .events = socketpair_ctx.what
    };

    int res = epoll_ctl(epoll, EPOLL_CTL_ADD, sock, &socketpair_event);
    if (res == -1) {
        log_error("error adding socketpair to list");
        goto close_epoll;
    }

    struct epoll_event events[64];
    while (true) {
        int n_events = epoll_wait(epoll, events, sizeof events, -1);
        if (n_events == -1) {
            log_error("failed waiting");
            goto close_epoll;
        }

        for (int i = 0; i < n_events; ++i) {
            struct event_context* context = events[i].data.ptr;

            if (events[i].events & EPOLLRDHUP) {
                epoll_ctl(epoll, EPOLL_CTL_DEL, context->fd, NULL);
                close(context->fd);
                --*clients;
            }

            res = context->task(context->fd, events[i].events, context->args);
            handle_res(epoll, context, clients, res);
        }
    }

    return NULL;

close_epoll:
    close(epoll);

close_socket:
    close(sock);

    return NULL;
}

struct thread_pool* thread_pool(uint32_t threads) {
    struct thread_pool* pool = malloc(sizeof *pool);
    if(!pool) {
        log_error("error allocating memory");
        return NULL;
    }

    if (threads == 0)
        threads = get_nprocs() - 1;

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

void thread_pool_free(struct thread_pool* pool) {
    struct event_context close_event = {
        .fd = -1
    };

    for (uint32_t i = 0; i < pool->n_threads; ++i) {
        log_debug("freeing thread %d", i);
        send(pool->threads[i].sock, &close_event, sizeof close_event, 0);
        close(pool->threads[i].sock);
        pthread_join(pool->threads[i].thread_handle,  NULL);
    }

    free(pool->threads);
    free(pool);
}

int thread_pool_enqueue(struct thread_pool* pool, struct event_context* context) {
    uint32_t min_worker = 0;
    uint32_t min_worker_value = pool->threads[0].clients;

    for (uint32_t i = 1; i < pool->n_threads; ++i) {
        uint32_t clients = pool->threads[i].clients;
        if (clients < min_worker_value) {
            min_worker = i;
            min_worker_value = clients;
        }
    }

    struct thread_context* worker_context = &pool->threads[min_worker];

    log_debug("sending task to thread: %d", min_worker);
    int res = send(worker_context->sock, context, sizeof *context, 0);
    if (res <= 0) {
        log_errno("error sending to thread");
        return -1;
    }

    return 0;
}
