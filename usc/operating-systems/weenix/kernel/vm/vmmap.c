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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"
#include "mm/tlb.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        vmmap_t *map = NULL;

        dbg(DBG_PRINT, "(GRADING3B)\n");

        map = (vmmap_t *)slab_obj_alloc(vmmap_allocator);

        KASSERT(NULL != map);

        list_init(&map->vmm_list);
        map->vmm_proc = NULL;

        return map;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        vmarea_t *vma = NULL;

        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.a)\n");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {  
                if (vma->vma_obj != NULL) {
                        dbg(DBG_PRINT, "(GRADING3B)\n");
                        vma->vma_obj->mmo_ops->put(vma->vma_obj);
                }
                list_remove(&vma->vma_plink);
                list_remove(&vma->vma_olink);
                vmarea_free(vma);
        } list_iterate_end();

        map->vmm_proc=NULL;

        slab_obj_free(vmmap_allocator, map);
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        vmarea_t *vma = NULL;

        KASSERT(NULL != map && NULL != newvma);
        KASSERT(NULL == newvma->vma_vmmap);
        KASSERT(newvma->vma_start < newvma->vma_end);
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start
                && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
        dbg(DBG_PRINT, "(GRADING3A 3.b)\n");

        newvma->vma_vmmap = map;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                if (newvma->vma_start < vma->vma_start) {
                        dbg(DBG_PRINT, "(GRADING3B)\n");

                        list_insert_before(&vma->vma_plink, &newvma->vma_plink);
                        return;
                }
        } list_iterate_end();
        list_insert_tail(&map->vmm_list, &newvma->vma_plink);
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        vmarea_t *vma = NULL;
        uint32_t index;

        KASSERT(NULL != map);
        KASSERT(0 < npages);
        dbg(DBG_PRINT, "(GRADING3A 3.c)\n");

        if (dir == VMMAP_DIR_HILO) {
                dbg(DBG_PRINT,"(GRADING3B)\n");
                index = ADDR_TO_PN(USER_MEM_HIGH);
                list_iterate_reverse(&map->vmm_list, vma, vmarea_t, vma_plink) {
                        if (index - vma->vma_end >= npages) {
                                dbg(DBG_PRINT,"(GRADING3B)\n");
                                return index - npages;
                        }

                        index = vma->vma_start;
                } list_iterate_end();
                if (index - ADDR_TO_PN(USER_MEM_LOW) >= npages) {
                        dbg(DBG_PRINT,"(GRADING3B)\n");
                        return index - npages;
                }
        }
        KASSERT(dir != VMMAP_DIR_LOHI);
        /* Nobody use low to high now */
        /*
        if (dir == VMMAP_DIR_LOHI) {
                index = ADDR_TO_PN(USER_MEM_LOW);
                list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                        if (vma->vma_start - index >= npages) {
                                return index;
                        }

                        index = vma->vma_end;
                } list_iterate_end();
                if (ADDR_TO_PN(USER_MEM_HIGH) - index >= npages) {
                        return index;
                }
        }
        */
        return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        vmarea_t *vma = NULL;

        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                if (vma->vma_start <= vfn && vma->vma_end > vfn) {
                        dbg(DBG_PRINT,"(GRADING3B)\n");
                        return vma;
                }
        } list_iterate_end();

        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        vmmap_t *newmap = NULL;
        vmarea_t *vma, *newvma;

        vma = NULL;
        newvma = NULL;

        dbg(DBG_PRINT, "(GRADING3B)\n");
        newmap = vmmap_create();
        KASSERT(NULL != newmap);

        newmap->vmm_proc = map->vmm_proc;

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                newvma = vmarea_alloc();
                KASSERT(NULL != newvma);

                newvma->vma_start = vma->vma_start;
                newvma->vma_end = vma->vma_end;
                newvma->vma_off = vma->vma_off;
                newvma->vma_prot = vma->vma_prot;
                newvma->vma_flags = vma->vma_flags;
                newvma->vma_vmmap = newmap;
                newvma->vma_obj = NULL;
                list_insert_tail(&newmap->vmm_list, &newvma->vma_plink);
        } list_iterate_end();

        return newmap;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        int err;
        vmarea_t *vma = NULL;
        mmobj_t *o = NULL;
        mmobj_t *shadow = NULL;

        KASSERT(NULL != map);
        KASSERT(0 < npages);
        KASSERT(!(~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC) & prot));
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
        KASSERT(PAGE_ALIGNED(off));
        dbg(DBG_PRINT, "(GRADING3A 3.f)\n");

        if (lopage == 0) {
                dbg(DBG_PRINT, "(GRADING3B)\n");
                lopage = vmmap_find_range(map, npages, dir);
                if ((int)lopage < 0) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        return -1;
                }
        } else {
                dbg(DBG_PRINT, "(GRADING3B)\n");
                err = vmmap_remove(map, lopage, npages);
                KASSERT(err >= 0);
        }

        vma = vmarea_alloc();
        KASSERT(NULL != vma);
        vma->vma_start = lopage;
        vma->vma_end = lopage + npages;
        vma->vma_off = ADDR_TO_PN(off);
        vma->vma_prot = prot;
        vma->vma_flags = flags;

        if (file == NULL) {
                dbg(DBG_PRINT, "(GRADING3B)\n");
                o = anon_create(); 
        } else {
                dbg(DBG_PRINT, "(GRADING3B)\n");
                err = file->vn_ops->mmap(file, vma, &o);
                KASSERT(err >= 0);
                /*o->mmo_ops->ref(o);*/
        }

        vma->vma_obj = o;
        list_insert_tail(&o->mmo_un.mmo_vmas, &vma->vma_olink);
        
        if (flags & MAP_PRIVATE) {
                dbg(DBG_PRINT,"(GRADING3B)\n");
                shadow = shadow_create();
                shadow->mmo_shadowed = o;
                shadow->mmo_un.mmo_bottom_obj = o;
                vma->vma_obj = shadow;
        }

        if (new) {
                dbg(DBG_PRINT,"(GRADING3B)\n");
                *new = vma;
        }

        vmmap_insert(map, vma);

        dbg(DBG_PRINT, "Leave vmmap_map\n");

        return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        vmarea_t *vma, *newvma;
        mmobj_t *shadow = NULL;

        vma = NULL;
        newvma = NULL;

        dbg(DBG_PRINT, "(GRADING3B)\n");
        list_iterate_begin (&map->vmm_list, vma, vmarea_t, vma_plink) {
                if (lopage > vma->vma_start && lopage + npages < vma->vma_end) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        newvma = vmarea_alloc();

                        newvma->vma_start = lopage + npages;
                        newvma->vma_end = vma->vma_end;
                        newvma->vma_flags = vma->vma_flags;
                        newvma->vma_prot = vma->vma_prot;
                        newvma->vma_off = vma->vma_off - vma->vma_start + lopage + npages;
                        vma->vma_end = lopage;
                        vmmap_insert(map, newvma);
                        vma->vma_obj->mmo_ops->ref(vma->vma_obj);

                        if (newvma->vma_flags & MAP_PRIVATE) {
                                dbg(DBG_PRINT, "(GRADING3D)\n");

                                shadow = shadow_create();
                                shadow->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;
                                shadow->mmo_shadowed = vma->vma_obj;
                                newvma->vma_obj = shadow;

                                shadow = shadow_create();
                                shadow->mmo_un.mmo_bottom_obj = newvma->vma_obj->mmo_un.mmo_bottom_obj;
                                shadow->mmo_shadowed = newvma->vma_obj->mmo_shadowed;
                                vma->vma_obj = shadow;
                                list_insert_tail(&(vma->vma_obj->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas), &newvma->vma_olink);
                        }
                 } else if (lopage <= vma->vma_start && lopage + npages < vma->vma_end && lopage + npages > vma->vma_start) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        vma->vma_off += lopage + npages - vma->vma_start;
                        vma->vma_start = lopage + npages;
                 } else if (lopage > vma->vma_start && lopage + npages >= vma->vma_end && lopage  < vma->vma_end) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        vma->vma_end = lopage;
                 } else if (lopage <= vma->vma_start && lopage + npages >= vma->vma_end) {
                        dbg(DBG_PRINT, "(GRADING3D)\n");
                        vma->vma_obj->mmo_ops->put(vma->vma_obj);
                        list_remove(&vma->vma_plink);
                        list_remove(&vma->vma_olink);
                        vmarea_free(vma);
                 }
        } list_iterate_end();

        tlb_flush_all();
        pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage),
                       (uintptr_t)PN_TO_ADDR(lopage + npages));

        return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        vmarea_t *vma = NULL;

        KASSERT((ADDR_TO_PN(USER_MEM_LOW) <= startvfn)
                && (ADDR_TO_PN(USER_MEM_HIGH) >= startvfn + npages));
        dbg(DBG_PRINT, "(GRADING3A 3.e)\n");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                if (startvfn < vma->vma_start && startvfn + npages > vma->vma_start) {
                        dbg(DBG_PRINT,"(GRADING3B)\n");
                        return 0;
                }
                if (startvfn >= vma->vma_start && startvfn < vma->vma_end) {
                        dbg(DBG_PRINT,"(GRADING3B)\n");
                        return 0;
                }
        } list_iterate_end();

        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        int err;
        uint32_t vfn, off, offset;
        vmarea_t *vma = NULL;
        pframe_t *pframe = NULL;
        char *index;

        index = (char *)buf;
        vfn = ADDR_TO_PN(vaddr);
        /* bytes offset */
        offset = PAGE_OFFSET(vaddr);

        while ((int)count > 0) {
                vma = vmmap_lookup(map, vfn);
                KASSERT(vma && vma->vma_obj);

                /* page frame offset */
                off = vma->vma_off - vma->vma_start;

                err = vma->vma_obj->mmo_ops->lookuppage(vma->vma_obj, vfn + off, 0, &pframe);
                if (err < 0) {
                        dbg(DBG_PRINT,"(GRADING3D)\n");
                        return err;
                }

                memcpy((void *)index, (void *)((uintptr_t)pframe->pf_addr + offset), MIN(count, PAGE_SIZE - offset));
                index += PAGE_SIZE - offset;
                count -= PAGE_SIZE - offset;
                vfn ++;
                offset = 0;
                
        }
        return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        uint32_t vfn, off, offset;
        int err;
        vmarea_t *vma = NULL;
        pframe_t *pframe = NULL;
        char *index;

        index = (char *)buf;
        vfn = ADDR_TO_PN(vaddr);
        /* bytes offset */
        offset = PAGE_OFFSET(vaddr);

        while ((int)count > 0) {
                vma = vmmap_lookup(map, vfn);
                KASSERT(vma && vma->vma_obj);

                /* page frame offset */
                off = vma->vma_off - vma->vma_start;

                err = vma->vma_obj->mmo_ops->lookuppage(vma->vma_obj, vfn + off, 1, &pframe);
                if (err < 0) {
                        dbg(DBG_PRINT,"(GRADING3D)\n");
                        return err;
                }

                pframe_dirty(pframe);

                memcpy((void *)((uintptr_t)pframe->pf_addr + offset), (void *)index, MIN(count, PAGE_SIZE - offset));
                index += PAGE_SIZE - offset;
                count -= PAGE_SIZE + offset;
                vfn ++;
                offset = 0;
        }

        return 0;
}
