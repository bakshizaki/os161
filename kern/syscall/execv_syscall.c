#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <copyinout.h>
#include <execv_syscall.h>

int sys_execv(userptr_t program, userptr_t * args, int32_t *retval)
{
	int argc=0;
	/*char **argv;*/
	userptr_t temp_args = *args;
	char temp_string[100];
	kprintf("%s\n",(char*)program);
	kprintf("%s\n",(char*)*args);
	char *test_ptr;
	size_t got;
	(void)program;
	(void)retval;
	while(temp_args->_dummy)
	{
		argc++;
		copyinstr(temp_args, temp_string, sizeof(temp_string), &got);
		/*kprintf("%d: %s ",argc, (char*)temp_args);*/
		test_ptr = (char*)temp_args;
		while(*test_ptr != '\0')
		{
			kprintf("%d ", *test_ptr);
			test_ptr++;
		}
		kprintf("\n");
		temp_args += got+2;
		kprintf("%d: %s\n",argc, temp_string);
	}
	return 0;
}
