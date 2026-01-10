#ifdef SCHED_RR_THREAD

#include "scheduler.h"
#include "proc.h"
#include "threads.h"
#include "../lib/include/x86_64.h"
#include "../lib/include/panic.h"

struct context kcontext;
struct context *kcontext_ptr = &kcontext;

struct thread *get_next_thread(void) {
    if (!proc_list)
        panic("RR_THREAD: no procs");

    struct proc_node *pnode = proc_list;

    do {
        struct proc *p = pnode->data;
        if (p->threads) {
            struct thread *t = peek_thread_list(p->threads);
            shift_thread_list(&p->threads);

            if (t->state == RUNNABLE)
                return t;
        }
        pnode = pnode->next;
    } while (pnode != proc_list);

    panic("RR_THREAD: no runnable threads");
}

void scheduler() {
    while (1) {
        //printf("scheduling\n");
        struct thread *next_thread = get_next_thread();
        current_cpu.current_thread = next_thread;
        switch_context(&kcontext_ptr, next_thread->context);
    }
}

void yield() {
    //printf("yield\n");
    switch_context(&(current_cpu.current_thread->context), kcontext_ptr);
}

#endif