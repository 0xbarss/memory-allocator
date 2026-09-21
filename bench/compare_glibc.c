#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "strategy.h"

typedef void *(*alloc_fn_t)(size_t);
typedef void  (*free_fn_t)(void *);

typedef struct {
    double time_ms;
    double heap_mb;
} bench_result_t;

typedef enum {
    ALLOC_GLIBC,
    ALLOC_FIRST_FIT,
    ALLOC_BEST_FIT,
    ALLOC_SEGREGATED
} allocator_type_t;

// ---------------------------------------------------------------------------
// Workload Definitions
// ---------------------------------------------------------------------------

// Workload A: Uniform Small Objects (5,000 allocations of 16-64 bytes)
static void workload_small_objects(alloc_fn_t my_malloc_fn, free_fn_t my_free_fn) {
    const int count = 5000;
    void **ptrs = malloc(count * sizeof(void *));
    if (!ptrs) return;

    for (int i = 0; i < count; i++) {
        size_t sz = 16 + (i % 4) * 16; // 16, 32, 48, 64 bytes
        ptrs[i] = my_malloc_fn(sz);
    }
    for (int i = 0; i < count; i++) {
        if (ptrs[i]) my_free_fn(ptrs[i]);
    }
    free(ptrs);
}

// Workload B: Large Buffer Churn (1,000 allocations of 8 KB - 64 KB)
static void workload_large_buffer_churn(alloc_fn_t my_malloc_fn, free_fn_t my_free_fn) {
    const int count = 1000;
    void **ptrs = malloc(count * sizeof(void *));
    if (!ptrs) return;

    for (int i = 0; i < count; i++) {
        size_t sz = 8192 + (i % 8) * 8192; // 8 KB to 64 KB
        ptrs[i] = my_malloc_fn(sz);
        if (i % 3 == 0 && ptrs[i]) {
            my_free_fn(ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    for (int i = 0; i < count; i++) {
        if (ptrs[i]) my_free_fn(ptrs[i]);
    }
    free(ptrs);
}

// Workload C: Random Life-Cycle (10,000 random alloc / free operations)
static void workload_random_lifecycle(alloc_fn_t my_malloc_fn, free_fn_t my_free_fn) {
    const int ops = 10000;
    const int max_live = 1000;
    void **slots = calloc(max_live, sizeof(void *));
    if (!slots) return;

    unsigned int seed = 42;
    for (int i = 0; i < ops; i++) {
        int idx = rand_r(&seed) % max_live;
        if (slots[idx] == NULL) {
            size_t sz = 16 + (rand_r(&seed) % 512);
            slots[idx] = my_malloc_fn(sz);
        } else {
            my_free_fn(slots[idx]);
            slots[idx] = NULL;
        }
    }
    for (int i = 0; i < max_live; i++) {
        if (slots[i]) my_free_fn(slots[i]);
    }
    free(slots);
}

// Workload D: Fragmentation Torture (Allocate 4,000, free alternating, allocate varied)
static void workload_fragmentation_torture(alloc_fn_t my_malloc_fn, free_fn_t my_free_fn) {
    const int count = 4000;
    void **ptrs = malloc(count * sizeof(void *));
    if (!ptrs) return;

    // Phase 1: Allocate contiguous blocks
    for (int i = 0; i < count; i++) {
        ptrs[i] = my_malloc_fn(64);
    }

    // Phase 2: Create holes by freeing alternating blocks
    for (int i = 0; i < count; i += 2) {
        if (ptrs[i]) {
            my_free_fn(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    // Phase 3: Allocate varied sizes to test search policies on fragmented free list
    const int new_count = 2000;
    void **new_ptrs = malloc(new_count * sizeof(void *));
    if (new_ptrs) {
        for (int i = 0; i < new_count; i++) {
            size_t sz = 32 + (i % 7) * 16; // 32 to 128 bytes
            new_ptrs[i] = my_malloc_fn(sz);
        }
        for (int i = 0; i < new_count; i++) {
            if (new_ptrs[i]) my_free_fn(new_ptrs[i]);
        }
        free(new_ptrs);
    }

    // Cleanup remaining blocks
    for (int i = 0; i < count; i++) {
        if (ptrs[i]) my_free_fn(ptrs[i]);
    }
    free(ptrs);
}

// ---------------------------------------------------------------------------
// Isolated Benchmark Runner (using fork)
// ---------------------------------------------------------------------------
typedef void (*workload_fn_t)(alloc_fn_t, free_fn_t);

static bench_result_t run_isolated(workload_fn_t workload, allocator_type_t alloc_type) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Child process: completely isolated heap
        close(pipe_fd[0]);

        alloc_fn_t afn;
        free_fn_t ffn;

        if (alloc_type == ALLOC_GLIBC) {
            afn = malloc;
            ffn = free;
        } else {
            afn = my_malloc;
            ffn = my_free;
            if (alloc_type == ALLOC_FIRST_FIT) {
                set_allocation_strategy(STRATEGY_FIRST_FIT);
            } else if (alloc_type == ALLOC_BEST_FIT) {
                set_allocation_strategy(STRATEGY_BEST_FIT);
            } else if (alloc_type == ALLOC_SEGREGATED) {
                set_allocation_strategy(STRATEGY_SEGREGATED);
            }
        }

        void *break_before = sbrk(0);
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        workload(afn, ffn);

        clock_gettime(CLOCK_MONOTONIC, &end);
        void *break_after = sbrk(0);

        double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                            (end.tv_nsec - start.tv_nsec) / 1000000.0;

        ptrdiff_t heap_diff = (char *)break_after - (char *)break_before;
        double heap_mb = heap_diff > 0 ? (double)heap_diff / (1024.0 * 1024.0) : 0.0;

        bench_result_t result = { .time_ms = elapsed_ms, .heap_mb = heap_mb };
        ssize_t written = write(pipe_fd[1], &result, sizeof(result));
        (void)written;
        close(pipe_fd[1]);
        _exit(0);
    }

    // Parent process
    close(pipe_fd[1]);
    bench_result_t result = {0};
    ssize_t read_bytes = read(pipe_fd[0], &result, sizeof(result));
    (void)read_bytes;
    close(pipe_fd[0]);
    waitpid(pid, NULL, 0);

    return result;
}

// ---------------------------------------------------------------------------
// Main Benchmark Driver
// ---------------------------------------------------------------------------
int main(void) {
    printf("\n==================================================================================================\n");
    printf("                  Phase 5 Benchmark: Allocation Strategy Comparison vs glibc                      \n");
    printf("==================================================================================================\n");

    const char *workload_names[] = {
        "Small Objects",
        "Large Buffer Churn",
        "Random Life-Cycle",
        "Fragmentation Torture"
    };

    workload_fn_t workloads[] = {
        workload_small_objects,
        workload_large_buffer_churn,
        workload_random_lifecycle,
        workload_fragmentation_torture
    };

    bench_result_t results[4][4]; // [workload][allocator]

    for (int w = 0; w < 4; w++) {
        printf("Running %-22s ... ", workload_names[w]);
        fflush(stdout);
        for (int a = 0; a < 4; a++) {
            results[w][a] = run_isolated(workloads[w], (allocator_type_t)a);
        }
        printf("done.\n");
    }

    // Measure Peak Heap Growth under Fragmentation Torture
    double peak_heap[4];
    for (int a = 0; a < 4; a++) {
        peak_heap[a] = results[3][a].heap_mb;
    }

    // Print Formatted Table
    printf("\n");
    printf("%-22s | %-12s | %-23s | %-22s | %-21s\n",
           "Workload", "glibc (ms)", "forge First-Fit (ms)", "forge Best-Fit (ms)", "forge SegList (ms)");
    printf("-----------------------+--------------+-------------------------+------------------------+----------------------\n");

    for (int w = 0; w < 4; w++) {
        printf("%-22s | %9.2f ms | %20.2f ms | %19.2f ms | %18.2f ms\n",
               workload_names[w],
               results[w][0].time_ms,
               results[w][1].time_ms,
               results[w][2].time_ms,
               results[w][3].time_ms);
    }

    printf("-----------------------+--------------+-------------------------+------------------------+----------------------\n");
    printf("%-22s | %9.2f MB | %20.2f MB | %19.2f MB | %18.2f MB\n",
           "Heap Growth (Torture)",
           peak_heap[0],
           peak_heap[1],
           peak_heap[2],
           peak_heap[3]);
    printf("==================================================================================================\n\n");

    return 0;
}
