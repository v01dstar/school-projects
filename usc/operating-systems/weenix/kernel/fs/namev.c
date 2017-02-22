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
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        int ret;

        KASSERT(NULL != dir);
        KASSERT(NULL != name);
        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

        if(dir->vn_ops->lookup==NULL) {
                dbg(DBG_PRINT,"(GRADING2D)\n");
                return -ENOTDIR;
        }

        if (len == 0) {
                dbg(DBG_PRINT,"(GRADING2D)\n");
                return -EINVAL;
        }
        if (len == 1 && name[0] == '.') {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                *result = dir;
                vref(*result);
                return 0;
        }

        if (len > STR_MAX) {
                dbg(DBG_PRINT,"(GRADING2D)\n");
                return -ENAMETOOLONG;
        }

        ret = dir->vn_ops->lookup(dir, name, len, result);
        if (ret < 0) {
                dbg(DBG_PRINT, "(GRADING2D)\n");

                return ret;
        }
        dbg(DBG_PRINT, "(GRADING2D)\n");

        return 0;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
        vnode_t *dir = NULL;
        int index, len, ret;

        KASSERT(NULL != pathname);
        KASSERT(NULL != namelen);
        KASSERT(NULL != name);
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        index = 0;
        if (pathname[0] == '\0') {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                return -EINVAL;
        }
        if (pathname[0] == '/') {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                dir = vfs_root_vn;
                index ++;
        } else if (base == NULL) {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                dir = curproc->p_cwd;
        } else {
                dbg(DBG_PRINT, "(GRADING2D)\n");
                dir = base;
        }

        *res_vnode = dir;
        vref(dir);
        *namelen = 0;
        *name = &pathname[index];
        while (pathname[index] != '\0') {
                *namelen = *namelen + 1;
                index ++;
                if (pathname[index] == '/') {
                        ret = lookup(dir, *name, *namelen, res_vnode);
                        if (ret < 0) {
                                vput(dir);
                                dbg(DBG_PRINT, "(GRADING2D)\n");
                                return ret;
                        }

                        KASSERT(NULL != *res_vnode);
                        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

                        while(pathname[++index] == '/');
                        vput(dir);
                        dir = *res_vnode;
                        *name = &pathname[index];
                        *namelen = 0;
                }
        } 

        return 0;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        int ret;
        size_t len;
        const char *name = NULL;
        vnode_t *dir = NULL;

        ret = dir_namev(pathname, &len, &name, base, &dir);
        if (ret >= 0) {
                ret = lookup(dir, name, len, res_vnode);
                if ((ret == -ENOENT)&&(flag & O_CREAT)) {
                        KASSERT(NULL != dir->vn_ops->create);
                        dbg(DBG_PRINT, "(GRADING2A 2.c)\n");

                        ret = dir->vn_ops->create(dir, name, len, res_vnode);
                }
                vput(dir);
                dbg(DBG_PRINT, "(GRADING2D)\n");
        }
        dbg(DBG_PRINT, "(GRADING2D)\n");
        return ret;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
