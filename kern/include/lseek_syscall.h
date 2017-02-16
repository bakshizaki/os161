#ifndef _LSEEK_SYSCALL_
#define _LSEEK_SYSCALL_

int sys_lseek(int fd, off_t pos, int whence, int32_t *retval_high, int32_t *retval_low);

#endif
