/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <spl.h>
#include <mips/tlb.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	as->segment_head = NULL;
	as->pagetable_head = NULL;
	as->pagetable_tail = NULL;
	as->heap_start = 0;
	as->heap_break = 0;
	as->max_heap = 0;
	as->total_pages = 0;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;
	struct segment * temp_segment;
	struct pte * temp_pte;
	paddr_t temp_paddr;
	int result;

	//dumbvm_can_sleep();
	
	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	//copy the segment list
	temp_segment = old->segment_head;
	while(temp_segment != NULL)
	{
		result = add_segment(temp_segment->as_vbase, temp_segment->as_npages, temp_segment->permission, &(new->segment_head));
		if(result)
			return ENOMEM;
		temp_segment = temp_segment->next;
	}

	//copy page table and pages
	temp_pte = old->pagetable_head;
	while(temp_pte != NULL)
	{
		if(temp_pte->is_valid == 1)
		{
			temp_paddr = alloc_kpages(1);
			if(temp_paddr == 0)
				return ENOMEM;
			temp_paddr = KVADDR_TO_PADDR(temp_paddr);
			result = add_pte(temp_pte->vpn, (temp_paddr>>12), temp_pte->permission, &(new->pagetable_head), &(new->pagetable_tail));
			if(result)
				return ENOMEM;
			new->total_pages++;
			memmove((void *) PADDR_TO_KVADDR(temp_paddr), (const void *)PADDR_TO_KVADDR((temp_pte->ppn)<<12), PAGE_SIZE);
			
		}
		else {
			//page is on disk? what do we do now?
		}
		temp_pte = temp_pte->next;

	}

	new->heap_start = old->heap_start;
	new->heap_break = old->heap_break;
	new->max_heap = old->max_heap;
	*ret = new;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	delete_segment_list(&(as->segment_head));
	delete_pages(&(as->pagetable_head), as);
	delete_pagetable(&(as->pagetable_head),&(as->pagetable_tail));

	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages;
	int result;
	KASSERT(as != NULL);
	KASSERT(sz > 0);


	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	result = add_segment(vaddr, npages, readable | writeable | executable, &(as->segment_head));
	if(result)
		return result;
	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	struct segment * temp_segment = as->segment_head;
	// base and npages for last segment
	vaddr_t vbase = 0;
	size_t npages = 0;
	int is_segment_found = 0;
	while(temp_segment != NULL)
	{
		is_segment_found = 1;
		temp_segment->permission = 7;
		vbase = temp_segment->as_vbase;
		npages = temp_segment->as_npages;
		temp_segment = temp_segment->next;
	}
	if(is_segment_found)
	{
		as->heap_start = vbase+npages * PAGE_SIZE;
		as->heap_break = as->heap_start;

	}
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	struct segment * temp_segment = as->segment_head;
	while(temp_segment != NULL)
	{
		temp_segment->permission = temp_segment->backup_permission;
		temp_segment = temp_segment->next;
	}
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */
	KASSERT(as!=NULL);

	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}

//////////// custom functions ///////////////
int add_segment(vaddr_t as_vbase, size_t as_npages, int permission, struct segment **head)
{
	struct segment *temp = (struct segment *)kmalloc(sizeof(struct segment));
	if(temp == NULL)
		return -1;
	struct segment *iter_segment = *head;
	temp->as_vbase = as_vbase;
	temp->as_npages = as_npages;
	temp->permission = permission;
	temp->backup_permission = permission;
	temp->next = NULL;

	if(*head == NULL)
	{
		*head = temp;
		return 0;
	}

	while(iter_segment->next != NULL)
		iter_segment=iter_segment->next;
	iter_segment->next = temp;
	return 0;
}

void delete_segment_list(struct segment **head)
{
	struct segment *iter_segment = *head;
	struct segment *temp_next;
	if(iter_segment != NULL)
	{
		temp_next = iter_segment->next;
	}
	else
		return;
	while(iter_segment != NULL)
	{
		temp_next = iter_segment->next;
		kfree(iter_segment);
        iter_segment = NULL;
		iter_segment = temp_next;
	}
    *head = NULL;

}


int add_pte(uint32_t vpn, uint32_t ppn, int permission, struct pte **head, struct pte **tail)
{
	struct pte *temp = (struct pte *) kmalloc(sizeof(struct pte));
	if (temp == NULL)
		return -1;
	temp->vpn = vpn;
	temp->ppn = ppn;
	temp->permission = permission;
	temp->next = NULL;
	temp->is_valid = 1;
	temp->is_referenced = 1;

	if(*tail == NULL)
	{
		*head = temp;
		*tail = temp;
		return 0;
	}

	(*tail)->next = temp;
	(*tail) = (*tail)->next;
	return 0;
}

void delete_pagetable(struct pte **head,struct pte **tail )
{
	struct pte *iter_pte = *head;
	struct pte *temp_next;
	if(iter_pte != NULL)
	{
		temp_next = iter_pte->next;
	}
	else
		return;
	while(iter_pte != NULL)
	{
		temp_next = iter_pte->next;
		kfree(iter_pte);
		iter_pte = temp_next;
	}
    *head = NULL;
    *tail = NULL;
}
// delete all pages from memory present in pagetable
void delete_pages(struct pte **pagetable_head, struct addrspace *as)
{
	struct pte *temp = *pagetable_head;
	while(temp != NULL)
	{
		if(temp->is_valid == 1) // if page in physical memory
		{
			free_kpages(PADDR_TO_KVADDR(temp->ppn << 12));
			as->total_pages--;
		}
		else {
			// remove physical pages
		}
		temp = temp->next;
	}
}
struct pte *
search_pagetable(struct pte **pagetable_head, uint32_t vpn)
{
	struct pte * temp_pte = *pagetable_head;
	while(temp_pte != NULL)
	{
		if(temp_pte->vpn == vpn)
		{
			return temp_pte;
		}
		temp_pte = temp_pte->next;
	}
	return NULL;

}
