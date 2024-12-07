// test_allocator.c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "allocator.h"

#define NUM_THREADS 4
#define NUM_ITERATIONS 10000


// Structure to pass allocator functions to threads
typedef struct {
    struct chunk* (*alloc_func)(size_t size);
    void (*free_func)(struct chunk* c);
    const char* allocator_name;
} allocator_info_t;



void* thread_function(void* arg) {
    allocator_info_t* allocator = (allocator_info_t*)arg;
    struct chunk* chunks[NUM_ITERATIONS];
    size_t total_allocated = 0;
    unsigned int seed = (unsigned int)(uintptr_t)pthread_self(); // Unique seed per thread

    // Allocation phase
    
    // Note : The standard rand() function is not thread-safe. When used in a multithreaded environment without proper synchronization, it can lead to unexpected behavior, including infinite loops or crashes. We will use rand_r() instead !
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        size_t size = rand_r(&seed) % 1024 + 1; // Random size between 1 and 1024
        chunks[i] = allocator->alloc_func(size);
        if (chunks[i]) {
            // memset(chunks[i]->content, 0xAA, size);
            total_allocated += size;
        } else {
            printf("Allocation failed at iteration %d\n", i);
            chunks[i] = NULL;
        }
    }

    // Deallocation phase
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        if (chunks[i]) {
            allocator->free_func(chunks[i]);
        }
    }

    printf("%s: Thread completed. Total allocated: %zu bytes\n",
           allocator->allocator_name, total_allocated);
    return NULL;
}

void run_test(struct chunk* (*alloc_func)(size_t),
              void (*free_func)(struct chunk*),
              const char* allocator_name) {
    pthread_t threads[NUM_THREADS];
    allocator_info_t allocator = {alloc_func, free_func, allocator_name};

    clock_t start_time = clock();

    // Start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_function, &allocator);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_t end_time = clock();
    double elapsed_secs = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("%s: Total time taken: %.2f seconds\n", allocator_name, elapsed_secs);
}

int main() {
    printf("Testing Lock-Based Allocator\n");
    run_test(alloc_chunk_lock_based, free_chunk_lock_based, "Lock-Based Allocator");

    printf("\nTesting Lock-Free Allocator\n");
    run_test(alloc_chunk_lock_free, free_chunk_lock_free, "Lock-Free Allocator");

    return 0;
}

