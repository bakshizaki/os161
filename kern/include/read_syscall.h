#ifndef _READ_SYSCALL_
#define _READ_SYSCALL_

int sys_read(int fd, userptr_t buf, size_t bytes_to_read, int32_t *retval);

#endif
