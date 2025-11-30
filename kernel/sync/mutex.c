//
// Created by ShipOS developers on 03.01.24.
// Copyright (c) 2024 SHIPOS. All rights reserved.
//

#include "mutex.h"
#include "../lib/include/panic.h"
#include "../sched/scheduler.h"

extern struct cpu current_cpu;

int init_mutex(struct mutex *lk, char *name) {
    lk->locked = 0;
    lk->wait_list = 0;
    lk->name = name;

    init_spinlock(&lk->lock, "mutex_inner_lock");
}

void acquire_mutex(struct mutex *lk) {
    acquire_spinlock(&lk->lock);   // захватываем спинлок для защиты полей locked и wait_list

    while (lk->locked) {  // между пробуждением и захватом мьютекс могли перехватить
        push_thread_list(&lk->wait_list, current_cpu.current_thread);
        change_thread_state(current_cpu.current_thread, WAIT);
        
        release_spinlock(&lk->lock);  // если не отпустить спинлок, прерывания останутся выключенными (дедлок)
        
        yield();  // уходим в сон, отдавая процессор другим задачам
        
        acquire_spinlock(&lk->lock);  // спинлок для безопасной проверки lk->locked
    }

    lk->locked = 1;  // теперь мьютекс наш
    release_spinlock(&lk->lock);  // отпускаем защиту структуры
}

void release_mutex(struct mutex *lk) {
    acquire_spinlock(&lk->lock);

    lk->locked = 0;

    if (lk->wait_list) {
        struct thread *t = pop_thread_list(&lk->wait_list);
        change_thread_state(t, RUNNABLE);
    }
}