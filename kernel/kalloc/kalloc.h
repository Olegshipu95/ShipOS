#ifndef KALLOC_H
#define KALLOC_H

// #include "../lib/include/stdint.h"
#include <inttypes.h>
#include <stddef.h>

void kinit(uint64_t, uint64_t);
void *kalloc(void);
void kfree(void *);
void *kzalloc(size_t size);
uint64_t count_pages();

#endif
