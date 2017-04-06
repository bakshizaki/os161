#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define DUMBVM_STACKPAGES    18

static struct coremap_entry * coremap;
static struct spinlock coremap_spinlock = SPINLOCK_INITIALIZER;
unsigned int latest_page;
unsigned int npages_allocated;
unsigned int nentries_coremap;

void
vm_bootstrap(void)
{
	/* Do nothing. */
}

void
coremap_bootstrap() {
	unsigned nfixed_entries;
	unsigned fixed_size;
	paddr_t fixedspace_lastaddr;

	unsigned nentries = ram_getsize() / PAGE_SIZE;
	nentries_coremap = nentries;
	unsigned avail_size = (nentries * sizeof(struct coremap_entry));
	unsigned npages = (avail_size / PAGE_SIZE) + ((avail_size % PAGE_SIZE > 0) ? 1 : 0);
   
	coremap = (struct coremap_entry *) PADDR_TO_KVADDR(ram_stealmem(npages));
	spinlock_acquire(&coremap_spinlock);

	fixedspace_lastaddr = ram_getfirstfree();
	fixed_size = (fixedspace_lastaddr / PAGE_SIZE);
    nfixed_entries = fixed_size + ((fixed_size % PAGE_SIZE > 0) ? 1 : 0);

    unsigned int i;
    npages_allocated = nfixed_entries;
    for(i = 0; i < nfixed_entries; i++) {
    	coremap[i].is_allocated = 1;
    }

    latest_page = nfixed_entries;

	spinlock_release(&coremap_spinlock);
}

static
void
dumbvm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

paddr_t
alloc_one_page() {
	
	paddr_t pa;
	unsigned int init_index = latest_page;
	unsigned int temp_index = latest_page;
	unsigned nentries = nentries_coremap;

	temp_index = (temp_index + 1) % nentries;
	while(coremap[temp_index].is_allocated == 1){
		temp_index = (temp_index + 1) % nentries;
		if(init_index == temp_index) 
		{
			// Check return value
			return 0;
		}
	}

	pa = temp_index;
	coremap[temp_index].is_allocated = 1;
	coremap[temp_index].is_last_page = 1;
	latest_page = temp_index;
	npages_allocated++;
	return pa * PAGE_SIZE; 
}

paddr_t
alloc_mul_page(unsigned npages) {
	
	paddr_t pa;
	int count;
	unsigned int init_index = latest_page;
	unsigned int temp_index = latest_page;
	unsigned nentries = nentries_coremap;
	unsigned int i;


	while(1) {

		temp_index = (temp_index + 1) % nentries;
		if(latest_page == temp_index) 
		{
			// Check return value
			return 0;
		}
		while(coremap[temp_index].is_allocated == 1){
			temp_index = (temp_index + 1) % nentries;
			if(latest_page == temp_index) 
			{
				// Check return value
				return 0;
			}
		}
		init_index = temp_index;
		count = npages;
		while(count > 0 && coremap[temp_index].is_allocated == 0 && temp_index < nentries) {
			count--;

			if(count == 0) {
				for(i = init_index; i<temp_index ; i++) {
					coremap[i].is_allocated = 1;
				}
				coremap[i].is_allocated = 1;
				coremap[i].is_last_page = 1;
				pa = init_index;
				latest_page = temp_index;
				npages_allocated += npages;
				return pa * PAGE_SIZE;
			}

			temp_index++;
		}	
	}

}

static
paddr_t
getppages(unsigned long npages) {
	
	paddr_t addr;
	spinlock_acquire(&coremap_spinlock);
	if(npages == 1) {
		addr = alloc_one_page();
	} 
	else {
		addr = alloc_mul_page(npages);
	}
	spinlock_release(&coremap_spinlock);

	return addr;
}

vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;

	dumbvm_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
		unsigned int page_index;
		/* nothing - leak the memory. */
		paddr_t paddr = addr - MIPS_KSEG0;
		page_index = paddr / PAGE_SIZE;
		unsigned nentries = nentries_coremap;
		spinlock_acquire(&coremap_spinlock);
		while(coremap[page_index].is_last_page != 1)
		{
			coremap[page_index].is_allocated = 0;
			page_index = (page_index + 1) % nentries;
			npages_allocated--;
		}
		coremap[page_index].is_allocated = 0;
		coremap[page_index].is_last_page = 0;
		npages_allocated--;
		spinlock_release(&coremap_spinlock);
}

unsigned
int
coremap_used_bytes() {
	return npages_allocated * PAGE_SIZE;
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	(void) faulttype;
	(void) faultaddress;
	return 0;
}

