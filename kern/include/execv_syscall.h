#ifndef _EXECV_SYSCALL_
#define _EXECV_SYSCALL_

int sys_execv(userptr_t program, userptr_t * args, int32_t *retval);

#endif
