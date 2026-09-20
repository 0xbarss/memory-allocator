#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "../src/heap.h"
#include "../src/allocator.h"

#define UNUSED(x) ((void)(x))

void test_contiguity(void) {
    printf("  [RUN] test_contiguity\n");

    void *p1 = bump_alloc(32);
    void *p2 = bump_alloc(64);
    void *p3 = bump_alloc(128);

    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);

    // Contiguous layout check
    assert((char *)p2 == (char *)p1 + 32);
    assert((char *)p3 == (char *)p2 + 64);
    assert(p1 < p2 && p2 < p3);

    printf("  [PASS] test_contiguity\n");
}

void test_memory_safety_no_overlap(void) {
    printf("  [RUN] test_memory_safety_no_overlap\n");

    size_t size_a = 32, size_b = 64, size_c = 128;

    unsigned char *a = (unsigned char *)bump_alloc(size_a);
    unsigned char *b = (unsigned char *)bump_alloc(size_b);
    unsigned char *c = (unsigned char *)bump_alloc(size_c);

    assert(a != NULL && b != NULL && c != NULL);

    // 1. Fill with distinct patterns
    memset(a, 0xAA, size_a);
    memset(b, 0xBB, size_b);
    memset(c, 0xCC, size_c);

    // 2. Verify all bytes
    for (size_t i = 0; i < size_a; i++) assert(a[i] == 0xAA);
    for (size_t i = 0; i < size_b; i++) assert(b[i] == 0xBB);
    for (size_t i = 0; i < size_c; i++) assert(c[i] == 0xCC);

    // 3. Mutate only block B
    memset(b, 0x55, size_b);

    // 4. Verify A and C are completely untouched
    for (size_t i = 0; i < size_a; i++) assert(a[i] == 0xAA);
    for (size_t i = 0; i < size_b; i++) assert(b[i] == 0x55);
    for (size_t i = 0; i < size_c; i++) assert(c[i] == 0xCC);

    printf("  [PASS] test_memory_safety_no_overlap\n");
}

void test_chunk_refill(void) {
    printf("  [RUN] test_chunk_refill\n");

    // 5 * 1024 = 5120 bytes (exceeds default 4096-byte chunk)
    unsigned char *blocks[5];
    for (int i = 0; i < 5; i++) {
        blocks[i] = (unsigned char *)bump_alloc(1024);
        assert(blocks[i] != NULL);
        memset(blocks[i], (unsigned char)(i + 1), 1024);
    }

    for (int i = 0; i < 5; i++) {
        unsigned char expected = (unsigned char)(i + 1);
        for (size_t j = 0; j < 1024; j++) {
            assert(blocks[i][j] == expected);
        }
    }

    printf("  [PASS] test_chunk_refill\n");
}

void test_large_allocation(void) {
    printf("  [RUN] test_large_allocation\n");

    size_t large_size = 8192;
    unsigned char *ptr = (unsigned char *)bump_alloc(large_size);
    assert(ptr != NULL);

    memset(ptr, 0xEE, large_size);
    for (size_t i = 0; i < large_size; i++) {
        assert(ptr[i] == 0xEE);
    }

    printf("  [PASS] test_large_allocation\n");
}

void test_header_payload_math(void) {
    printf("  [RUN] test_header_payload_math\n");
    char fake_block[sizeof(block_header_t) + 64];
    block_header_t *orig_header = (block_header_t *)fake_block;
    orig_header->size = 64;
    orig_header->is_free = 0;

    void *payload = payload_of(orig_header);
    assert((char *)payload == fake_block + sizeof(block_header_t));

    block_header_t *recovered = header_of(payload);
    assert(recovered == orig_header);
    assert(recovered->size == 64);
    printf("  [PASS] test_header_payload_math\n");
}

