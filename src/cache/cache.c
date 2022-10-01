#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/queue.h>
#include "../logging.h"
#include "../security/hashing.h"
#include "../utils.h"
#include "cache.h"

#define DEFAULT_SIZE 300  // 20 MB

#define BUCKETS 50

struct entry {
    uint32_t hash;
    char* data;
    uint32_t length;

    STAILQ_ENTRY(entry) entries;
};

STAILQ_HEAD(char_list, entry);

struct cache {
    uint64_t max_size;
    uint64_t allocated_memory;
    pthread_mutex_t lock;
    struct char_list memory[BUCKETS];
};

struct cache* cache(uint64_t max_size) {
    if (max_size == 0)
        max_size = DEFAULT_SIZE;

    struct cache* memory_cache = malloc(sizeof *memory_cache);
    if (!memory_cache) {
        log_error("error alocating memory");
        return NULL;
    }

    memory_cache->allocated_memory = 0;
    memory_cache->max_size = max_size;

    for (uint32_t i = 0; i < BUCKETS; ++i) {
        STAILQ_INIT(memory_cache->memory + i);
    }

    if (pthread_mutex_init(&memory_cache->lock, NULL) != 0) {
        log_errno("error creating mutex");
        goto free_memory;
    }

    return memory_cache;

free_memory:
    free(memory_cache);

    return NULL;
}

static void free_entry(struct entry* en) {
    free(en->data);
    free(en);
}

static void* memndup(void* memory, uint32_t length) {
    void* new_mem = malloc(length);
    if (!new_mem)
        return NULL;

    memcpy(new_mem, memory, length);
    return new_mem;
}

char* cache_get_file(struct cache* cache, const char* path, uint32_t* length) {
    uint32_t hash = crc_32((const unsigned char*)path, *length);

    struct char_list* head = cache->memory + (hash % BUCKETS);

    struct entry* item;

    pthread_mutex_lock(&cache->lock);

    STAILQ_FOREACH(item, head, entries) {
        if(item->hash == hash) {
            *length = item->length;
            char* content = memndup(item->data, *length);
            pthread_mutex_unlock(&cache->lock);
            return content;
        }
    }

    char* content = read_file(path, length);
    if (!content) {
        log_error("error reading file");
        pthread_mutex_unlock(&cache->lock);
        return NULL;
    }

    if (*length > cache->max_size) { // not saving in cache if too big (sorry not sorry)
        pthread_mutex_unlock(&cache->lock);
        return content;
    }

    // free memory until we have enough
    if(cache->allocated_memory + *length > cache->max_size) {
        for (uint32_t i = 0; i < BUCKETS; i++) {
            struct char_list* list = cache->memory + i;
            if (STAILQ_EMPTY(list))
                continue;

            while(cache->allocated_memory + *length > cache->max_size) {
                struct entry* data = STAILQ_FIRST(list);
                STAILQ_REMOVE_HEAD(list, entries);
                cache->allocated_memory -= data->length;
                free_entry(data);
            }
        }
    }

    struct entry* new_item = malloc(sizeof *new_item);
    if (!new_item) {
        log_error("error alocating memory");
        goto free_content;
    }

    *new_item = (struct entry) {
        .hash = hash,
        .data = content,
        .length = *length
    };

    STAILQ_INSERT_TAIL(head, new_item, entries);
    cache->allocated_memory += *length;

    pthread_mutex_unlock(&cache->lock);

    return content;

free_content:
    pthread_mutex_unlock(&cache->lock);

    free(content);

    return NULL;

}

void cache_free(struct cache* cache) {
    for (uint32_t i = 0; i < BUCKETS; i++) {
        struct char_list* list = cache->memory + i;
        while (!STAILQ_EMPTY(list)) {
            struct entry* data = STAILQ_FIRST(list);
            STAILQ_REMOVE_HEAD(list, entries);
            free_entry(data);
        }
    }

    pthread_mutex_destroy(&cache->lock);
    free(cache);
}
