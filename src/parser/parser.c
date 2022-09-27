#include "parser.h"

int parse_config(const char* path, struct config* out_config) {
    (void)path;
    out_config->root_path = "blabla";
    return 0;
}
