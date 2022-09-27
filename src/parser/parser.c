#include <stdlib.h>
#include <unistd.h>
#include "parser.h"
#include "../logging.h"

int parse_config(const char* path, struct config* out_config) {
    (void)path;
    out_config->root_path = "blabla";
    return 0;
}

enum parsaer_result {
    PARSING_FINISHED,
    PARSING_CONTINUE,
    PARSING_FAILED
};

static int parse_header (
    const char* buffer,
    uint32_t* index,
    const char** out_header,
    uint32_t* out_header_len,
    const char** out_header_value,
    uint32_t* out_header_value_len
) {
    const char* header = buffer + *index;
    if (*header == '\r') {
        return PARSING_FINISHED;
    }

    const char* separator = strchr(header, ':');
    if (!separator) {
        log_warning("error parsing header");
        return PARSING_FAILED;
    }

    uint32_t header_length = separator - header;

    const char* header_value = separator + 1;
    while(*header_value == ' ')
        ++header_value;

    const char* endline = strchr(header_value , '\r');
    if (!endline) {
        log_warning("error parsing header");
        return PARSING_FAILED;
    }

    uint32_t header_value_len = endline - header_value;

    *out_header = header;
    *out_header_len = header_length;
    *out_header_value = header_value;
    *out_header_value_len = header_value_len;

    *index += (endline + 4) - header;

    return 0;
}

int parse_word(const char* buffer, uint32_t* index, const char** out_word, uint32_t* out_word_len) {
    const char* cursor = buffer + *index;

    while(*cursor == ' ')
        ++cursor;

    const char* word = cursor;

    while(*cursor != ' ' && *cursor != '\r')
        ++cursor;

    uint32_t length = cursor - word;

    *out_word = word;
    *out_word_len = length;

    *index += cursor - buffer;

    return 0;
}

int parse_request(const char* request, struct http_header* out_header, struct http_body* out_body) {
    uint32_t index = 0;

    const char* method;
    uint32_t method_len;
    parse_word(request, &index, &method, &method_len);

    if(strncmp(method, "GET", method_len) == 0) {
        out_header->method = GET;
    } else {
        out_header->method = POST;
    }

    const char* path;
    uint32_t path_len;
    parse_word(request, &index, &path, &path_len);

    out_header->path.data = path;
    out_header->path.length = path_len;

    while(request[index] != '\r')
        ++index;

    index += 2;

    int res = PARSING_CONTINUE;

    const char* header;
    uint32_t header_len;

    const char* header_value;
    uint32_t header_value_len;

    while(res == PARSING_CONTINUE) {
        res = parse_header(request, &index, &header, &header_len, &header_value, &header_value_len);
        if (res == PARSING_FAILED) {
            log_warning("parsing failed");
            return -1;
        }

        if (strncmp(header, "Connection", header_len) == 0) {
            if (strncmp(header_value, "Close", header_value_len) == 0) {
                out_header->connection = CLOSE;
            } else {
                out_header->connection = KEEP_ALIVE;
            }
        }
    }

    index += 2;

    out_body->data = &request[index];
    out_body->length = strlen(request) - (out_body->data - request);

    return 0;
}

static const char* str_codes[] = {
    "200 OK",
    "400 BAD REQUEST",
    "404 NOT FOUND"
};

#define BUFFER_SIZE 4096 * 2

char* construct_response(const char* content, uint16_t code) {
    static const char response[] = "HTTP/1.1 %s\r\nContent-Length: %d\r\n\r\n%s";
    static const char bad_response[] = "HTTP/1.1 %s\r\n\r\n";

    char* buffer = malloc(BUFFER_SIZE * sizeof *buffer);
    if (!buffer) {
        log_error("error allocating memory");
        return NULL;
    }

    int res = 0;

    if (content) {
        res = snprintf(buffer, BUFFER_SIZE, response, str_codes[code], (int)strlen(content), content);
        if ((unsigned int)res <= sizeof response) {
            log_error("error constructing response");
            goto free_buffer;
        }
    } else {
        res = snprintf(buffer, BUFFER_SIZE, bad_response, str_codes[code]);
        if ((unsigned int)res <= sizeof bad_response) {
            log_error("error constructing response");
            goto free_buffer;
        }
    }

    return buffer;

free_buffer:
    free(buffer);

    return NULL;
}
