#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <waitpid_syscall.h>
#include <copyinout.h>

int sys_waitpid(pid_t pid, userptr_t status, int options, int32_t *retval)
{
	int result;
	struct proc *child_proc;	
	if(options != 0)
		return EINVAL;
	if(pid<2 || pid>__PID_MAX)
		return ESRCH;
	if(proc_table[pid] == NULL)
		return ESRCH;
	if(proc_table[pid]->p_ppid != curproc->p_pid)
		return ECHILD;

	child_proc = proc_table[pid];
	lock_acquire(curproc->p_cv_lock);
	while(child_proc->p_exit_status == false)
		cv_wait(curproc->p_cv, curproc->p_cv_lock);
	lock_release(curproc->p_cv_lock);
	if(status == NULL)
	{
		*retval = pid;
		return 0;
	}
	result = copyout(&(child_proc->p_exit_code), status, sizeof(child_proc->p_exit_code));
	if(result)
		return EFAULT;
	*retval = pid;
	proc_destroy(child_proc);
	proc_table[pid] = NULL;
	return 0;
}

