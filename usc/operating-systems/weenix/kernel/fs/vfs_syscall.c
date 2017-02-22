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
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.10 2014/12/22 16:15:17 william Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read fs_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)
{
        file_t *f = NULL;
        unsigned int bytes;

        if (fd == -1) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        f = fget(fd);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        if ((f->f_mode & FMODE_READ) != FMODE_READ) {
                fput(f);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        if (S_ISDIR(f->f_vnode->vn_mode)) {
                fput(f);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EISDIR;
        }

        bytes = f->f_vnode->vn_ops->read(f->f_vnode, f->f_pos, buf, nbytes);

        if (bytes == nbytes) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                do_lseek(fd, bytes, SEEK_CUR);
        } else {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                do_lseek(fd, 0, SEEK_END);
        }
        fput(f);

        return bytes;
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * fs_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
        file_t *f = NULL;
        int bytes;

        if (fd == -1) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        f = fget(fd);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        if ((f->f_mode & FMODE_WRITE) != FMODE_WRITE &&
            (f->f_mode & FMODE_APPEND) != FMODE_APPEND ) {
                fput(f);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        if ((f->f_mode & FMODE_APPEND) == FMODE_APPEND) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                do_lseek(fd, 0, SEEK_END);
        }

        bytes = f->f_vnode->vn_ops->write(f->f_vnode, f->f_pos, buf, nbytes);
        if (bytes > 0) {
                KASSERT((S_ISCHR(f->f_vnode->vn_mode)) ||
                        (S_ISBLK(f->f_vnode->vn_mode)) ||
                        ((S_ISREG(f->f_vnode->vn_mode)) && (f->f_pos <= f->f_vnode->vn_len)));
                dbg(DBG_PRINT, "(GRADING2A 3.a)\n");

                do_lseek(fd, bytes, SEEK_CUR);
        }

        fput(f);

        return bytes;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
        file_t *f = NULL;

        dbg(DBG_PRINT, "(GRADING2D)\n");

        if (fd == -1) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        f = fget(fd);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        dbg(DBG_PRINT, "(GRADING2D)\n");
        fput(f);
        fput(f);
        curproc->p_files[fd] = NULL;

        return 0;
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{
        file_t *f = NULL;
        int new_fd;

        dbg(DBG_PRINT, "(GRADING2D)\n");

        if (fd == -1) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        f = fget(fd);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }

        new_fd = get_empty_fd(curproc);
        if (new_fd == -EMFILE) {
                fput(f);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EMFILE;
        }

        dbg(DBG_PRINT, "(GRADING2D)\n");
        curproc->p_files[new_fd] = f;

        return new_fd;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
        file_t *f = NULL;

        dbg(DBG_PRINT, "(GRADING2D)\n");

        if (ofd == -1) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        if (nfd < 0 || nfd >= NFILES) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        f = fget(ofd);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }


        if (curproc->p_files[nfd] != NULL) {
                if (nfd != ofd) {
                        dbg(DBG_PRINT, "(GRADING2D)\n");
                        do_close(nfd);
                } else {
                        dbg(DBG_PRINT, "(GRADING2D)\n");
                        fput(f);
                }
        }

        dbg(DBG_PRINT, "(GRADING2D)\n");
        curproc->p_files[nfd] = f;
        
        return nfd;
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mknod(const char *path, int mode, unsigned devid)
{
        size_t namelen;
        const char *name = NULL;
        vnode_t *dir, *result;
        int ret;

        dir = NULL;
        result = NULL;
        if (!S_ISCHR(mode) && !S_ISBLK(mode)) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EINVAL;
        }
        ret = dir_namev(path, &namelen, &name, NULL, &dir);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        ret = lookup(dir, name, namelen, &result);
        if (ret < 0) {
                if (ret == -ENOTDIR || dir->vn_ops->mknod == NULL || !S_ISDIR(dir->vn_mode)){
                        vput(dir);
                        dbg(DBG_PRINT, "(GRADING2D)\n");
                        return -ENOTDIR;
                }
                if (ret == -ENOENT) {
                        KASSERT(NULL != dir->vn_ops->mknod);
                        dbg(DBG_PRINT, "(GRADING2A 3.b)\n");

                        ret = dir->vn_ops->mknod(dir, name, namelen, mode, devid);
                        vput(dir);
                        return ret;
                }
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        vput(dir);
        vput(result);
        dbg(DBG_PRINT, "(GRADING2D)\n");
        return -EEXIST;
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
        size_t namelen;
        const char *name = NULL;
        vnode_t *dir, *result;
        int ret;

        dir = NULL;
        result = NULL;

        ret = dir_namev(path, &namelen, &name, NULL, &dir);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        if (namelen == 0) {
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EEXIST;
        }
        ret = lookup(dir, name, namelen, &result);
        if (ret < 0) {
                if (ret == -ENOTDIR || dir->vn_ops->mkdir == NULL || !S_ISDIR(dir->vn_mode)){
                        vput(dir);
                        dbg(DBG_PRINT, "(GRADING2D)\n");
                        return -ENOTDIR;
                }
                if (ret == -ENOENT) {
                        KASSERT(NULL != dir->vn_ops->mkdir);
                        dbg(DBG_PRINT, "(GRADING2A 3.c)\n");

                        ret = dir->vn_ops->mkdir(dir, name, namelen);
                        vput(dir);
                        return ret;
                }
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        vput(dir);
        vput(result);
        dbg(DBG_PRINT, "(GRADING2D)\n");
        return -EEXIST;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_rmdir(const char *path)
{
        size_t namelen;
        const char *name = NULL;
        vnode_t *dir;
        int ret;

        dir = NULL;

        ret = dir_namev(path, &namelen, &name, NULL, &dir);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }

        if (namelen == 1 && name[0] == '.') {
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EINVAL;
        }

        if (namelen == 2 && name[0] == '.' && name[1] == '.') {
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -ENOTEMPTY;
        }

        if (dir->vn_ops->rmdir == NULL || !S_ISDIR(dir->vn_mode)) {
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -ENOTDIR;
                 
        }
        KASSERT(NULL != dir->vn_ops->rmdir);
        dbg(DBG_PRINT, "(GRADING2A 3.d)\n");

        ret = dir->vn_ops->rmdir(dir, name, namelen);
        vput(dir);
        return ret;
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
        size_t namelen;
        const char *name = NULL;
        vnode_t *dir, *result;
        int ret;

        dir = NULL;
        result = NULL;

        ret = dir_namev(path, &namelen, &name, NULL, &dir);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        ret = lookup(dir, name, namelen, &result);

        if (ret < 0) {
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        if (S_ISDIR(result->vn_mode)) {
                vput(dir);
                vput(result);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EISDIR;
        }
        KASSERT(NULL != dir->vn_ops->unlink);
        dbg(DBG_PRINT, "(GRADING2A 3.e)\n");

        ret = dir->vn_ops->unlink(dir, name, namelen);
        vput(dir);
        vput(result);
        return ret;
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 *      o EISDIR
 *        from is a directory.
 */
int
do_link(const char *from, const char *to)
{
        size_t namelen;
        const char *name = NULL;
        vnode_t *from_vnode, *dir, *to_vnode;
        int ret;

        from_vnode = NULL;
        dir = NULL;
        to_vnode = NULL;
        
        ret = open_namev(from, 0, &from_vnode, NULL);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return ret;
        }
        if (S_ISDIR(from_vnode->vn_mode)) {
                vput(from_vnode);
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return -EISDIR;
        }
        ret = dir_namev(to, &namelen, &name, NULL, &dir);
        if (ret < 0) {
                vput(from_vnode);
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return ret;
        }
        ret = lookup(dir, name, namelen, &to_vnode);
        if (ret < 0) {
                if (ret == -ENOENT) {
                        if (NULL == dir->vn_ops->link) {
                                dbg(DBG_PRINT, "(GRADING2D)\n");
                                vput(from_vnode);
                                vput(dir);

                                return -ENOTDIR;
                        }
                        ret = dir->vn_ops->link(from_vnode, dir, name, namelen);
                        dbg(DBG_PRINT, "(GRADING2D)\n");
                        vput(from_vnode);
                        vput(dir);

                        return ret;
                }
                vput(from_vnode);
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        vput(from_vnode);
        vput(dir);
        vput(to_vnode);
        dbg(DBG_PRINT, "(GRADING2D)\n");

        return -EEXIST;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
        int ret;

        dbg(DBG_PRINT, "(GRADING2D)\n");
        ret = do_link(oldname, newname);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return ret;
        }
        return do_unlink(oldname);
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
        vnode_t *dir;
        int ret;

        ret = open_namev(path, 0, &dir, NULL);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return ret;
        }
        if (!S_ISDIR(dir->vn_mode)) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                vput(dir);

                return -ENOTDIR;
        }
        vput(curproc->p_cwd);
        curproc->p_cwd = dir;
        dbg(DBG_PRINT, "(GRADING2D)\n");

        return 0; 
}

/* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
 * If the readdir fs_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)
{
        file_t *f = NULL;
        int bytes;

        dbg(DBG_PRINT, "(GRADING2D)\n");

        if (fd == -1) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        f = fget(fd);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }

        if (!S_ISDIR(f->f_vnode->vn_mode) || f->f_vnode->vn_ops->readdir == NULL) {
                fput(f);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -ENOTDIR;
        }

        bytes = f->f_vnode->vn_ops->readdir(f->f_vnode, f->f_pos, dirp);

        fput(f);

        if (bytes == 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return 0;
        }
        dbg(DBG_PRINT, "(GRADING2D)\n");
        do_lseek(fd, bytes, SEEK_CUR);
        return sizeof(*dirp);
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
        file_t *f = NULL;

        dbg(DBG_PRINT, "(GRADING2D)\n");
        if (!(whence == SEEK_SET) && !(whence == SEEK_CUR) && !(whence == SEEK_END) ) {
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return -EINVAL;
        }

        if (fd == -1) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }
        f = fget(fd);
        if (f == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EBADF;
        }

        if (whence == SEEK_SET && offset >= 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                f->f_pos = offset;
        } else if (whence == SEEK_CUR && f->f_pos+offset >= 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                f->f_pos += offset;
        } else if (whence == SEEK_END && f->f_vnode->vn_len + offset >= 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                f->f_pos = f->f_vnode->vn_len + offset;
        } else {
                fput(f);
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return -EINVAL;
        }
        dbg(DBG_PRINT, "(GRADING2D)\n");
        fput(f);

        return f->f_pos;
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_stat(const char *path, struct stat *buf)
{
        size_t namelen;
        const char *name = NULL;
        vnode_t *dir, *result;
        int ret;

        dir = NULL;
        result = NULL;

        ret = dir_namev(path, &namelen, &name, NULL, &dir);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }
        if (namelen == 0) {
                KASSERT(NULL != dir->vn_ops->stat);
                dbg(DBG_PRINT, "(GRADING2A 3.f)\n");
                dir->vn_ops->stat(dir, buf);
                vput(dir);
                return 0;
        }
        ret = lookup(dir, name, namelen, &result);
        if (ret < 0) {
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return ret;
        }

        KASSERT(NULL != result->vn_ops->stat);
        dbg(DBG_PRINT, "(GRADING2A 3.f)\n");

        result->vn_ops->stat(result, buf);

        vput(dir);
        vput(result);

        return 0;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif
