#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/heap.h"

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

int main(void) {
    printf("========================================\n");
    printf("        Running Unit Tests              \n");
    printf("========================================\n");

    test_contiguity();
    test_memory_safety_no_overlap();
    test_chunk_refill();
    test_large_allocation();

    printf("========================================\n");
    printf("        ALL TESTS PASSED!               \n");
    printf("========================================\n");
    return 0;
}
