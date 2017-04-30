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
#include <elf.h>

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
//#define VM_STACKPAGES    801

/*static struct coremap_entry * coremap;*/
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
    nfixed_entries = fixed_size + ((fixedspace_lastaddr % PAGE_SIZE > 0) ? 1 : 0);

    unsigned int i;
    npages_allocated = nfixed_entries;
	//mark kernel + coremap pages allocated and fixed
    for(i = 0; i < nfixed_entries; i++) {
    	coremap[i].is_allocated = 1;
		coremap[i].is_fixed = 1;
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
	coremap[temp_index].is_fixed = 1;
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
					coremap[i].is_fixed = 1;
				}
				coremap[i].is_allocated = 1;
				coremap[i].is_last_page = 1;
				coremap[i].is_fixed = 1;
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
			coremap[page_index].is_fixed = 0;
			page_index = (page_index + 1) % nentries;
			npages_allocated--;
		}
		coremap[page_index].is_allocated = 0;
		coremap[page_index].is_fixed = 0;
		coremap[page_index].is_last_page = 0;
		npages_allocated--;
		spinlock_release(&coremap_spinlock);
}

unsigned
int
coremap_used_bytes() {
	return npages_allocated * PAGE_SIZE;
}

unsigned
int
coremap_free_space() {
	return (nentries_coremap - npages_allocated - 100) * PAGE_SIZE;
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}
static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	paddr_t paddr = 0;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	struct segment * temp_segment;
	struct pte * temp_pte;
	int spl;
	int is_segment_found = 0;
	int page_permission =0;
	uint32_t vpn, ppn;
	

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		/*panic("dumbvm: got VM_FAULT_READONLY\n");*/
		return EINVAL;
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}
	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	//check if faultaddress is present in any segment
	temp_segment = as->segment_head;
	while(temp_segment != NULL)
	{
		vaddr_t vtop;
		vaddr_t vbase;
		KASSERT(temp_segment->as_vbase != 0);
		KASSERT(temp_segment->as_npages != 0);
		KASSERT((temp_segment->as_vbase & PAGE_FRAME) == temp_segment->as_vbase);
		vbase = temp_segment->as_vbase;
		vtop = temp_segment->as_vbase + temp_segment->as_npages * PAGE_SIZE;
		if(faultaddress >= vbase && faultaddress < vtop) {
			page_permission = temp_segment->permission;
			is_segment_found = 1;
			break;
		}
		temp_segment = temp_segment->next;

	}

	// if segment not found then check in heap and stack
	if(is_segment_found == 0)
	{
		vaddr_t stackbase, stacktop;
		stackbase = USERSTACK - VM_STACKPAGES * PAGE_SIZE;
		stacktop = USERSTACK;
		if (faultaddress >= stackbase && faultaddress < stacktop) {
			is_segment_found = 1;
			page_permission = PF_R | PF_W;
		}
		else if(faultaddress >= as->heap_start && faultaddress < as->heap_break)
		{
			is_segment_found = 1;
			page_permission = PF_R | PF_W;
		}
	}
	goto looping;
looping:
	if(is_segment_found == 0)
		return EFAULT;
	
	//now check permission
	if(faulttype == VM_FAULT_READ && ((page_permission & PF_R) == 0))
		return EFAULT;
	if(faulttype == VM_FAULT_WRITE && ((page_permission & PF_W) == 0))
		return EFAULT;

	// now look for vpn in pagetable
	vpn = faultaddress >> 12;
	temp_pte = search_pagetable(&(as->pagetable_head), vpn);

	// if we find vpn in pagetable
	if(temp_pte != NULL)
	{
		//check permission again? not sure if this is necessary
		if(faulttype == VM_FAULT_READ && (( temp_pte->permission & PF_R  )== 0))
			return EFAULT;
		if(faulttype == VM_FAULT_WRITE && (( temp_pte->permission & PF_W  )== 0))
			return EFAULT;

		//pte lock acquire
		//check if page in memory
		if(temp_pte->is_valid)
		{
			ppn = temp_pte->ppn;
			paddr = ppn<<12;
		}
		else { //page on disk
			//TODO: do this later
}

	}
	else { // we did not find vpn in pagetable
			int coremap_index;
			struct pte *added_pte;
			paddr = alloc_kpages(1);
			if(paddr == 0)
				return ENOMEM;
			paddr = KVADDR_TO_PADDR(paddr);
			coremap_index = paddr / PAGE_SIZE;
			as_zero_region(paddr, 1);
			paddr = paddr & PAGE_FRAME;
			add_pte(vpn, (paddr>>12), page_permission, &(as->pagetable_head), &(as->pagetable_tail), &added_pte);
			//put pte address in coremap
			coremap[coremap_index].coremap_pte = added_pte;
			// make is fixed bit as 0
			coremap[coremap_index].is_fixed = 0;
			//TODO: acquire pte lock
			as->total_pages++;
		
	}
	
	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	//find an empty entry in TLB and put our values
	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		if(( page_permission & PF_W ) == PF_W)
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		else
			elo = paddr | TLBLO_VALID;

		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		//pte lock release
		splx(spl);
		return 0;
	}

	// if no empty entry found then just overwrite a random entry
	ehi = faultaddress;
	if((page_permission & PF_W )== PF_W)
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	else
		elo = paddr | TLBLO_VALID;
	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	tlb_random(ehi, elo);
	//pte lock release
	splx(spl);
	return 0;
}

