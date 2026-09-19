#include <stdio.h>
#include <unistd.h>
#include "allocator.h"
#include "heap.h"

#define MIN_PAYLOAD 16
#define MIN_SPLIT_SIZE ((BLOCK_HEADER_SIZE) + (BLOCK_FOOTER_SIZE) + (MIN_PAYLOAD))

static block_header_t *heap_blocks_head = NULL;

void set_footer(block_header_t *ptr) {
    if (ptr == NULL) return;
    block_footer_t *footer = footer_of(ptr);
    footer->size = ptr->size;
    footer->is_free = ptr->is_free;
}

static void split_block(block_header_t *block, size_t requested_size) {
    size_t total_available = block->size;

    if (total_available >= requested_size + MIN_SPLIT_SIZE) {
        size_t remainder_size = total_available - requested_size - BLOCK_HEADER_SIZE - BLOCK_FOOTER_SIZE;

        block->size = requested_size;
        set_footer(block);

        block_header_t *remainder = (block_header_t *)((char *)block + BLOCK_HEADER_SIZE + requested_size + BLOCK_FOOTER_SIZE);
        remainder->size = remainder_size;
        remainder->is_free = 1;
        remainder->next = block->next;
        block->next = remainder;
        set_footer(remainder);
    }
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;
    size_t aligned_size = ALIGN(size);
    block_header_t *curr = heap_blocks_head;
    block_header_t *last = NULL;

    // Search existing blocks (first-fit)
    while (curr != NULL) {
        if (curr->is_free == 1 && curr->size >= aligned_size) {
            curr->is_free = 0;
            split_block(curr, aligned_size);
            set_footer(curr);
            return payload_of(curr);
        }
        last = curr;
        curr = curr->next;
    }

    // No Block Found -> Extend Heap
    size_t total_needed = BLOCK_HEADER_SIZE + aligned_size + BLOCK_FOOTER_SIZE;
    block_header_t *new_block = (block_header_t *)heap_extend(total_needed);

    if (new_block == NULL) return NULL;
    new_block->size = aligned_size;
    new_block->is_free = 0;
    new_block->next = NULL;

    if (heap_blocks_head == NULL) heap_blocks_head = new_block;
    else last->next = new_block;
    set_footer(new_block);
    return payload_of(new_block);
}

void coalesce(block_header_t *header) {
    // Previous Block
    block_header_t *prev = NULL;
    int is_prev_free = 0;
    if (header != heap_blocks_head) {
        block_footer_t *prev_footer = (block_footer_t *)((char *)header - BLOCK_FOOTER_SIZE);
        prev = (block_header_t *)((char *)prev_footer - prev_footer->size - BLOCK_HEADER_SIZE);
        is_prev_free = prev->is_free;
    }

    // Next Block
    block_header_t *next = (block_header_t *)((char *)header + BLOCK_HEADER_SIZE + header->size + BLOCK_FOOTER_SIZE);
    int is_next_free = 0;
    if ((void *)next < sbrk(0)) {
        is_next_free = next->is_free;
    }

    // Prev allocated, next free
    if (!is_prev_free && is_next_free) {
        header->size += BLOCK_HEADER_SIZE + next->size + BLOCK_FOOTER_SIZE;
        header->is_free = 1;
        header->next = next->next;
        set_footer(header);
    }
    // Prev free, next allocated
    else if (is_prev_free && !is_next_free) {
        prev->size += BLOCK_HEADER_SIZE + header->size + BLOCK_FOOTER_SIZE;
        prev->next = header->next;
        set_footer(prev);
    }
    // Both free
    else if (is_prev_free && is_next_free) {
        prev->size += (2*BLOCK_HEADER_SIZE + header->size + next->size + 2*BLOCK_FOOTER_SIZE);
        prev->next = next->next;
        set_footer(prev);
    }
    // Both allocated
    else {
        header->is_free = 1;
        set_footer(header);
    }
}

void my_free(void *ptr) {
    if (ptr == NULL) return;
    coalesce(header_of(ptr));
}
