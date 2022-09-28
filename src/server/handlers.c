#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "handlers.h"
#include "routes.h"
#include "../logging.h"
#include "../parser/parser.h"
#include "../utils.h"

#define BUFFER_SIZE 4096

static int file_handler(const struct http_header* header, struct http_response* out_response) {
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

    uint32_t length = 0;
    char* content = read_file(path, &length);
    if (!content) {
        log_warning("error at reading file");
        code = NOT_FOUND;
    }

    out_response->code = code;
    out_response->data = content;
    out_response->length = length;

    return 0;
}

static int handle_request(const struct http_header* header, const struct http_body* body, struct http_response* out_response) {
    uint32_t index = 0;
    route_handler_t handler = route_handlers[index];

    while (handler) {
        int res = handler(header, body, out_response);
        if (res == 0)
            return 0;

        handler = route_handlers[++index];
    }

    if(file_handler(header, out_response) != 0) {
        out_response->code = BAD_REQUEST;
        out_response->data = NULL;
        out_response->length = 0;
    }

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

void* handle_client(int client) {
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
    res = handle_request(&header, &body, &response);
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
    close(client);

    return NULL;
}
