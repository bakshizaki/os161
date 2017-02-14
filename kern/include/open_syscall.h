#ifndef _OPEN_SYSCALL_
#define _OPEN_SYSCALL_

int sys_open(userptr_t filename, int flags, mode_t mode, int32_t *retval);

#endif

