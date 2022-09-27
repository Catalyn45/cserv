#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "handlers.h"
#include "../logging.h"
#include "../parser/parser.h"
#include "../utils.h"

#define BUFFER_SIZE 4096

int handle_client(int client) {
    char buffer[BUFFER_SIZE];

    int res = recv(client, buffer, BUFFER_SIZE - 1, 0);
    if (res == -1) {
        log_warning("error at recv");
        goto close_client;
    }

    buffer[res] = '\0';

    struct http_header header;
    struct http_body body;

    res = parse_request(buffer, &header, &body);
    if (res != 0) {
        log_warning("error parsing request");
        goto close_client;
    }

    char path[BUFFER_SIZE] = ".";
    strncat(path, header.path.data, header.path.length);

    log_info("path: %s", path);
    int code = OK;

    char* content = read_file(path);
    if (!content) {
        log_warning("error at reading file");
        code = NOT_FOUND;
    }

    char* response = construct_response(content, code);
    free(content);

    if (!response) {
        log_warning("error creating response");
        goto close_client;
    }

    res = send(client, response, strlen(response), 0);
    free(response);

    if (res == -1) {
        log_warning("error at send");
        goto close_client;
    }

close_client:
    close(client);

    return 0;
}
