#include <types.h>
#include <kern/errno.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>
#include <copyinout.h>
#include <chdir_syscall.h>

int sys_chdir(userptr_t pathname, int32_t *retval)
{
	char kpathname[__NAME_MAX];  //is this correct?
	int result;
	size_t got;
	if(pathname == NULL)
		return ENODEV;
	
	result = copyinstr(pathname, kpathname, __NAME_MAX, &got);
	if(result)
		return result;

	result = vfs_chdir(kpathname);
	if(result)
		return result;
	*retval = 0;
	return result;
}
