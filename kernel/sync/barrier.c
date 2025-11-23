#include "barrier.h"
#include "../sched/scheduler.h"

extern struct cpu current_cpu;

void init_barrier(struct barrier *b, int count, char *name) {
    b->threshold = count;
    b->count = 0;
    b->generation = 0;
    b->wait_list = 0;
    init_spinlock(&b->lock, name);
}

void barrier_wait(struct barrier *b) {
    acquire_spinlock(&b->lock);

    int gen = b->generation;
    b->count++;

    if (b->count >= b->threshold) {
        // Мы последние, гооол!
        b->count = 0;
        b->generation++; // Начинаем новое поколение
        
        // Будим всех
        while (b->wait_list != 0) {
            struct thread *t = pop_thread_list(&b->wait_list);
            change_thread_state(t, RUNNABLE);
        }
        release_spinlock(&b->lock);
    } else {
        // Мы не последние, надо спать
        push_thread_list(&b->wait_list, current_cpu.current_thread);
        change_thread_state(current_cpu.current_thread, WAIT);
        
        release_spinlock(&b->lock);
        yield();
    }
}