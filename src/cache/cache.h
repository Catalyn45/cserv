#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

struct cache* cache(uint64_t max_size);

char* cache_get_file(struct cache* cache, const char* path, uint32_t* length);

void cache_free(struct cache* cache);

#endif
