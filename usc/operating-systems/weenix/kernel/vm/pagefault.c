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
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page (don't forget
 * about shadow objects, especially copy-on-write magic!). Make
 * sure that if the user writes to the page it will be handled
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{
        int err;
        uint32_t pdflags, ptflags;
        uintptr_t paddr;
        vmarea_t *vma = NULL;
        pframe_t *pframe = NULL;

        dbg(DBG_PRINT, "Go into handle_pagefault\n");

        vma = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(vaddr));

        if (vma == NULL) {
                dbg(DBG_PRINT, "(GRADING3C)\n");
                proc_kill(curproc, EFAULT);
        }
        if (cause & FAULT_EXEC && ! (vma->vma_prot & PROT_EXEC)) {
                proc_kill(curproc, EFAULT);
        }
        if (cause & FAULT_WRITE && ! (vma->vma_prot & PROT_WRITE)) {
                proc_kill(curproc, EFAULT);
        }
        if (!(vma->vma_prot & PROT_READ)) {
                proc_kill(curproc, EFAULT);
        }

        err = pframe_lookup(vma->vma_obj, vma->vma_off + ADDR_TO_PN(vaddr) - vma->vma_start, cause & FAULT_WRITE ? 1 : 0, &pframe);

        if (err < 0) {
                dbg(DBG_PRINT, "(GRADING3C)\n");
                proc_kill(curproc, EFAULT);
        }

        pdflags = PD_PRESENT | PD_USER;
        ptflags = PT_PRESENT | PT_USER;

        if (cause & FAULT_WRITE) {
                dbg(DBG_PRINT, "(GRADING3B)\n");
                pdflags |= PD_WRITE;
                ptflags |= PT_WRITE;
                pframe_dirty(pframe);
        }

        pframe_pin(pframe);

        paddr = pt_virt_to_phys((uintptr_t)pframe->pf_addr);

        
        pt_map(curproc->p_pagedir, (uint32_t)PAGE_ALIGN_DOWN(vaddr),
               (uint32_t)PAGE_ALIGN_DOWN(paddr), pdflags, ptflags) ;

        pframe_unpin(pframe);

        dbg(DBG_PRINT, "Leave handle_pagefault\n");
}