void test_byte_alignment(void) {
    printf("  [RUN] test_byte_alignment\n");
    void *ptr1 = my_malloc(1);
    void *ptr2 = my_malloc(7);
    void *ptr3 = my_malloc(9);
    void *ptr4 = my_malloc(35);
    void *ptr5 = my_malloc(101);

    assert(((size_t)ptr1 & (ALIGNMENT-1)) == 0);
    assert(((size_t)ptr2 & (ALIGNMENT-1)) == 0);
    assert(((size_t)ptr3 & (ALIGNMENT-1)) == 0);
    assert(((size_t)ptr4 & (ALIGNMENT-1)) == 0);
    assert(((size_t)ptr5 & (ALIGNMENT-1)) == 0);
    printf("  [PASS] test_byte_alignment\n");
}

void test_memory_reuse(void) {
    printf("  [RUN] test_memory_reuse\n");
    void *p1 = my_malloc(64);
    void *p2 = my_malloc(64);
    void *p3 = my_malloc(64);

    // Suppress unused warnings
    UNUSED(p1);
    UNUSED(p3);

    // Free the middle block
    my_free(p2);

    // Record the current program break
    void *break_before = sbrk(0);

    // Allocate 64 bytes again - MUST reuse p2!
    void *p4 = my_malloc(64);
    assert(p4 == p2);

    // Verify program break did not advance
    void *break_after = sbrk(0);
    assert(break_before == break_after);
    printf("  [PASS] test_memory_reuse\n");
}

void test_splitting(void) {
    printf("  [RUN] test_splitting\n");
    // Allocate a large 1024-byte block, then free it
    void *p = my_malloc(1024);
    my_free(p);

    // Request 64 bytes - should split the 1024-byte block
    void *small1 = my_malloc(64);
    assert(small1 == p);

    // Request another 64 bytes - should get the carved remainder
    void *small2 = my_malloc(64);
    assert(small2 > small1);
    assert((char *)small2 < (char *)small1 + 1024);
    printf("  [PASS] test_splitting\n");
}

void test_coalescing(void) {
    printf("  [RUN] test_coalescing\n");
    void *a = my_malloc(128);
    void *b = my_malloc(128);
    void *c = my_malloc(128);

    // Free B, then free A -> should coalesce into 1 block of ~256+ bytes
    my_free(b);
    my_free(a);

    // Free C -> should coalesce A, B, and C into 1 block of ~384+ bytes
    my_free(c);

    // Allocate 384 bytes -> must fit in the merged block without heap growth
    void *break_before = sbrk(0);
    void *big = my_malloc(384);
    assert(big == a);
    assert(sbrk(0) == break_before);
    printf("  [PASS] test_coalescing\n");
}

void test_calloc_zero_init(void) {
    printf("  [RUN] test_calloc_zero_init\n");
    size_t count = 64;
    size_t elem_size = 4;
    unsigned char *ptr = (unsigned char *)my_calloc(count, elem_size);
    assert(ptr != NULL);
    assert(((size_t)ptr & (ALIGNMENT - 1)) == 0);

    // Verify all bytes are 0
    for (size_t i = 0; i < count * elem_size; i++) {
        assert(ptr[i] == 0);
    }

    my_free(ptr);
    printf("  [PASS] test_calloc_zero_init\n");
}

void test_calloc_overflow(void) {
    printf("  [RUN] test_calloc_overflow\n");
    // SIZE_MAX * 2 should overflow
    void *p1 = my_calloc(SIZE_MAX, 2);
    assert(p1 == NULL);

    // Another overflow case
    void *p2 = my_calloc(SIZE_MAX / 2 + 1, 3);
    assert(p2 == NULL);

    // Zero element or size should return NULL
    void *p3 = my_calloc(0, 10);
    assert(p3 == NULL);
    void *p4 = my_calloc(10, 0);
    assert(p4 == NULL);

    printf("  [PASS] test_calloc_overflow\n");
}

