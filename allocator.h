// allocator.h

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <stdalign.h>

struct chunk {
    size_t size;
    union {
        struct chunk* next; // Used when the chunk is free
        char content[0];    // Used when the chunk is allocated
    };
};

struct chunk* alloc_chunk_lock_based(size_t size);
void free_chunk_lock_based(struct chunk* c);

struct chunk* alloc_chunk_lock_free(size_t size);
void free_chunk_lock_free(struct chunk* c);

#endif // ALLOCATOR_H

