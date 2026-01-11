#ifndef SLAB_H
#define SLAB_H

#include <inttypes.h>
#include <stddef.h>

void *kmalloc_slab(size_t size);
void kfree_slab(void *ptr);

void init_slab_cache(void);

#endif
