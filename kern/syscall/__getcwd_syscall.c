#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <vnode.h>
#include <__getcwd_syscall.h>

int sys___getcwd(userptr_t buf, size_t bytes_to_read, int32_t *retval)
{
	int result=-1;
	struct iovec iov;
	struct uio u;

	iov.iov_ubase = buf;
	iov.iov_len = bytes_to_read; 		//is this correct?
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_offset = 0;
	u.uio_resid = bytes_to_read;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = proc_getas();

	result = vfs_getcwd(&u);
	if (result) {
		return result;
	}
	*retval = u.uio_offset;
	return result;
}

