#include "allocator.h"

#define NUM_BINS 8

typedef enum {
    STRATEGY_FIRST_FIT,
    STRATEGY_BEST_FIT,
    STRATEGY_SEGREGATED
} alloc_strategy_t;

extern alloc_strategy_t current_strategy;

int get_bin_index(size_t size);
void insert_into_bin(block_header_t *header);
void remove_from_bin(block_header_t *header);

void set_allocation_strategy(alloc_strategy_t strategy);
block_header_t *find_block_first_fit(block_header_t *head, size_t size);
block_header_t *find_block_best_fit(block_header_t *head, size_t size);
block_header_t *find_block_segregated(block_header_t *head, size_t size);
