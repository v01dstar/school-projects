/******************************************************************************/
/* Important Spring 2015 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "config.h"
#include "globals.h"

#include "errno.h"

#include "util/init.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"

#include "mm/slab.h"
#include "mm/page.h"

kthread_t *curthr; /* global */
static slab_allocator_t *kthread_allocator = NULL;

#ifdef __MTP__
/* Stuff for the reaper daemon, which cleans up dead detached threads */
static proc_t *reapd = NULL;
static kthread_t *reapd_thr = NULL;
static ktqueue_t reapd_waitq;
static list_t kthread_reapd_deadlist; /* Threads to be cleaned */

static void *kthread_reapd_run(int arg1, void *arg2);
#endif

void
kthread_init()
{
        kthread_allocator = slab_allocator_create("kthread", sizeof(kthread_t));
        KASSERT(NULL != kthread_allocator);
}

/**
 * Allocates a new kernel stack.
 *
 * @return a newly allocated stack, or NULL if there is not enough
 * memory available
 */
static char *
alloc_stack(void)
{
        /* extra page for "magic" data */
        char *kstack;
        int npages = 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT);
        kstack = (char *)page_alloc_n(npages);

        return kstack;
}

/**
 * Frees a stack allocated with alloc_stack.
 *
 * @param stack the stack to free
 */
static void
free_stack(char *stack)
{
        page_free_n(stack, 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT));
}

void
kthread_destroy(kthread_t *t)
{
        KASSERT(t && t->kt_kstack);
        free_stack(t->kt_kstack);
        if (list_link_is_linked(&t->kt_plink))
                list_remove(&t->kt_plink);

        slab_obj_free(kthread_allocator, t);
}

/*
 * Allocate a new stack with the alloc_stack function. The size of the
 * stack is DEFAULT_STACK_SIZE.
 *
 * Don't forget to initialize the thread context with the
 * context_setup function. The context should have the same pagetable
 * pointer as the process.
 */
kthread_t *
kthread_create(struct proc *p, kthread_func_t func, long arg1, void *arg2)
{
        kthread_t *kthr = NULL;

        KASSERT(NULL != p);
        dbg(DBG_PRINT, "(GRADING1A 3.a)\n");

        kthr = (kthread_t *)slab_obj_alloc(kthread_allocator);
        memset(kthr, 0, sizeof(kthread_t));

        /* initialize the TCB */
        kthr->kt_kstack = alloc_stack();
        kthr->kt_retval = 0;
        kthr->kt_errno = 0;
        kthr->kt_proc = p;
        kthr->kt_cancelled = 0;
        kthr->kt_wchan = NULL;
        kthr->kt_state = KT_NO_STATE;
        list_link_init(&kthr->kt_qlink);
        list_link_init(&kthr->kt_plink);
        list_insert_tail(&p->p_threads, &kthr->kt_plink);

        /* initialize the context */
        context_setup(&kthr->kt_ctx, func, arg1, arg2, kthr->kt_kstack,
                      DEFAULT_STACK_SIZE, p->p_pagedir);
        return kthr;
}

/*
 * If the thread to be cancelled is the current thread, this is
 * equivalent to calling kthread_exit. Otherwise, the thread is
 * sleeping and we need to set the cancelled and retval fields of the
 * thread.
 *
 * If the thread's sleep is cancellable, cancelling the thread should
 * wake it up from sleep.
 *
 * If the thread's sleep is not cancellable, we do nothing else here.
 */
void
kthread_cancel(kthread_t *kthr, void *retval)
{
        KASSERT(NULL != kthr);
        dbg(DBG_PRINT, "(GRADING1A 3.b)\n");

        if (kthr == curthr) {
                kthread_exit(retval);
        } else {
                kthr->kt_cancelled = 1;
                kthr->kt_retval = retval;
                if (kthr->kt_state == KT_SLEEP_CANCELLABLE)
                        sched_cancel(kthr);
        }
}

/*
 * You need to set the thread's retval field, set its state to
 * KT_EXITED, and alert the current process that a thread is exiting
 * via proc_thread_exited.
 *
 * It may seem unneccessary to push the work of cleaning up the thread
 * over to the process. However, if you implement MTP, a thread
 * exiting does not necessarily mean that the process needs to be
 * cleaned up.
 */
void
kthread_exit(void *retval)
{
        KASSERT(!curthr->kt_wchan);
        KASSERT(!curthr->kt_qlink.l_next && !curthr->kt_qlink.l_prev);
        KASSERT(curthr->kt_proc == curproc);
        dbg(DBG_PRINT, "(GRADING1A 3.c)\n");

        curthr->kt_retval = retval;
        curthr->kt_state = KT_EXITED;
        proc_thread_exited(retval);
}

/*
 * The new thread will need its own context and stack. Think carefully
 * about which fields should be copied and which fields should be
 * freshly initialized.
 *
 * You do not need to worry about this until VM.
 */
kthread_t *
kthread_clone(kthread_t *thr)
{
        kthread_t *newthr = NULL;

        KASSERT(KT_RUN == thr->kt_state);
        dbg(DBG_PRINT, "(GRADING3A 8.a)\n");

        newthr = (kthread_t *)slab_obj_alloc(kthread_allocator);
        KASSERT(NULL != newthr);

        newthr->kt_kstack = alloc_stack();
        KASSERT(NULL != newthr->kt_kstack);
        newthr->kt_ctx.c_kstack = (uintptr_t)newthr->kt_kstack;
        newthr->kt_ctx.c_kstacksz = thr->kt_ctx.c_kstacksz;
        newthr->kt_retval = thr->kt_retval;
        newthr->kt_errno = thr->kt_errno;
        newthr->kt_proc = NULL;
        newthr->kt_cancelled = thr->kt_cancelled;
        newthr->kt_state = KT_RUN;
        newthr->kt_wchan = thr->kt_wchan;
        list_link_init(&newthr->kt_qlink);
        list_link_init(&newthr->kt_plink);
        
        if (newthr->kt_wchan) {
                dbg(DBG_PRINT,"(GRADING3B)\n");
                list_insert_head(&newthr->kt_wchan->tq_list, &newthr->kt_qlink);
                newthr->kt_wchan->tq_size ++;
        }

        KASSERT(KT_RUN == newthr->kt_state);
        dbg(DBG_PRINT, "(GRADING3A 8.a)\n");

        return newthr;
}

/*
 * The following functions will be useful if you choose to implement
 * multiple kernel threads per process. This is strongly discouraged
 * unless your weenix is perfect.
 */
#ifdef __MTP__
int
kthread_detach(kthread_t *kthr)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_detach");
        return 0;
}

int
kthread_join(kthread_t *kthr, void **retval)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_join");
        return 0;
}

/* ------------------------------------------------------------------ */
/* -------------------------- REAPER DAEMON ------------------------- */
/* ------------------------------------------------------------------ */
static __attribute__((unused)) void
kthread_reapd_init()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_init");
}
init_func(kthread_reapd_init);
init_depends(sched_init);

void
kthread_reapd_shutdown()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_shutdown");
}

static void *
kthread_reapd_run(int arg1, void *arg2)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_run");
        return (void *) 0;
}
#endif
