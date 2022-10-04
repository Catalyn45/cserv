#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "handlers.h"
#include "routes.h"
#include "../security/path_checkers.h"
#include "../logging.h"
#include "../parser/parser.h"
#include "../utils.h"
#include "../threadpool.h"

#define BUFFER_SIZE 4096

static int file_handler(const struct http_header* header, struct http_response* out_response, struct cache* memory_cache) {
    if(header->method != GET)
        return -1;

    char path[BUFFER_SIZE] = ".";
    if (header->path.length == 1 && *header->path.data == '/') {
        strcat(path, "/index.html");
    } else {
        strncat(path, header->path.data, header->path.length);
    }

    log_info("path: %s", path);
    int code = OK;

    uint32_t length = strlen(path);
    char* content = cache_get_file(memory_cache, path, &length);
    if (!content) {
        log_warning("error at reading file");
        code = NOT_FOUND;
    }

    out_response->code = code;
    out_response->data = content;
    out_response->length = length;

    return 0;
}

static int handle_request(const struct http_header* header, const struct http_body* body, struct http_response* out_response, struct cache* memory_cache) {
    path_checker_t* checker = path_checkers;
    while(*checker) {
        if ((*checker)(header->path.data, header->path.length) == 0)
            goto send_400;

        ++checker;
    }

    route_handler_t* handler = route_handlers;
    while (*handler) {
        int res = (*handler)(header, body, out_response);
        if (res == 0)
            return 0;

        ++handler;
    }

    if(file_handler(header, out_response, memory_cache) != 0)
        goto send_400;

    return 0;

send_400:
    out_response->code = BAD_REQUEST;
    out_response->data = NULL;
    out_response->length = 0;

    return 0;
}

static int read_request(int client, char* request, struct http_header* header, struct http_body* body) {
    uint32_t request_len = 0;
    request[0] = '\0';

    char* buffer = request;

    int res = 0;

    while(!strstr(request, "\r\n\r\n")) {
        res = recv(client, buffer, BUFFER_SIZE - request_len - 1, 0);
        if (res <= 0) {
            log_warning("error at recv");
            return -1;
        }

        buffer[res] = '\0';

        request_len += res;
        buffer += res;
    }

    res = parse_request(request, header, body);
    if (res != 0) {
        log_warning("error parsing request");
        return -1;
    }

    uint32_t remaining_len = header->content_length;
    remaining_len -= body->length;

    buffer += (body->data - request) + body->length;
    while(remaining_len > 0) {
        res = recv(client, buffer, remaining_len, 0);
        if (res <= 0) {
            log_warning("error at recv");
            return -1;
        }

        buffer[res] = '\0';

        remaining_len -= res;
        buffer += res;
    }

    body->length = header->content_length;

    return 0;
}

static int handle_response(int client, const struct http_response* response) {
    uint32_t length;
    char* output = construct_response(response->data, response->code, &length);
    if (!response) {
        log_warning("error creating response");
        return -1;
    }

    int res = send(client, output, length, 0);
    free(output);
    if (res == -1) {
        log_warning("error at send");
        return -1;
    }

    return 0;
}

static int task_handler(int client, uint32_t what, void* args) {
    struct cache* memory_cache = args;

    if (!(what & EPOLLIN))
        return 0;

    char request[BUFFER_SIZE];

    struct http_header header;
    struct http_body body;
    log_debug("reading request");
    int res = read_request(client, request, &header, &body);
    if (res != 0) {
        log_warning("failed to read request");
        goto close_client;
    }

    struct http_response response;
    log_debug("handling request");
    res = handle_request(&header, &body, &response, memory_cache);
    if (res != 0) {
        log_warning("failed to handle request");
        goto close_client;
    }

    log_debug("sending response");
    res = handle_response(client, &response);
    if (res != 0) {
        log_warning("failed to send response");
        goto close_client;
    }

close_client:

    return TASK_CLOSE;
}

void* handle_client(struct cache* memory_cache, struct thread_pool* pool, int client) {
    struct event_context context = {
        .fd = client,
        .what = EPOLLIN,
        .task = task_handler,
        .args = memory_cache
    };

    int res = thread_pool_enqueue(pool, &context);
    if (res != 0) {
        log_error("error enqueueing task");
        return NULL;
    }

    return NULL;
}
