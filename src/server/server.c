#include <stdlib.h>
#include "../logging.h"
#include "server.h"

struct cserv_server {
    const struct server_args* args;
};

struct cserv_server* cserv_server(const struct server_args* args) {
    struct cserv_server* server = malloc(sizeof *server);
    if (!server) {
        log_errno("error allocating memory for server");
        return NULL;
    }

    *server = (struct cserv_server) {
        .args = args
    };

    return server;
}

int cserv_start(struct cserv_server* server) {
    (void) server;
    return 0;
}

void cserv_stop(struct cserv_server* server) {
    (void) server;
}

void cserv_free(struct cserv_server* server) {
    free(server);
}
