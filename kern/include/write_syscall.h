#ifndef _WRITE_SYSCALL_
#define _WRITE_SYSCALL_


int sys_write(int fd, userptr_t buf, size_t bytes_to_write, int32_t *retval);


#endif
