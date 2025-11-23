#ifndef UNTITLED_OS_SEQLOCK_H
#define UNTITLED_OS_SEQLOCK_H

#include "spinlock.h"
#include "../lib/include/atomic.h"

typedef struct seqlock {
    uint32_t sequence;
    struct spinlock lock; // Блокировка только для write
} seqlock_t;

void init_seqlock(seqlock_t *sl, char *name);
void write_seqlock(seqlock_t *sl);
void write_sequnlock(seqlock_t *sl);
uint32_t read_seqbegin(seqlock_t *sl);
int read_seqretry(seqlock_t *sl, uint32_t start_seq);

#endif