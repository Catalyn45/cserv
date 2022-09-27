#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdint.h>

struct config {
    const char* root_path;
};

enum connection_types {
    CLOSE,
    KEEP_ALIVE
};

enum methods {
    GET,
    POST
};

enum codes {
    OK,
    BAD_REQUESTS,
    NOT_FOUND
};

struct http_header {
    struct {
        const char* data;
        uint32_t length;
    } path;

    int method;
    int connection;
};

struct http_body {
    const char* data;
    uint32_t length;
};

int parse_config(const char* path, struct config* out_config);
int parse_request(const char* request, struct http_header* out_header, struct http_body* out_body);

char* construct_response(const char* content, uint16_t code);

#endif
