#include <stdio.h>
#include <unistd.h>

#define CHUNK_SIZE 4096

static void *heap_start = NULL;
static void *heap_end = NULL;
static void *bump_ptr = NULL;

static inline void *incr_ptr(void *ptr, size_t size) {
    return (void *)((char *)ptr + size);
}

void *heap_extend(size_t bytes) {
    void *old_break = sbrk(bytes);
    if (old_break == (void *)-1) {
        return NULL;
    }
    if (heap_start == NULL) {
        heap_start = old_break;
    }
    heap_end = incr_ptr(old_break, bytes);
    return old_break;
}

void *bump_alloc(size_t size) {
    if (bump_ptr == NULL || incr_ptr(bump_ptr, size) > heap_end) {
        // To prevent subsequent system calls,
        // requests memory from the kernel in chunks
        // when bump_ptr runs out space or is initialized.
        size_t res = (size > CHUNK_SIZE) ? size: CHUNK_SIZE;
        void *mem = heap_extend(res);
        if (mem == NULL) return NULL;
        if (bump_ptr == NULL) bump_ptr = mem;
    }
    void *curr = bump_ptr;
    bump_ptr = incr_ptr(bump_ptr, size);
    return curr;
}
