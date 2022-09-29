#ifndef _PATH_CHECKERS_H_
#define _PATH_CHECKERS_H_

#include <stdint.h>

typedef int (*path_checker_t)(const char* path, uint32_t length);

extern path_checker_t path_checkers[];

#endif
