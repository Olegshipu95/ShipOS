#include "slub.h"
#include "kalloc.h"
#include "../memlayout.h"
#include "../list/list.h"

#define SLAB_SIZES_COUNT 4
static const size_t slab_sizes[SLAB_SIZES_COUNT] = {8, 16, 32, 64};


struct slab_cache;

struct page {
    struct list list;
    void *freelist;
    unsigned int inuse;
    unsigned int objects;
    struct slab_cache *cache; 
};

struct slab_cache {
    size_t object_size;         
    struct list slabs_partial;  
    struct list slabs_full;     
    struct list slabs_empty;    
};

static struct slab_cache caches[SLAB_SIZES_COUNT];

static void slab_cache_init(struct slab_cache *cache, size_t size) {
    cache->object_size = size;
    lst_init(&cache->slabs_partial);
    lst_init(&cache->slabs_full);
    lst_init(&cache->slabs_empty);
}

void slabs_init_all() {
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        slab_cache_init(&caches[i], slab_sizes[i]);
    }
}

static struct slab_cache* get_cache(size_t size) {
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        if (size <= caches[i].object_size)
            return &caches[i];
    }
    return NULL;
}


static struct slab_cache* find_cache_for_slab(struct page *slab) {
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        struct slab_cache *cache = &caches[i];
        struct list *lst;

        for (lst = cache->slabs_full.next; lst != &cache->slabs_full; lst = lst->next)
            if ((struct page*)lst == slab) return cache;

        for (lst = cache->slabs_partial.next; lst != &cache->slabs_partial; lst = lst->next)
            if ((struct page*)lst == slab) return cache;

        for (lst = cache->slabs_empty.next; lst != &cache->slabs_empty; lst = lst->next)
            if ((struct page*)lst == slab) return cache;
    }
    return NULL;
}

static struct page* create_slab(struct slab_cache *cache) {
    struct page *slab = (struct page*)kalloc();
    if (!slab) return NULL;

    slab->inuse = 0;
    slab->objects = (PGSIZE - sizeof(struct page)) / cache->object_size;
    slab->cache = cache;
    slab->freelist = (void*)((uintptr_t)slab + sizeof(struct page));

    char *ptr = (char*)slab->freelist;
    for (unsigned int i = 0; i < slab->objects - 1; i++) {
        *(void**)ptr = ptr + cache->object_size;
        ptr += cache->object_size;
    }
    *(void**)ptr = NULL;

    lst_init(&slab->list);
    lst_push(&cache->slabs_partial, slab);
    return slab;
}


void* malloc_slub(size_t size) {
    struct slab_cache *cache = get_cache(size);
    if (!cache) return NULL;

    struct page *slab;

    if (!lst_empty(&cache->slabs_partial)) {
        slab = (struct page*)lst_pop(&cache->slabs_partial);
    } else if (!lst_empty(&cache->slabs_empty)) {
        slab = (struct page*)lst_pop(&cache->slabs_empty);
    } else {
        slab = create_slab(cache);
        if (!slab) return NULL;
    }

    void *obj = slab->freelist;
    slab->freelist = *(void**)obj;
    slab->inuse++;

    if (slab->inuse == slab->objects)
        lst_push(&cache->slabs_full, slab);
    else
        lst_push(&cache->slabs_partial, slab);

    return obj;
}



void free_slub(void *ptr) {
    struct page *slab = (struct page*)((uintptr_t)ptr & ~(PGSIZE - 1));
    struct slab_cache *cache = slab->cache;

    *(void**)ptr = slab->freelist;
    slab->freelist = ptr;
    slab->inuse--;

    lst_remove(&slab->list);
    if (slab->inuse == 0)
        lst_push(&cache->slabs_empty, slab);
    else
        lst_push(&cache->slabs_partial, slab);
}
