#ifndef SLAB_TEST_H
#define SLAB_TEST_H

#include <stddef.h>

size_t slab_get_cache_count(void);
size_t slab_get_cache_object_size(int idx);
size_t slab_get_cache_slabs_full_count(int idx);
size_t slab_get_cache_slabs_partial_count(int idx);
size_t slab_get_cache_slabs_empty_count(int idx);
size_t slab_get_cache_total_objects(int idx);

#endif