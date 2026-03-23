// credit tsoding => https://www.youtube.com/watch?v=gtk3RZHwJUA
#ifndef DYNARR_H
#define DYNARR_H

#include <stdlib.h>

typedef struct {
   size_t count;
   size_t capacity;
} Header;

#define ARR_INIT_CAP 1

#define arr_push(arr, x) \
    do { \
        if (arr == NULL) { \
            Header* header = malloc(sizeof(*arr)*ARR_INIT_CAP + sizeof(Header)); \
            header->count = 0; \
            header->capacity = ARR_INIT_CAP; \
            arr = (void*)(header + 1); \
        } \
        Header* header = (Header*)(arr) - 1; \
        if (header->count >= header->capacity) { \
            size_t new_cap = header->capacity *= 2; \
            void* tmp = realloc(header, sizeof(*arr) * new_cap + sizeof(Header)); \
            if (tmp) { \
                header = (Header*)tmp; \
                header->capacity = new_cap; \
                arr = (void*)(header + 1); \
            } \
            arr = (void*)(header + 1); \
        } \
        (arr)[header->count++] = (x); \
    } while (0)

#define arr_len(arr) ((arr) ? ((Header*)(arr) - 1)->count : 0)

#define arr_free(arr) \
    do { \
        if (arr) { \
            free((Header*)(arr) - 1); \
            arr = NULL; \
        } \
    } while (0)

#endif
