#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../logging.h"
#include "server.h"
#include "handlers.h"

struct cserv_server {
    const struct server_args* args;
    struct sockaddr_in listen_address;
    int listen_socket;
};

static int init_listen_addr(const char* ip, uint16_t port, struct sockaddr_in* out_addr) {
    *out_addr = (struct sockaddr_in) {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };

    if (inet_pton(AF_INET, ip, &out_addr->sin_addr) != 1) {
        log_errno("error converting ip to addr");
        return -1;
    }

    return 0;
}

struct cserv_server* cserv_server(const struct server_args* args) {
    struct cserv_server* server = malloc(sizeof *server);
    if (!server) {
        log_errno("error allocating memory for server");
        return NULL;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1) {
        log_errno("error creating socket");
        goto free_server;
    }

    int reuse = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
        log_errno("error seting socket option");
        goto close_socket;
    }

    int res = init_listen_addr(args->listen_ip, args->listen_port, &server->listen_address);
    if (res != 0) {
        log_error("error initializing listening address");
        goto close_socket;
    }

    res = bind(server_socket, (const struct sockaddr*)&server->listen_address, sizeof server->listen_address);
    if (res != 0) {
        log_errno("error binding socket");
        goto close_socket;
    }

    *server = (struct cserv_server) {
        .args = args,
        .listen_socket = server_socket
    };

    return server;

close_socket:
    close(server_socket);

free_server:
    free(server);

    return NULL;
}

int cserv_start(struct cserv_server* server) {
    int server_socket = server->listen_socket;

    log_info("starting server");
    if (listen(server_socket, SOMAXCONN) != 0) {
        log_errno("error listening socket");
        return -1;
    }

    log_info("server started, listening for clients");
    while (1) {
        int client = accept(server_socket, NULL, NULL);
        if (client == -1) {
            if (errno != ECONNABORTED) {
                log_errno("Error accepting client");
                return -1;
            }

            log_warning("connection aborted");
        }

        log_info("client connected");
        handle_client(client);
    }

    return 0;
}

void cserv_free(struct cserv_server* server) {
    close(server->listen_socket);

    free(server);
}