void test_realloc_edge_cases(void) {
    printf("  [RUN] test_realloc_edge_cases\n");

    // ptr == NULL acts like malloc
    void *p = my_realloc(NULL, 64);
    assert(p != NULL);
    assert(((size_t)p & (ALIGNMENT - 1)) == 0);

    // new_size == 0 acts like free
    void *p_null = my_realloc(p, 0);
    assert(p_null == NULL);

    // Verify p was actually freed by checking that a 64-byte allocation reuses it
    void *reused = my_malloc(64);
    assert(reused == p);
    my_free(reused);

    printf("  [PASS] test_realloc_edge_cases\n");
}

void test_realloc_shrink(void) {
    printf("  [RUN] test_realloc_shrink\n");

    size_t orig_size = 256;
    size_t new_size = 64;
    unsigned char *p = (unsigned char *)my_malloc(orig_size);
    assert(p != NULL);

    // Fill with pattern
    memset(p, 0xAB, orig_size);

    // Shrink
    unsigned char *shrunk = (unsigned char *)my_realloc(p, new_size);
    // Should shrink in-place (return the same pointer)
    assert(shrunk == p);

    // First new_size bytes must be preserved
    for (size_t i = 0; i < new_size; i++) {
        assert(shrunk[i] == 0xAB);
    }

    // Remainder should have been split and available for reuse
    void *remainder = my_malloc(64);
    assert(remainder != NULL);
    assert(remainder > (void *)shrunk);

    my_free(shrunk);
    my_free(remainder);

    printf("  [PASS] test_realloc_shrink\n");
}

void test_realloc_grow_in_place(void) {
    printf("  [RUN] test_realloc_grow_in_place\n");

    unsigned char *a = (unsigned char *)my_malloc(64);
    unsigned char *b = (unsigned char *)my_malloc(64);
    assert(a != NULL && b != NULL);

    memset(a, 0x42, 64);

    // Free b so it is free immediately after a
    my_free(b);

    // Realloc a to 128 bytes - should absorb b in place
    unsigned char *grown = (unsigned char *)my_realloc(a, 128);
    assert(grown == a);

    // Check original 64 bytes preserved
    for (size_t i = 0; i < 64; i++) {
        assert(grown[i] == 0x42);
    }

    my_free(grown);
    printf("  [PASS] test_realloc_grow_in_place\n");
}

void test_realloc_fallback_move(void) {
    printf("  [RUN] test_realloc_fallback_move\n");

    unsigned char *a = (unsigned char *)my_malloc(64);
    unsigned char *b = (unsigned char *)my_malloc(64);
    assert(a != NULL && b != NULL);

    memset(a, 0x77, 64);
    memset(b, 0x88, 64);

    // b is still allocated, so a cannot grow in-place. Must move!
    unsigned char *moved = (unsigned char *)my_realloc(a, 256);
    assert(moved != NULL);
    assert(moved != a);

    // Data in moved must match original a
    for (size_t i = 0; i < 64; i++) {
        assert(moved[i] == 0x77);
    }

    // b must be untouched
    for (size_t i = 0; i < 64; i++) {
        assert(b[i] == 0x88);
    }

    // Old block a must be freed, so a 64-byte allocation should reuse it
    void *reused = my_malloc(64);
    assert(reused == a);

    my_free(moved);
    my_free(b);
    my_free(reused);

    printf("  [PASS] test_realloc_fallback_move\n");
}

int main(void) {
    printf("========================================\n");
    printf("        Running Unit Tests              \n");
    printf("========================================\n");

    test_contiguity();
    test_memory_safety_no_overlap();
    test_chunk_refill();
    test_large_allocation();
    test_header_payload_math();
    test_byte_alignment();
    test_memory_reuse();
    test_splitting();
    test_coalescing();
    test_calloc_zero_init();
    test_calloc_overflow();
    test_realloc_edge_cases();
    test_realloc_shrink();
    test_realloc_grow_in_place();
    test_realloc_fallback_move();

    printf("========================================\n");
    printf("        ALL TESTS PASSED!               \n");
    printf("========================================\n");
    return 0;
}
