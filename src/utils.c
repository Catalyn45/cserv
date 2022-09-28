#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "utils.h"
#include "logging.h"

char* read_file(const char* path, uint32_t* out_size) {
    int file = open(path, O_RDONLY);
    if(file == -1) {
        log_errno("unable to open file: %s", path);
        return NULL;
    }

    int size = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET);

    char* content = malloc((size + 1) * sizeof *content);
    if (!content) {
        log_error("failed to allocate memory");
        goto close_file;
    }

    if (read(file, content, size) <= 0) {
        log_error("failed to read");
        goto close_file;
    }

    close(file);

    content[size] = '\0';
    *out_size = size;

    return content;

close_file:
    close(file);

    return NULL;
}
