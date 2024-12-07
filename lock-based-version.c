#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#define LARGE_CHUNK_SIZE (64 * 1024 * 1024) // 64 MB

//Definition of the chunk structure
struct chunk {
    size_t size; // The size of the memory block respresnted by the chunk 
    union {
        struct chunk* next; // Used when the chunk is free
        char content[0];    // used when the chunk is allocated
    };
};

// Global variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Global mutex lock
struct chunk* free_list = NULL;                    // Global free list

// Function to add a chunk back to the free list
void free_chunk(struct chunk* c) {
    pthread_mutex_lock(&mutex);  // Acquire the lock for thread safety
    c->next = free_list;         // Pointing c->next to the current head of free_list
    free_list = c;               // Making c the new head of free_list
    pthread_mutex_unlock(&mutex); // Release the lock
}

// Function to create a large chunk of (64 MB)
struct chunk* create_large_chunk() {
    size_t total_size = sizeof(struct chunk) + LARGE_CHUNK_SIZE;
    struct chunk* c = (struct chunk*)malloc(total_size); // Allocate memory

    if (!c) {
        // Allocation failure
        return NULL;
    }

    c->size = LARGE_CHUNK_SIZE; // Initialize the size of the chunk
    return c; // Return the new chunk
}

// Function to allocate a chunk of at least 'size' bytes
struct chunk* alloc_chunk(size_t size) {
    pthread_mutex_lock(&mutex); // Acquire the lock

    struct chunk** prev = &free_list;
    struct chunk* c = free_list;

    // 1: Find the first suitable chunk in the free_list
    while (c) {
        if (c->size >= size) {
            *prev = c->next; // Removes c from free_list
            break;
        }
        prev = &c->next;
        c = c->next;
    }

    if (!c) { // No suitable chunk found
        pthread_mutex_unlock(&mutex); // Release the lock before allocating
        c = create_large_chunk();     // Create a new large chunk
        if (!c) {
            // Allocation failure
            return NULL;
        }
        // Add the new chunk to free_list
        pthread_mutex_lock(&mutex);   // re-acquire the lock
        c->next = free_list; // c is the new head of free_list
        free_list = c;
        pthread_mutex_unlock(&mutex); // release the lock
        // re-try allocation
        return alloc_chunk(size);
    }

    //  2: Deciding if we need to split the chunk
    if (size + sizeof(struct chunk) < c->size) {
        // if the chunk is larger than needed then split the chunk
        struct chunk* d = (struct chunk*)((uintptr_t)c + sizeof(struct chunk) + size);
        d->size = c->size - size - sizeof(struct chunk); // Remaining size
        c->size = size; // Adjust the size of the allocated chunk
        // Free the remaining chunk
        d->next = free_list;
        free_list = d;
    }

    pthread_mutex_unlock(&mutex); // Release the lock
    return c; // Return the allocated chunk
}

// Example usage
int main() {
    // Allocate a chunk of 1024 bytes
    struct chunk* c1 = alloc_chunk(1024);
    if (c1) {
        printf("Allocated chunk at %p with size %zu bytes\n", (void*)c1, c1->size);
    } else {
        printf("Allocation failed\n");
    }

    // Use the allocated chunk
    // Remember that the usable memory starts after the struct chunk
    char* data = c1->content;
    snprintf(data, c1->size, "Hello, World!");
    printf("Data in chunk: %s\n", data);

    // Free the chunk
    free_chunk(c1);

    // Allocate another chunk
    struct chunk* c2 = alloc_chunk(2048);
    if (c2) {
        printf("Allocated chunk at %p with size %zu bytes\n", (void*)c2, c2->size);
    } else {
        printf("Allocation failed\n");
    }

    // Clean up: Free the chunk
    free_chunk(c2);

    // Note: In a real application, you might need to free any remaining chunks
    // and release resources properly.

    return 0;
}
