#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <sbrk_syscall.h>
#include <vm.h>
#include <addrspace.h>

int sys_sbrk(intptr_t amount, int32_t *retval)
{
	struct addrspace * as = proc_getas();
	vaddr_t heap_start = as->heap_start;
	vaddr_t heap_break = as->heap_break;
	vaddr_t max_heap = as->max_heap;
	vaddr_t temp_heap_break = heap_break;
	vaddr_t iter_addr;
	KASSERT((heap_break & PAGE_FRAME) == heap_break);
	if((uint32_t)(amount & PAGE_FRAME) != (uint32_t)amount)
		return EINVAL;
	temp_heap_break = temp_heap_break + amount;
	if(temp_heap_break < heap_start)
		return EINVAL;
	if(temp_heap_break>= max_heap)
	{
		if(amount>0)
			return ENOMEM;
		else
			return EINVAL;
	}
	*retval = heap_break;
	if(temp_heap_break < heap_break) {
		//free all the pages from heap_break to temp_heap_break-1
		for(iter_addr = temp_heap_break; iter_addr<heap_break; iter_addr+= PAGE_SIZE)
		{
			delete_one_page(iter_addr, as);
		}
	}
	as->heap_break = temp_heap_break;
	return 0;
}
