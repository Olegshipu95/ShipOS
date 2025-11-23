#include "semaphore.h"
#include "../sched/scheduler.h"

extern struct cpu current_cpu;

void sem_init(struct semaphore *s, int value, char *name) {
    s->value = value;
    s->wait_list = 0;
    init_spinlock(&s->lock, name);
}

void sem_wait(struct semaphore *s) {
    acquire_spinlock(&s->lock);

    while (s->value <= 0) {
        push_thread_list(&s->wait_list, current_cpu.current_thread);
        change_thread_state(current_cpu.current_thread, WAIT);
        release_spinlock(&s->lock);
        
        yield();
        
        acquire_spinlock(&s->lock);
    }

    s->value--;
    release_spinlock(&s->lock);
}

void sem_post(struct semaphore *s) {
    acquire_spinlock(&s->lock);
    
    s->value++;
    
    if (s->wait_list != 0) {
        struct thread *t = pop_thread_list(&s->wait_list);
        change_thread_state(t, RUNNABLE);
    }
    
    release_spinlock(&s->lock);
}