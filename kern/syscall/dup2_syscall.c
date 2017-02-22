#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <close_syscall.h>
#include <dup2_syscall.h>

int sys_dup2(int oldfd, int newfd, int32_t *retval)
{
	int result = -1;
	struct proc *current_proc;
	current_proc = curthread->t_proc;
	if(oldfd<0 || oldfd>OPEN_MAX)
		return EBADF;
	if(newfd<0 || newfd>OPEN_MAX)
		return EBADF;
	if(current_proc->p_filetable[oldfd]==NULL)
		return EBADF;

	lock_acquire(current_proc->p_filetable[oldfd]->fh_accesslock);
	if(oldfd == newfd)
	{
		*retval = newfd;
		return 0;
	}

	if(current_proc->p_filetable[newfd] != NULL)
	{
		result = sys_close(newfd, retval);
		if(result)
			return result;
	}
	current_proc->p_filetable[newfd] = current_proc->p_filetable[oldfd];
	*retval = newfd;
	return 0;
}
