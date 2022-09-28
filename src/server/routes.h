#ifndef _ROUTES_H_
#define _ROUTES_H_

#include "../parser/parser.h"

typedef int (*route_handler_t)(const struct http_header* header, const struct http_body* body, struct http_response* out_response);

extern route_handler_t route_handlers[];

#endif
