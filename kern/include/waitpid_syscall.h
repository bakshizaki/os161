#ifndef _WAITPID_SYSCALL_
#define _WAITPID_SYSCALL_

int sys_waitpid(pid_t pid, userptr_t status, int options, int32_t *retval);

#endif
