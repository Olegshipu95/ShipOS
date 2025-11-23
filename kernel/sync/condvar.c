#include "condvar.h"
#include "../sched/scheduler.h"

extern struct cpu current_cpu;

void init_condvar(struct condvar *cv, char *name) {
    cv->wait_list = 0;
    init_spinlock(&cv->lock, name);
}

void cv_wait(struct condvar *cv, struct mutex *m) {
    // Берем лок CV, чтобы безопасно добавить себя в список
    acquire_spinlock(&cv->lock);
    
    push_thread_list(&cv->wait_list, current_cpu.current_thread);
    change_thread_state(current_cpu.current_thread, WAIT);
    
    // Отпускаем переданный мьютекс
    release_mutex(m);
    
    // Засыпаем (отпустив лок CV)
    release_spinlock(&cv->lock);
    yield();

    // Когда проснулись, обязаны снова захватить мьютекс
    acquire_mutex(m);
}

void cv_signal(struct condvar *cv) {
    acquire_spinlock(&cv->lock);
    if (cv->wait_list != 0) {
        struct thread *t = pop_thread_list(&cv->wait_list);
        change_thread_state(t, RUNNABLE);
    }
    release_spinlock(&cv->lock);
}

void cv_broadcast(struct condvar *cv) {
    acquire_spinlock(&cv->lock);
    while (cv->wait_list != 0) {
        struct thread *t = pop_thread_list(&cv->wait_list);
        change_thread_state(t, RUNNABLE);
    }
    release_spinlock(&cv->lock);
}