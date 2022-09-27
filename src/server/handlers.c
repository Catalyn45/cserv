#include <unistd.h>
#include <sys/socket.h>
#include "handlers.h"
#include "../logging.h"
#include "../parser/parser.h"

static const char test_response[] = "HTTP/1.1 200 OK\r\nContent-Length: 48\r\n\r\n<html><body><h1> Hello world </h1></body></html>";
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

    char path[BUFFER_SIZE];
    strncpy(path, header.path.data, header.path.length);
    path[header.path.length] = '\0';

    log_info("path: %s", path);

    res = send(client, test_response, sizeof test_response, 0);
    if (res == -1) {
        log_warning("error at send");
        goto close_client;
    }

close_client:
    close(client);

    return 0;
}
