#ifndef SLUB_H
#define SLUB_H

#include <stddef.h>

void* malloc_slub(size_t size);
void free_slub(void *ptr);

void slabs_init_all();

#endif