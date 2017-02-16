#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>
#include <stat.h>
#include <kern/seek.h>
#include <lseek_syscall.h>

int sys_lseek(int fd, off_t pos, int whence, int32_t *retval_high, int32_t *retval_low)
{
	int result;
	off_t temp_offset=0;
	struct stat statbuf;
	struct proc *current_proc;
	current_proc = curthread->t_proc;
	if(fd<0 || fd>OPEN_MAX)
		return EBADF;
	if(current_proc->p_filetable[fd]==NULL)
		return EBADF;
	lock_acquire(current_proc->p_filetable[fd]->fh_accesslock);
	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
		return EINVAL;

	if(whence == SEEK_SET)
	{
		temp_offset = pos;
		if(temp_offset < 0)
			return EINVAL;
	}
	if(whence == SEEK_CUR)
	{
		temp_offset = current_proc->p_filetable[fd]->fh_offset + pos;
		if(temp_offset < 0)
			return EINVAL;
	}
	if(whence == SEEK_END)
	{
		result = VOP_STAT(current_proc->p_filetable[fd]->fh_vnode, &statbuf)	;
		if(result)
			return result;
		temp_offset = statbuf.st_size;

		temp_offset = temp_offset + pos;
		if(temp_offset < 0)
			return EINVAL;
	}
	current_proc->p_filetable[fd]->fh_offset = temp_offset;
	*retval_high = (int32_t) (temp_offset >> 32);
	*retval_low = (int32_t) temp_offset;
	lock_release(current_proc->p_filetable[fd]->fh_accesslock);
	return 0;

}

