#ifndef SLOB_TEST_H
#define SLOB_TEST_H

#include <stddef.h>

size_t slob_get_page_count(void);
size_t slob_get_total_free_blocks(void);
size_t slob_get_total_allocated_blocks(void);
int slob_has_adjacent_free_blocks(void);

#endif