#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "strategy.h"
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
        if (current_strategy == STRATEGY_SEGREGATED) {
            insert_into_bin(remainder);
        }
    }
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;
    size_t aligned_size = ALIGN(size);
    block_header_t *curr = heap_blocks_head;

    // Search existing blocks
    if (current_strategy == STRATEGY_FIRST_FIT) {
        curr = find_block_first_fit(curr, aligned_size);
    } else if (current_strategy == STRATEGY_BEST_FIT) {
        curr = find_block_best_fit(curr, aligned_size);
    } else {
        curr = find_block_segregated(curr, aligned_size);
    }

    if (curr != NULL) {
        curr->is_free = 0;
        split_block(curr, aligned_size);
        set_footer(curr);
        return payload_of(curr);
    }

    // No Block Found -> Extend Heap
    size_t total_needed = BLOCK_HEADER_SIZE + aligned_size + BLOCK_FOOTER_SIZE;
    block_header_t *new_block = (block_header_t *)heap_extend(total_needed);

    if (new_block == NULL) return NULL;
    new_block->size = aligned_size;
    new_block->is_free = 0;
    new_block->next = NULL;

    if (heap_blocks_head == NULL) heap_blocks_head = new_block;
    else if (current_strategy != STRATEGY_SEGREGATED) {
        block_header_t *last = heap_blocks_head;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_block;
    }
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
        if (current_strategy == STRATEGY_SEGREGATED) remove_from_bin(next);
        header->size += BLOCK_HEADER_SIZE + next->size + BLOCK_FOOTER_SIZE;
        header->is_free = 1;
        header->next = next->next;
        set_footer(header);
        if (current_strategy == STRATEGY_SEGREGATED) insert_into_bin(header);
    }
    // Prev free, next allocated
    else if (is_prev_free && !is_next_free) {
        if (current_strategy == STRATEGY_SEGREGATED) remove_from_bin(prev);
        prev->size += BLOCK_HEADER_SIZE + header->size + BLOCK_FOOTER_SIZE;
        prev->next = header->next;
        set_footer(prev);
        if (current_strategy == STRATEGY_SEGREGATED) insert_into_bin(prev);
    }
    // Both free
    else if (is_prev_free && is_next_free) {
        if (current_strategy == STRATEGY_SEGREGATED) {
            remove_from_bin(prev);
            remove_from_bin(next);
        }
        prev->size += (2*BLOCK_HEADER_SIZE + header->size + next->size + 2*BLOCK_FOOTER_SIZE);
        prev->next = next->next;
        set_footer(prev);
        if (current_strategy == STRATEGY_SEGREGATED) insert_into_bin(prev);
    }
    // Both allocated
    else {
        header->is_free = 1;
        set_footer(header);
        if (current_strategy == STRATEGY_SEGREGATED) insert_into_bin(header);
    }
}

void my_free(void *ptr) {
    if (ptr == NULL) return;
    coalesce(header_of(ptr));
}

void *my_calloc(size_t nmemb, size_t size) {
    // Check for multiplication overflow
    if (size != 0 && nmemb > SIZE_MAX / size) {
        return NULL;
    }
    size_t total_size = nmemb * size;
    void *ptr = my_malloc(total_size);
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void *my_realloc(void *ptr, size_t new_size) {
    if (ptr == NULL) return my_malloc(new_size);
    if (new_size == 0) {
        my_free(ptr);
        return NULL;
    }

    size_t aligned_size = ALIGN(new_size);
    block_header_t *header = header_of(ptr);
    if (aligned_size <= header->size) {
        split_block(header, aligned_size);
        return ptr;
    }

    block_header_t *next = header->next;
    if (next != NULL && next->is_free) {
        size_t combined_size = header->size + BLOCK_HEADER_SIZE + next->size + BLOCK_FOOTER_SIZE;
        if (aligned_size <= combined_size) {
            if (current_strategy == STRATEGY_SEGREGATED) remove_from_bin(next);
            header->size = combined_size;
            header->next = next->next;
            set_footer(header);
            split_block(header, aligned_size);
            return ptr;
        }
    }

    void *new_ptr = my_malloc(aligned_size);
    if (new_ptr == NULL) return NULL;
    memcpy(new_ptr, ptr, header->size);
    my_free(ptr);
    return new_ptr;
}
