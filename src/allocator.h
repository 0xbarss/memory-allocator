#include <unistd.h>

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define BLOCK_HEADER_SIZE sizeof(block_header_t)

/*
 * Byte Offset:  0        7 8    11 12   15 16               23 24               31
 *              ┌──────────┬───────┬───────┬───────────────────┬───────────────────┐
 * Field:       │   size   │is_free│padding│       next        │     padding2      │
 * Type:        │  size_t  │  int  │  int  │struct block_header│       void*       │
 * Bytes:       │  8 bytes │4 bytes│4 bytes│      8 bytes      │      8 bytes      │
 *              └──────────┴───────┴───────┴───────────────────┴───────────────────┘
*/
typedef struct block_header {
    size_t size;                // 8 bytes  : Size of payload
    int is_free;                // 4 bytes  : 0->allocated, 1->free
    int padding;                // 4 bytes  : padding to align next pointer
    struct block_header *next;  // 8 bytes  : next block in the linked list
    void *padding2;             // 8 bytes  : padding for struct size to 32 bytes
} block_header_t;               // 32 bytes : Total Size

static inline void *payload_of(block_header_t *header) {
    if (!header) return NULL;
    return (void *)((char *)header + BLOCK_HEADER_SIZE);
}

static inline block_header_t *header_of(void *ptr) {
    if (!ptr) return NULL;
    return (block_header_t *)((char *)ptr - BLOCK_HEADER_SIZE);
}

void *my_malloc(size_t size);
void my_free(void *ptr);
