#include "slab.h"
#include "kalloc.h"
#include "../memlayout.h"
#include "../list/list.h"

#define SLAB_SIZES_COUNT 4
static size_t slab_sizes[SLAB_SIZES_COUNT] = {8, 16, 32, 64};

struct slab_obj {
    struct list list;
    char data[];
};

struct slab {
    struct list list;
    struct slab_obj *allocated_objects;
    struct slab_obj *free_objects;
};

struct slab_cache {
    unsigned int object_size;
    unsigned int num;

    struct list slabs_full;
    struct list slabs_partial;
    struct list slabs_empty;
};


static struct slab_cache caches[SLAB_SIZES_COUNT];

static void slab_obj_list_remove(struct slab_obj **head, struct slab_obj *obj) {
    if (!head || !*head || !obj) return;
    if (*head == obj) {
        *head = (struct slab_obj*)(*head)->list.next;
        return;
    }
    struct slab_obj *it = *head;
    while (it->list.next) {
        if ((struct slab_obj*)it->list.next == obj) {
            it->list.next = obj->list.next;
            return;
        }
        it = (struct slab_obj*)it->list.next;
    }
}


struct slab* alloc_slab(size_t obj_sz) {
    void *page = kalloc();
    if (!page) return NULL;

    struct slab *slab = (struct slab*)page;
    lst_init(&slab->list);
    slab->list.prev = NULL;

    slab->allocated_objects = NULL;
    slab->free_objects = NULL;

    char *p = (char*)page + sizeof(struct slab);
    char *end = (char*)page + PGSIZE;

    size_t sizeof_obj = sizeof(struct list) + obj_sz;

    struct slab_obj *prev = NULL;

    while (p + sizeof_obj <= end) {
        struct slab_obj *obj = (struct slab_obj*)p;
        lst_init(&obj->list);
        obj->list.prev = NULL;

        if (!slab->free_objects)
            slab->free_objects = obj;

        if (prev)
            lst_push((struct list*) prev, obj);

        prev = obj;

        p += sizeof_obj;
    }

    return slab;
}


void init_slab_cache() {
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {

        struct slab_cache *cache = &caches[i];
        cache->object_size = slab_sizes[i];

        lst_init(&cache->slabs_full);
        lst_init(&cache->slabs_partial);
        lst_init(&cache->slabs_empty);

        struct slab *s = alloc_slab(cache->object_size);
        lst_init(&s->list);

        lst_push(&cache->slabs_empty, &s->list);
    }
}

static struct slab_cache* get_cache(size_t size) {
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        if (size <= caches[i].object_size)
            return &caches[i];
    }
    return NULL;
}


void *kmalloc_slab(size_t size) {
    struct slab_cache *cache = get_cache(size);
    if (!cache) return NULL;

    struct slab *slab = NULL;
    struct list *node;

    for (;;) {
        if (!lst_empty(&cache->slabs_partial)) {
            node = (struct list*)lst_pop(&cache->slabs_partial);
            slab = (struct slab *)node;
        }
        else if (!lst_empty(&cache->slabs_empty)) {
            node = (struct list*)lst_pop(&cache->slabs_empty);
            slab = (struct slab *)node;
        }
        else {
            slab = alloc_slab(cache->object_size);
            if (!slab) return NULL;
            lst_init(&slab->list);
            lst_push(&cache->slabs_empty, &slab->list);

            node = (struct list*)lst_pop(&cache->slabs_empty);
            slab = (struct slab *)node;
        }

        if (slab->free_objects == NULL) {
            lst_push(&cache->slabs_full, &slab->list);
            slab = NULL;
            continue;
        }

        break;
    }

    struct slab_obj *obj = slab->free_objects;
    slab->free_objects = (struct slab_obj*)obj->list.next;

    obj->list.next = (struct list*)slab->allocated_objects;
    slab->allocated_objects = obj;

    if (slab->free_objects == NULL) {
        lst_push(&cache->slabs_full, &slab->list);
    } else {
        lst_push(&cache->slabs_partial, &slab->list);
    }

    return (void*)obj->data;
}

static struct slab_cache* find_cache_for_slab(struct slab *slab) {
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {
        struct slab_cache *cache = &caches[i];
        struct list *lst;

        for (lst = cache->slabs_full.next; lst != &cache->slabs_full; lst = lst->next)
            if ((struct slab *)lst == slab) return cache;

        for (lst = cache->slabs_partial.next; lst != &cache->slabs_partial; lst = lst->next)
            if ((struct slab *)lst == slab) return cache;

        for (lst = cache->slabs_empty.next; lst != &cache->slabs_empty; lst = lst->next)
            if ((struct slab *)lst == slab) return cache;
    }
    return NULL;
}


void kfree_slab(void *ptr) {
    if (!ptr) return;

    struct slab_obj *obj = (struct slab_obj*)((char*)ptr - offsetof(struct slab_obj, data));

    struct slab *slab = (struct slab*)((uintptr_t)obj & ~(PGSIZE - 1));

    struct slab_cache *cache = find_cache_for_slab(slab);
    if (!cache) return;

    slab_obj_list_remove(&slab->allocated_objects, obj);

    obj->list.next = (struct list*)slab->free_objects;
    slab->free_objects = obj;

    lst_remove(&slab->list);
    if (slab->allocated_objects == NULL) {
        lst_push(&cache->slabs_empty, &slab->list);
    } else {
        lst_push(&cache->slabs_partial, &slab->list);
    }
}

