#ifndef _PARSER_H_
#define _PARSER_H_

struct config {
    const char* root_path;
};

int parse_config(const char* path, struct config* out_config);


#endif
