#include <gtest/gtest.h>

extern "C" {
#include "../src/parser/parser.h"
}

static const char get_request[] =
    "GET /index.html HTTP/1.1\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";

static const char post_request[] =
    "POST /upload HTTP/1.1\r\n"
    "Content-Length: 16\r\n"
    "Connection: close\r\n"
    "\r\n"
    R"({"key": "value"})";

TEST(parser, get_request) {
    struct http_header header;
    struct http_body body;

    ASSERT_EQ(parse_request(get_request, &header, &body), 0);

    ASSERT_EQ(strncmp(header.path.data, "/index.html", header.path.length), 0);

    ASSERT_EQ(header.method, GET);

    ASSERT_EQ(header.connection, KEEP_ALIVE);

    ASSERT_EQ(body.length, 0);
}

TEST(parser, post_request) {
    struct http_header header;
    struct http_body body;

    ASSERT_EQ(parse_request(post_request, &header, &body), 0);

    ASSERT_EQ(strncmp(header.path.data, "/upload", header.path.length), 0);

    ASSERT_EQ(header.method, POST);

    ASSERT_EQ(header.connection, CLOSE);

    ASSERT_EQ(header.content_length, 16);

    ASSERT_EQ(body.length, 16);

    ASSERT_EQ(strncmp(body.data, R"({"key": "value"})", body.length), 0);
}

