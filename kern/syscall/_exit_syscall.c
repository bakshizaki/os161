#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <kern/wait.h>
#include <synch.h>
#include <_exit_syscall.h>

int sys__exit(int exitcode)
{
	struct proc *parent_proc;
	struct proc *current_proc;
	curproc->p_exit_code = _MKWAIT_EXIT(exitcode);
	current_proc = curthread->t_proc;
	
	parent_proc = proc_table[curproc->p_ppid];
	proc_remthread(curthread);
	// TODO: should i detect if parent is not present then delete the process here itself
	if(parent_proc!=NULL)
	{
		lock_acquire(parent_proc->p_cv_lock);
		current_proc->p_exit_status = true;
		cv_signal(parent_proc->p_cv, parent_proc->p_cv_lock);
		lock_release(parent_proc->p_cv_lock);
	}
	current_proc->p_exit_status = true;
	thread_exit();
}
