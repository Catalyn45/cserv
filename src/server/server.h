#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdint.h>

struct server_args {
    char* listen_addr;
    uint16_t listen_port;
};


struct cserv_server* cserv_server(const struct server_args* args);

int cserv_start(struct cserv_server* server);

void cserv_stop(struct cserv_server* server);

void cserv_free(struct cserv_server* server);


#endif
