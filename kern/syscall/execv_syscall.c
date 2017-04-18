#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <copyinout.h>
#include <execv_syscall.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>

int sys_execv(char * program, char **args)
{
	int argc=0;
	int result;
	/*char **argv;*/
	char** temp_args = args;
	char kprogram[100];
	char *buffer;
	int *arg_size_array;
	int arg_size_array_index = 0;
	vaddr_t *arg_ptr_array;
	size_t buffer_bytes_written = 0;
	size_t prev_bytes_written=0;
	size_t got = 0;
	int padding;
	int i;
	vaddr_t entrypoint, stackptr;
	struct addrspace *oldas = NULL;
	int copyout_offset = 0;
	char null_array[4]="\0\0\0";

	if(args == NULL)
		return EFAULT;

	result = copyinstr((userptr_t)program, kprogram, sizeof(kprogram), &got);
	if(result && got==0)
		return EFAULT;
	if(result && got>0)
		return E2BIG;
	result = copyin((userptr_t)args, temp_args, sizeof(char**));
	if(result)
		return EFAULT;

	while(*temp_args)
	{
		argc++;
		/*copyinstr((userptr_t)*temp_args, temp_string, sizeof(temp_string), &got);*/
		/*kprintf("%d: %s ",argc, (char*)temp_args);*/
		/*kprintf("%d: %s %d\n",argc, temp_string, got);*/
		temp_args += 1;
	}
	buffer = (char *)kmalloc(ARG_MAX);
	if(buffer == NULL)
		return ENOMEM;
	arg_size_array = (int *)kmalloc(sizeof(int)*argc);
	if(arg_size_array == NULL)
		return ENOMEM;
	arg_ptr_array = (vaddr_t *)kmalloc(sizeof(vaddr_t)*argc);
	if(arg_ptr_array == NULL)
		return ENOMEM;

	//Now read data
	temp_args = args;
	while(*temp_args)
	{
		got = 0;
		result = copyinstr((userptr_t)*temp_args, buffer+buffer_bytes_written, ARG_MAX - buffer_bytes_written, &got);
		if(result && got==0)
			return EFAULT;
		if(result && got>0)
			return E2BIG;
		prev_bytes_written = buffer_bytes_written;
		buffer_bytes_written += got;
		if(got%4 == 0)
			padding = 0;
		else
			padding = 4 - (got%4);
		for(i=0; i<padding; i++)
		{
			buffer[buffer_bytes_written] = '\0';
			buffer_bytes_written += 1;
		}
		arg_size_array[arg_size_array_index] = buffer_bytes_written - prev_bytes_written;
		arg_size_array_index += 1;

		temp_args += 1;

	}
	/*kprintf("Array:\n");*/
	/*for(i=0; i<argc; i++)*/
		/*kprintf("%d ", arg_size_array[i]);*/

	result = krunprogram(kprogram, &entrypoint, &stackptr, &oldas);
	if(result)
		return result;

	for(i=0; i<argc; i++)
	{
		stackptr = stackptr - arg_size_array[i];
		arg_ptr_array[i] = stackptr;
		result = copyout(buffer+copyout_offset, (userptr_t)stackptr, arg_size_array[i]);
		if(result)
		{
		switch_back_oldas(oldas);
		return result;
		}
		copyout_offset += arg_size_array[i];
	}

	stackptr = stackptr - sizeof(userptr_t);
	result = copyout(null_array,(userptr_t)stackptr, sizeof(userptr_t));
	if(result)
	{
		switch_back_oldas(oldas);
		return result;
	}
	 
	for(i = argc-1; i>=0; i--)
	{
		stackptr = stackptr - sizeof(userptr_t);
		result = copyout(&arg_ptr_array[i], (userptr_t)stackptr, sizeof(userptr_t));
		if(result)
		{
		switch_back_oldas(oldas);
		return result;
		}
	}

	// destroy addrspace of old process
	as_destroy(oldas);
	kfree(buffer);
	kfree(arg_size_array);
	kfree(arg_ptr_array);


	/* Warp to user mode. */
	enter_new_process(argc /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");

	return 0;
}


int krunprogram(char *progname, vaddr_t *entrypoint, vaddr_t *stackptr, struct addrspace **oldas)
{
	struct addrspace *as;
	struct vnode *v;

	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}


	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* save old as before switching*/
	*oldas = proc_getas();

	/* Switch to it and activate it. */
	proc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		switch_back_oldas(*oldas);
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		switch_back_oldas(*oldas);
		return result;
	}
	return 0;

}

void switch_back_oldas(struct addrspace *oldas)
{
		as_destroy(proc_getas());
		proc_setas(oldas);
		as_activate();
}
