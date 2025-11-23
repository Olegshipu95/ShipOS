#ifndef UNTITLED_OS_BARRIER_H
#define UNTITLED_OS_BARRIER_H

#include "spinlock.h"
#include "../sched/threads.h"

struct barrier {
    int threshold;      // Сколько потоков нужно
    int count;          // Сколько уже пришло
    int generation;     // Чтобы различать разные поколения использования барьера
    struct spinlock lock;
    struct thread_node *wait_list;
};

void init_barrier(struct barrier *b, int count, char *name);
void barrier_wait(struct barrier *b);

#endif