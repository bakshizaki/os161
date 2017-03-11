#ifndef _EXECV_SYSCALL_
#define _EXECV_SYSCALL_

int sys_execv(char *program,char **args);
int krunprogram(char *progname, vaddr_t *entrypoint, vaddr_t *stackptr, struct addrspace *oldas);
void switch_back_oldas(struct addrspace *oldas);

#endif
