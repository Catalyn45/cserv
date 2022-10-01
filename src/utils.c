#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "utils.h"
#include "logging.h"

char* read_file(const char* path, uint32_t* length) {
    char buffer[256];
    strncpy(buffer, path, *length);
    buffer[*length] = '\0';

    int file = open(buffer, O_RDONLY);
    if(file == -1) {
        log_errno("unable to open file: %s", buffer);
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
    *length = size + 1;

    return content;

close_file:
    close(file);

    return NULL;
}
