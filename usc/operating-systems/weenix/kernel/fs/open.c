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

/*
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND or O_CREAT.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int
do_open(const char *filename, int oflags)
{
        int fd, ret;
        file_t *f = NULL;
        vnode_t *res_vnode = NULL;

        fd = get_empty_fd(curproc);
        if (fd == -EMFILE) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EMFILE;
        }
        f = fget(-1);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -ENOMEM;
        }

        if ((oflags & O_WRONLY) == O_WRONLY && (oflags & O_RDWR) == O_RDWR) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EINVAL;
        } else if ((oflags & O_WRONLY) == O_WRONLY) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                f->f_mode = FMODE_WRITE;
        } else if ((oflags & O_RDWR) == O_RDWR) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                f->f_mode = FMODE_READ | FMODE_WRITE;
        } else {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                f->f_mode = FMODE_READ;
        }
        if ((oflags & O_APPEND) == O_APPEND) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                f->f_mode = f->f_mode | FMODE_APPEND;
        }

        ret = open_namev(filename, oflags, &res_vnode, NULL);

        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                fput(f);
                curproc->p_files[fd] = NULL;

                return ret;
        }
        if (S_ISDIR(res_vnode->vn_mode) && (oflags & O_WRONLY || oflags & O_RDWR)) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                fput(f);
                curproc->p_files[fd] = NULL;
                vput(res_vnode);
                return -EISDIR;
        }

        dbg(DBG_PRINT, "(GRADING2D)\n");
        curproc->p_files[fd] = f;
        f->f_vnode = res_vnode;
        f->f_pos = 0;

        return fd;
}
