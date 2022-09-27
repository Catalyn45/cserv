#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define log(kind, format, ...) \
    printf(kind format ", FILE: " __FILE__ ", LINE: %d\n", __VA_ARGS__ __VA_OPT__(,) __LINE__)

#define log_errno(format, ...) \
    log("\033[0;31m[ERROR] ", format ", ERRNO: %s", __VA_ARGS__ __VA_OPT__(,) strerror(errno))

#define log_error(format, ...) \
    log("\033[0;31m[ERROR] ", format, __VA_ARGS__)

#define log_info(format, ...) \
    log("\033[0;37m[INFO]  ", format, __VA_ARGS__)

#define log_debug(format, ...) \
    log("\033[0;36m[DEBUG] ", format, __VA_ARGS__)

#define log_warning(format, ...) \
    log("\033[0;33m[WARN]  ", format, __VA_ARGS__)

#endif
