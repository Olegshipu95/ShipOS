#include "seqlock.h"

void init_seqlock(seqlock_t *sl, const char *name) {
    sl->sequence = 0;
    init_spinlock(&sl->lock, name);
}

void write_seqlock(seqlock_t *sl) {
    acquire_spinlock(&sl->lock);
    
    // Increment sequence to odd (start of write)
    // Full memory barrier before and after
    __sync_fetch_and_add(&sl->sequence, 1);
    __sync_synchronize(); 
}

void write_sequnlock(seqlock_t *sl) {
    // Full memory barrier to ensure data writes are visible
    __sync_synchronize();
    
    // Increment sequence back to even (end of write)
    __sync_fetch_and_add(&sl->sequence, 1);
    
    release_spinlock(&sl->lock);
}

uint32_t read_seqbegin(seqlock_t *sl) {
    uint32_t seq;
    while (1) {
        seq = sl->sequence;
        // If even, no write is in progress
        if ((seq & 1) == 0) {
            break;
        }
        // CPU hint that we are in a spin-wait loop
        asm volatile("pause"); 
    }
    // Ensure that following reads don't happen before we check sequence
    __sync_synchronize();
    return seq;
}

int read_seqretry(seqlock_t *sl, uint32_t start_seq) {
    // Ensure all data reads are finished before checking sequence again
    __sync_synchronize();
    return (sl->sequence != start_seq);
}