#include "completion.h"
#include "../sched/scheduler.h"

extern struct cpu current_cpu;

void init_completion(struct completion *c, char *name) {
    c->done = 0;
    c->wait_list = 0;
    init_spinlock(&c->lock, name);
}

void wait_for_completion(struct completion *c) {
    acquire_spinlock(&c->lock);
    while (!c->done) {
        push_thread_list(&c->wait_list, current_cpu.current_thread);
        change_thread_state(current_cpu.current_thread, WAIT);
        release_spinlock(&c->lock);
        yield();
        acquire_spinlock(&c->lock);
    }
    release_spinlock(&c->lock);
}

void complete(struct completion *c) {
    acquire_spinlock(&c->lock);
    c->done = 1;
    // Будим всех, кто ждал
    while (c->wait_list != 0) {
        struct thread *t = pop_thread_list(&c->wait_list);
        change_thread_state(t, RUNNABLE);
    }
    release_spinlock(&c->lock);
}

void complete_all(struct completion *c) {
    complete(c);
}