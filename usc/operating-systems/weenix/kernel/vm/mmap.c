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

#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
        int err;
        uint32_t lopage, npages;
        file_t *f = NULL;
        vnode_t *vnode = NULL;
        vmarea_t *vma = NULL;

        KASSERT(NULL != curproc->p_pagedir);
        dbg(DBG_PRINT, "(GRADING3A 2.a)\n");

        if (PAGE_ALIGNED(off) == 0 || len == 0 || len > 0xc0000000) {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return -EINVAL;
        }

        if (flags & MAP_FIXED) {
                lopage = ADDR_TO_PN(addr);
                if ((uintptr_t)addr < USER_MEM_LOW || (uintptr_t)addr > USER_MEM_HIGH) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        return -EINVAL;
                }
        } else {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                lopage = 0;
        }
        npages = (len - 1) / PAGE_SIZE + 1;
 
        if (!(MAP_SHARED & flags) && !(MAP_PRIVATE & flags)) {
                dbg(DBG_PRINT, "(GRADING3C)\n");
                return -EINVAL;
        }

        if (~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC) & prot) {
                dbg(DBG_PRINT, "(GRADING3C)\n");
                return -EINVAL;
        }

        if (!(flags & MAP_ANON)) {
                f = fget(fd);
                if (f == NULL) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        return -EBADF;
                }

                if ((flags & MAP_PRIVATE && !(f->f_mode & FMODE_READ)) ||
                    (flags & MAP_SHARED && prot & PROT_WRITE && f->f_mode != (FMODE_READ | FMODE_WRITE))) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        fput(f);

                        return -EACCES;
                }
                vnode = f->f_vnode;
        }

        err = vmmap_map(curproc->p_vmmap, vnode, lopage, npages, prot, flags, off, VMMAP_DIR_HILO, &vma);

        if (f)
                fput(f);

        if (err < 0) {
                dbg(DBG_PRINT,"(GRADING3D)\n");
                return err;
        }
        
        if (vma == NULL) {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return -1;
        }

        *ret = PN_TO_ADDR(vma->vma_start);

        tlb_flush_all();

        return 0;
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
        uint32_t lopage, npages;

        KASSERT(NULL != curproc->p_pagedir);
        dbg(DBG_PRINT, "(GRADING3A 2.b)\n");

        if (len == 0 || len > 0xc0000000) {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return -EINVAL;
        }

        if ((uintptr_t)addr < USER_MEM_LOW || (uintptr_t)addr + len > USER_MEM_HIGH || (uintptr_t)addr > USER_MEM_HIGH) {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return -EINVAL;
        }

        lopage = ADDR_TO_PN(addr);
        npages = (len - 1) / PAGE_SIZE + 1;

        vmmap_remove(curproc->p_vmmap, lopage, npages);

        tlb_flush_all();
        
        return 0;
}

