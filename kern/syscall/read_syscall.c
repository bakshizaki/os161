#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>

int sys_read(int fd, userptr_t buf, size_t bytes_to_read, int32_t *retval)
{
	int result=-1;

}
