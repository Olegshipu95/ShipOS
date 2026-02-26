#ifndef UNTITLED_OS_SEQLOCK_H
#define UNTITLED_OS_SEQLOCK_H

#include "spinlock.h"

/**
 * @brief Sequence lock structure
 * Readers do not block writers. Writers are serialized via a spinlock.
 */
typedef struct seqlock {
    volatile uint32_t sequence;
    struct spinlock lock;
} seqlock_t;

void init_seqlock(seqlock_t *sl, const char *name);
void write_seqlock(seqlock_t *sl);
void write_sequnlock(seqlock_t *sl);
uint32_t read_seqbegin(seqlock_t *sl);
int read_seqretry(seqlock_t *sl, uint32_t start_seq);

#endif