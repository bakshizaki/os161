#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <close_syscall.h>

int sys_close(int fd, int32_t *retval)
{
	if(fd<0 || fd>OPEN_MAX)
		return EBADF;
	if(curproc->p_filetable[fd]==NULL)
		return EBADF;
	proc_destroy_file_handle(curproc, fd);
	*retval = 0;
	return 0;
}
