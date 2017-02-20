#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>
#include <copyinout.h>
#include <stat.h>
#include <open_syscall.h>

int sys_open(userptr_t filename, int flags, mode_t mode, int32_t *retval)
{
	int fd,result;
	struct vnode *v;
	off_t offset=0;
	char kfilename[__NAME_MAX];  //is this correct?
	size_t got;
	struct stat statbuf;

	if((flags & O_RDONLY) && (flags & O_WRONLY))
	{
		return EINVAL;
	}

	result = copyinstr(filename,kfilename,__NAME_MAX,&got);
	if(result)
		return result;

	//This whole block is going to need some protection from concurrent access
	fd = proc_get_available_fd(curproc);
	if(fd == -1)
		return EMFILE;

	result = vfs_open(kfilename, flags, mode, &v);
	if(result)
		return result;

	if(flags & O_APPEND) {
		result = VOP_STAT(v,&statbuf);
		if(result)
			return result;
		offset = statbuf.st_size;
	}
	result = proc_create_file_handle(curproc,fd,v,offset,flags); 
	if(result)
		return result;
	curproc->p_lastest_fd = fd;
	*retval = fd;
	return 0;
}
