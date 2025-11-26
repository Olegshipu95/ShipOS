#ifndef KALLOC_H
#define KALLOC_H

// #include "../lib/include/stdint.h"
#include <inttypes.h>

void kinit(uint64_t, uint64_t);
void *kalloc(void);
void kfree(void *);
uint64_t count_pages();

#endif