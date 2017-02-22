#ifndef __GETCWD_SYSCALL_
#define __GETCWD_SYSCALL_

int sys___getcwd(userptr_t buf, size_t bytes_to_read, int32_t *retval);

#endif
