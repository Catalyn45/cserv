#include <unistd.h>
#include <string.h>

#include "path_checkers.h"

// check if the path contains a double dot
static int double_dot(const char* path, uint32_t length) {
    if (length < 2)
        return -1;

    uint32_t first = 0;
    uint32_t second = 1;

    while(second < length) {
        if (path[first] == '.' && path[second] == '.')
            return 0;
        ++first;
        ++second;
    }

    return -1;
}

path_checker_t path_checkers[] = {
    double_dot,
    NULL
};
