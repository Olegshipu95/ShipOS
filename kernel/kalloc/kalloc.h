#ifndef KALLOC_H
#define KALLOC_H

//#include "../lib/include/stdint.h"
#include <inttypes.h>

void kinit(uint64_t, uint64_t);
void *kalloc(void);
void kfree(void*);
void *kmalloc(uint64_t);
void kmfree(void*);

#endif