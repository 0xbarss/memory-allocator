#include <unistd.h>
#include "allocator.h"
#include "heap.h"

static block_header_t *heap_blocks_head = NULL;

void *my_malloc(size_t size) {
    if (size == 0) return NULL;
    size_t aligned_size = ALIGN(size);
    block_header_t *curr = heap_blocks_head;
    block_header_t *last = NULL;

    // Search existing blocks (first-fit)
    while (curr != NULL) {
        if (curr->is_free == 1 && curr->size >= aligned_size) {
            curr->is_free = 0;
            return payload_of(curr);
        }
        last = curr;
        curr = curr->next;
    }

    // No Block Found -> Extend Heap
    size_t total_needed = BLOCK_HEADER_SIZE + aligned_size;
    block_header_t *new_block = (block_header_t *)heap_extend(total_needed);

    if (new_block == NULL) return NULL;
    new_block->size = aligned_size;
    new_block->is_free = 0;
    new_block->next = NULL;

    if (heap_blocks_head == NULL) heap_blocks_head = new_block;
    else last->next = new_block;
    return payload_of(new_block);
}

void my_free(void *ptr) {
    if (ptr == NULL) return;
    block_header_t *header = header_of(ptr);
    header->is_free = 1;
}
