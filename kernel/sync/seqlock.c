#include "seqlock.h"

void init_seqlock(seqlock_t *sl, char *name) {
    sl->sequence = 0;
    init_spinlock(&sl->lock, name);
}

void write_seqlock(seqlock_t *sl) {
    acquire_spinlock(&sl->lock);
    // Увеличиваем seq, делая его нечетным (начало записи)
    atomic_fetch_add(&sl->sequence, 1);
    __sync_synchronize(); // Барьер памяти
}

void write_sequnlock(seqlock_t *sl) {
    __sync_synchronize();
    // Увеличиваем seq, делая его четным (конец записи)
    atomic_fetch_add(&sl->sequence, 1);
    release_spinlock(&sl->lock);
}

uint32_t read_seqbegin(seqlock_t *sl) {
    uint32_t seq;
    while (1) {
        seq = atomic_load(&sl->sequence);
        if (seq % 2 == 0) break; // Если четное - записи нет, можно читать
        // Иначе ждем до завершения записи
        asm volatile("pause"); 
    }
    __sync_synchronize();
    return seq;
}

int read_seqretry(seqlock_t *sl, uint32_t start_seq) {
    __sync_synchronize();
    return (atomic_load(&sl->sequence) != start_seq);
}