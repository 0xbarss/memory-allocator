#include "heap.h"
#include <stdio.h>

int main(void) {
    void *p1 = bump_alloc(32);
    void *p2 = bump_alloc(64);
    void *p3 = bump_alloc(128);
    printf("p1: %p\n", p1);
    printf("p2: %p\n", p2);
    printf("p3: %p\n", p3);
    printf("p2 == (char *)p1 + 32  ->  %d\n", p2 == (char *)p1 + 32);
    printf("p2 == (char *)p1 + 32  ->  %d\n", p3 == (char *)p2 + 64);
    return 0;
}
