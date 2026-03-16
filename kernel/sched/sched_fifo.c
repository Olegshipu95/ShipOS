// #ifdef SCHED_FIFO

#include "scheduler.h"
#include "proc.h"
#include "threads.h"
#include "../lib/include/x86_64.h"
#include "../lib/include/panic.h"
#include "../lib/include/logging.h"

struct context kcontext;
struct context *kcontext_ptr = &kcontext;

struct thread *get_next_thread(void) {
    if (!proc_list)
        panic("FIFO: no procs");

    LOG("Step into next_thread");
    struct proc_node *pnode = proc_list;
    LOG("Get process node");
    do {
        struct proc *p = pnode->data;
        LOG("Get process");
        if (p->threads) {
            LOG("Has threads");
            struct thread_node *tnode = p->threads;
            LOG("Get thread node");
            do {
                if (tnode->data != current_cpu.current_thread && tnode->data->state == RUNNABLE) {
                    return tnode->data;
                }
                LOG("Thread not runnable");
                tnode = tnode->next;
                LOG("Get new thread node");
            } while (tnode != p->threads);
        }
        LOG("Thread done");
        pnode = pnode->next;
        LOG("Get new process node");
    } while (pnode != proc_list);

    panic("FIFO: no runnable threads");
}

void scheduler() {
    while (1) {
        //printf("scheduling\n");
        struct thread *next_thread = get_next_thread();
        LOG("GOT THREAD LET'S GOOOOOO");
        current_cpu.current_thread = next_thread;
        switch_context(&kcontext_ptr, next_thread->context);
    }
}

void yield() {
    //printf("yield\n");
    switch_context(&(current_cpu.current_thread->context), kcontext_ptr);
}
// #endif