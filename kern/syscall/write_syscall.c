#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <write_syscall.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>





int sys_write(int fd, userptr_t buf, size_t bytes_to_write, int32_t *retval)
{
	int result;
	off_t initial_offset; 
	struct iovec iov;
	struct uio u;
	struct proc *current_proc;
	current_proc = curthread->t_proc;
	if(fd<0 || fd>OPEN_MAX)
		return EBADF;
	if(current_proc->p_filetable[fd]==NULL)
		return EBADF;
	if(((current_proc->p_filetable[fd]->fh_flags)&(O_RDONLY|O_WRONLY|O_RDWR))== O_RDONLY)
		return EBADF;
	initial_offset = current_proc->p_filetable[fd]->fh_offset;

	iov.iov_ubase = buf;
	iov.iov_len = bytes_to_write; 		//is this correct?
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_offset = initial_offset;
	u.uio_resid = bytes_to_write;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = proc_getas();
	result = VOP_WRITE(current_proc->p_filetable[fd]->fh_vnode,&u);
	if(result)
		return result;
	*retval = u.uio_offset - initial_offset;
	return result;
}
