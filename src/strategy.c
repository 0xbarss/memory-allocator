#include "strategy.h"
#include <stdio.h>

alloc_strategy_t current_strategy = STRATEGY_FIRST_FIT;

block_header_t *bins[NUM_BINS] = {NULL};

int get_bin_index(size_t size) {
    if (size <= 32) return 0;
    if (size <= 64) return 1;
    if (size <= 128) return 2;
    if (size <= 256) return 3;
    if (size <= 512) return 4;
    if (size <= 1024) return 5;
    if (size <= 2048) return 6;
    return 7;
}

void insert_into_bin(block_header_t *header) {
    if (header == NULL) return;
    int index = get_bin_index(header->size);

    header->next = bins[index];
    bins[index] = header;
}

void remove_from_bin(block_header_t *header) {
    if (header == NULL) return;
    int index = get_bin_index(header->size);

    if (header == bins[index]) {
        bins[index] = header->next;
        header->next = NULL;
        return;
    }

    block_header_t *curr = bins[index];
    while (curr != NULL && curr->next != header) {
        curr = curr->next;
    }

    if (curr != NULL) {
        curr->next = header->next;
    }
    header->next = NULL;
}

void set_allocation_strategy(alloc_strategy_t strategy) {
    current_strategy = strategy;
}

block_header_t *find_block_first_fit(block_header_t *head, size_t size) {
    block_header_t *curr = head;
    while (curr != NULL) {
        if (curr->is_free && curr->size >= size) return curr;
        curr = curr->next;
    }
    return NULL;
}

block_header_t *find_block_best_fit(block_header_t *head, size_t size) {
    block_header_t *curr = head;
    block_header_t *best = NULL;
    while (curr != NULL) {
        if (curr->is_free && curr->size >= size) {
            if (best == NULL) best = curr;
            else if (curr->size < best->size) best = curr;
        }
        curr = curr->next;
    }
    return best;
}

block_header_t *find_block_segregated(block_header_t *head, size_t size) {
    (void)head; // Unused
    int start_bin = get_bin_index(size);
    block_header_t *curr;
    for (int i=start_bin; i<=NUM_BINS-1; i++) {
        curr = bins[i];
        // In the starting bin
        if (i == start_bin) {
            while (curr != NULL) {
                if (curr->size >= size) {
                    remove_from_bin(curr);
                    return curr;
                }
                curr = curr->next;
            }
        }

        // In Higher Bins
        else if (curr != NULL) {
            remove_from_bin(curr);
            return curr;
        }
    }
    return NULL;
}
