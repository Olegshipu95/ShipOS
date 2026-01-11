#include "slab.h"
#include "../kalloc.h"
#include "../../memlayout.h"
#include "../../list/list.h"
#include "../../sync/spinlock.h"

#define SLAB_SIZES_COUNT 9
static size_t slab_sizes[SLAB_SIZES_COUNT] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

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

    struct spinlock lock;
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

static size_t align_ptr_size(size_t size) {
    size_t a = sizeof(void*);
    return (size + a - 1) & ~(a - 1);
}

void init_slab_cache() {
    for (int i = 0; i < SLAB_SIZES_COUNT; i++) {

        struct slab_cache *cache = &caches[i];

        cache->object_size = (unsigned int)align_ptr_size(slab_sizes[i]);

        lst_init(&cache->slabs_full);
        lst_init(&cache->slabs_partial);
        lst_init(&cache->slabs_empty);

        init_spinlock(&cache->lock, "slab_cache_lock");

        struct slab *s = alloc_slab(cache->object_size);
        if (s) {
            lst_init(&s->list);
            lst_push(&cache->slabs_empty, &s->list);
        }
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

    acquire_spinlock(&cache->lock);

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
            if (!slab) {
                release_spinlock(&cache->lock);
                return NULL;
            }
            lst_init(&slab->list);
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

    void *ret = (void*)obj->data;

    release_spinlock(&cache->lock);
    return ret;
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

    acquire_spinlock(&cache->lock);

    slab_obj_list_remove(&slab->allocated_objects, obj);

    obj->list.next = (struct list*)slab->free_objects;
    slab->free_objects = obj;

    lst_remove(&slab->list);
    if (slab->allocated_objects == NULL) {
        lst_push(&cache->slabs_empty, &slab->list);
    } else {
        lst_push(&cache->slabs_partial, &slab->list);
    }

    release_spinlock(&cache->lock);
}

size_t slab_get_cache_count(void) {
    return SLAB_SIZES_COUNT;
}

size_t slab_get_cache_object_size(int idx) {
    if (idx < 0 || idx >= SLAB_SIZES_COUNT) return 0;
    return caches[idx].object_size;
}

static size_t count_slabs_in_list(struct list *head) {
    size_t cnt = 0;
    struct list *it = head->next;
    while (it != head) {
        cnt++;
        it = it->next;
    }
    return cnt;
}

size_t slab_get_cache_slabs_full_count(int idx) {
    if (idx < 0 || idx >= SLAB_SIZES_COUNT) return 0;
    acquire_spinlock(&caches[idx].lock);
    size_t cnt = count_slabs_in_list(&caches[idx].slabs_full);
    release_spinlock(&caches[idx].lock);
    return cnt;
}

size_t slab_get_cache_slabs_partial_count(int idx) {
    if (idx < 0 || idx >= SLAB_SIZES_COUNT) return 0;
    acquire_spinlock(&caches[idx].lock);
    size_t cnt = count_slabs_in_list(&caches[idx].slabs_partial);
    release_spinlock(&caches[idx].lock);
    return cnt;
}

size_t slab_get_cache_slabs_empty_count(int idx) {
    if (idx < 0 || idx >= SLAB_SIZES_COUNT) return 0;
    acquire_spinlock(&caches[idx].lock);
    size_t cnt = count_slabs_in_list(&caches[idx].slabs_empty);
    release_spinlock(&caches[idx].lock);
    return cnt;
}

size_t slab_get_cache_total_objects(int idx) {
    if (idx < 0 || idx >= SLAB_SIZES_COUNT) return 0;
    size_t obj_sz = caches[idx].object_size + sizeof(struct list);
    return (PGSIZE - sizeof(struct slab)) / obj_sz;
}