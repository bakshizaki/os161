#include <types.h>
#include <current.h>
#include <proc.h>
#include <getpid_syscall.h>

int sys_getpid(int32_t *retval)
{
	struct proc *current_proc;
	current_proc = curthread->t_proc;
	*retval = (int32_t)(current_proc->p_pid);
	return 0;
}
