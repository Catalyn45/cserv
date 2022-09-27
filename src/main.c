#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "logging.h"
#include "server/server.h"

struct cserv_server* server;

void close_handler(int signo) {
    (void) signo;
    log_info("closing server");
    if (server) {
        cserv_free(server);
    }

    exit(0);
}

int parse_args(int argc, char* argv[], struct server_args* out_args) {
    *out_args = (struct server_args) {
        .listen_ip = "0.0.0.0",
        .listen_port = 80
    };

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--listen") == 0) {
            out_args->listen_ip = argv[i+1];
            ++i;
        } else if (strcmp(argv[i], "--port") == 0) {
            out_args->listen_port = atoi(argv[i+1]);
            ++i;
        } else {
            log_error("invalid parameter: %s", argv[i]);
            return -1;
        }
    }

    return 0;
}


int main(int argc, char* argv[]) {
    log_info("parsing args");
    struct server_args args;
    if(parse_args(argc, argv, &args) != 0) {
        log_error("error parsing parameters");
        return -1;
    }

    log_info("initiating server");
    server = cserv_server(&args);
    if (!server) {
        log_error("error creating the web server");
        return -1;
    }

    if (signal(SIGINT, close_handler) == SIG_ERR) {
        log_errno("error creating close handler");
        goto free_serv;
    }

    log_info("starting server");
    if(cserv_start(server) != 0) {
        log_error("error starting server");
        goto free_serv;
    }

    log_info("freeing server");
    cserv_free(server);

    return 0;

free_serv:
    cserv_free(server);

    return -1;
}
