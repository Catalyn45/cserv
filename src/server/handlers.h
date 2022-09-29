#ifndef _HANDLERS_H_
#define _HANDLERS_H_

#include "../threadpool.h"
#include "../cache/cache.h"

void* handle_client(struct cache* memory_cache, struct thread_pool* pool, int client);

#endif
