#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>
#include <mips/trapframe.h>
#include <addrspace.h>
#include <lib.h>
#include <syscall.h>
#include <fork_syscall.h>

int sys_fork(struct trapframe *tf, int32_t *retval)
{
	struct trapframe *child_trapframe;
	struct proc *child_proc;
	pid_t child_pid;
	char childname[6];
	int result;
	char threadname[13];

	child_pid = proc_get_available_pid();
	if(child_pid == -1)
		return EMPROC;
	snprintf(childname, sizeof(childname), "%d", child_pid);
	child_proc = proc_create_runprogram(childname);
	child_proc->p_pid = child_pid;
	child_proc->p_ppid = curproc->p_pid;
	for(int i=0; i<__OPEN_MAX; i++)
	{
		proc_copy_file_handle(curproc->p_filetable[i],child_proc->p_filetable[i]);
	}
	child_proc->p_lastest_fd = curproc->p_lastest_fd;
	child_proc->p_exit_status = false;
	child_trapframe = kmalloc(sizeof(struct trapframe));
	*child_trapframe = *tf;
	result = as_copy(curproc->p_addrspace, &child_proc->p_addrspace);
	if(result)
		return result;
	snprintf(threadname, sizeof(threadname), "thread_%d", child_pid);
	result = thread_fork(threadname, child_proc, fork_proc_wrapper, child_trapframe, 1);
	if(result) {
		kfree(child_trapframe);
		return result;
	}
	
	*retval = child_pid;
	return 0; 
}
