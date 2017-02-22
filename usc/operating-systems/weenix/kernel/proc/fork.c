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

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
        int i;
        proc_t *proc = NULL;
        kthread_t *thr = NULL;
        vmarea_t *vmac, *vmap;
        mmobj_t *shadowc, *shadowp;

        KASSERT(regs != NULL);
        KASSERT(curproc != NULL);
        KASSERT(curproc->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        vmac = NULL;
        vmap = NULL;
        shadowc = NULL;
        shadowp = NULL;

        proc = proc_create("child");

        KASSERT(proc->p_state == PROC_RUNNING);
        KASSERT(proc->p_pagedir != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        proc->p_brk = curproc->p_brk;
        proc->p_start_brk = curproc->p_start_brk;
        proc->p_vmmap = vmmap_clone(curproc->p_vmmap);
        proc->p_vmmap->vmm_proc = proc;

        list_iterate_begin(&proc->p_vmmap->vmm_list, vmac, vmarea_t, vma_plink) {
                vmap = vmmap_lookup(curproc->p_vmmap, vmac->vma_start);

                vmac->vma_obj = vmap->vma_obj;
                vmap->vma_obj->mmo_ops->ref(vmap->vma_obj);

                if (vmap->vma_flags & MAP_PRIVATE) {
                        dbg(DBG_PRINT,"(GRADING3C)\n" );

                        shadowc = shadow_create();
                        shadowp = shadow_create();

                        shadowc->mmo_shadowed = vmap->vma_obj;
                        shadowc->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(vmap->vma_obj);
                        

                        shadowp->mmo_shadowed = vmap->vma_obj;
                        shadowp->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(vmap->vma_obj);



                        vmac->vma_obj = shadowc;
                        vmap->vma_obj = shadowp;

                }

                list_insert_tail(mmobj_bottom_vmas(vmac->vma_obj), &vmac->vma_olink);
        } list_iterate_end();

        pt_unmap_range(pt_get(), USER_MEM_LOW, USER_MEM_HIGH);
        pt_unmap_range(proc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);

        thr = kthread_clone(curthr);

        KASSERT(thr->kt_kstack != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        thr->kt_proc = proc;
        list_insert_tail(&proc->p_threads, &thr->kt_plink);

        regs->r_eax = 0;
        thr->kt_ctx.c_pdptr = proc->p_pagedir;
        thr->kt_ctx.c_eip = (uint32_t)userland_entry;
        thr->kt_ctx.c_esp = fork_setup_stack(regs, thr->kt_kstack);
        regs->r_eax = proc->p_pid;

        for (i = 0; i < NFILES; i++) {
                if (curproc->p_files[i]) {
                        dbg(DBG_PRINT, "(GRADING3B)\n");
                        proc->p_files[i] = curproc->p_files[i];
                        fref(curproc->p_files[i]);
                }
        }

        sched_make_runnable(thr);

        regs->r_eax = proc->p_pid;

        tlb_flush_all();

        return regs->r_eax;
}
