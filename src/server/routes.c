#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "../logging.h"
#include "routes.h"

static int post_test(const struct http_header* header, const struct http_body* body, struct http_response* out_response) {
    (void)body;

    if (header->method != POST) {
        return -1;
    }

    if (strncmp(header->path.data, "/test", header->path.length) != 0) {
        return -1;
    }

    out_response->code = OK;

    char* data = malloc(5 * sizeof *data);
    if (!data) {
        log_warning("error alocating memory");
        return -1;
    }

    strcpy(data, "test");

    out_response->data = data;
    out_response->length = 5;

    return 0;
}

route_handler_t route_handlers[] = {
    post_test,
    NULL
};
