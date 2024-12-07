// lock-allocator-free.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include "allocator.h"

#define LARGE_CHUNK_SIZE (64 * 1024 * 1024) // 64 MB

// Define a custom structure for the lock-free allocator
struct chunk_lock_free {
    size_t size;
    union {
        _Atomic(struct chunk_lock_free*) next; // Atomic next pointer
        char content[0];
    };
};

// Global atomic free list for the lock-free allocator
static _Atomic(struct chunk_lock_free*) free_list = NULL;

// Function to create a large chunk
static struct chunk_lock_free* create_large_chunk() {
    size_t total_size = sizeof(struct chunk_lock_free) + LARGE_CHUNK_SIZE;
    struct chunk_lock_free* c = (struct chunk_lock_free*)malloc(total_size);
    if (!c) {
        return NULL;
    }
    c->size = LARGE_CHUNK_SIZE;
    return c;
}

// Atomic pop operation
static struct chunk_lock_free* pop() {
    struct chunk_lock_free* head;

    while (1) {
        head = atomic_load_explicit(&free_list, memory_order_acquire);

        if (head == NULL) {
            return NULL; // The free list is empty
        }

        struct chunk_lock_free* next = atomic_load_explicit(&head->next, memory_order_relaxed);

        if (atomic_compare_exchange_weak_explicit(
                &free_list,
                &head,
                next,
                memory_order_acquire,
                memory_order_relaxed)) {
            // Successfully removed the head; return it
            return head;
        }
        // Retry if the operation failed
    }
}

// Atomic push operation
static void push(struct chunk_lock_free* c) {
    struct chunk_lock_free* old_head;
    do {
        old_head = atomic_load_explicit(&free_list, memory_order_relaxed);
        atomic_store_explicit(&c->next, old_head, memory_order_relaxed);
    } while (!atomic_compare_exchange_weak_explicit(
        &free_list,
        &old_head,
        c,
        memory_order_release,
        memory_order_relaxed));
}

// Function to free a chunk
void free_chunk_lock_free(struct chunk* c_generic) {
    struct chunk_lock_free* c = (struct chunk_lock_free*)c_generic;
    push(c);
}

// Function to allocate a chunk with splitting
struct chunk* alloc_chunk_lock_free(size_t size) {
    struct chunk_lock_free* c;

    while (1) {
        c = pop();
        if (!c) {
            // No chunks available, create a new large chunk
            c = create_large_chunk();
            if (!c) {
                // Allocation failed
                return NULL;
            }
            // Add the new chunk to free_list and retry
            free_chunk_lock_free((struct chunk*)c);
            continue;
        }

        if (c->size >= size) {
            // Decide whether to split the chunk
            if (size + sizeof(struct chunk_lock_free) < c->size) {
                // Split the chunk
                uintptr_t chunk_address = (uintptr_t)c;
                uintptr_t new_chunk_address = chunk_address + sizeof(struct chunk_lock_free) + size;

                // Ensure alignment
                new_chunk_address = (new_chunk_address + alignof(struct chunk_lock_free) - 1) & ~(alignof(struct chunk_lock_free) - 1);

                struct chunk_lock_free* d = (struct chunk_lock_free*)new_chunk_address;
                d->size = c->size - size - (new_chunk_address - chunk_address);
                c->size = size; // Adjust the size of the allocated chunk

                // Free the remaining chunk
                atomic_thread_fence(memory_order_release);
                free_chunk_lock_free((struct chunk*)d);
            }
            // Return the allocated chunk
            return (struct chunk*)c;
        } else {
            // Chunk too small, add it back to free_list
            free_chunk_lock_free((struct chunk*)c);
            // Loop again to try another chunk
        }
    }
}

