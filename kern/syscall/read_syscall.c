#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>
#include <read_syscall.h>

int sys_read(int fd, userptr_t buf, size_t bytes_to_read, int32_t *retval)
{
	int result=-1;
	off_t initial_offset; 
	struct iovec iov;
	struct uio u;
	struct proc *current_proc;
	current_proc = curthread->t_proc;
	if(fd<0 || fd>OPEN_MAX)
		return EBADF;
	if(current_proc->p_filetable[fd]==NULL)
		return EBADF;
	lock_acquire(current_proc->p_filetable[fd]->fh_accesslock);
	if(((current_proc->p_filetable[fd]->fh_flags)&(O_RDONLY|O_WRONLY|O_RDWR))== O_WRONLY)
		return EBADF;
	initial_offset = current_proc->p_filetable[fd]->fh_offset;

	iov.iov_ubase = buf;
	iov.iov_len = bytes_to_read; 		//is this correct?
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_offset = initial_offset;
	u.uio_resid = bytes_to_read;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = proc_getas();
	result = VOP_READ(current_proc->p_filetable[fd]->fh_vnode,&u);
	if(result) {
		lock_release(current_proc->p_filetable[fd]->fh_accesslock);
		return result;
	}
	*retval = u.uio_offset - initial_offset;
	lock_release(current_proc->p_filetable[fd]->fh_accesslock);
	return result;
}
