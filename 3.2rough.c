struct segment {
	vaddr_t as_vbase;
	size_t as_npages;
	int permission;
	int backup_permission;
	struct semgent *next;

}

void add_segment(vaddr_t as_vbase, size_t as_npages, int permission, struct segment **head)
{
	struct segment *temp = (struct segment *)kmalloc(sizeof(struct segment));
	struct segment *iter_segment = *head;
	temp->as_vbase = as_vbase;
	temp->as_npages = as_npages;
	temp->permission = permission;
	temp->backup_permission = permission;
	temp->next = NULL;

	if(*head == NULL)
	{
		*head = temp;
		return;
	}

	while(iter_segment->next != NULL)
		iter_segment=iter_segment->next;
	iter_segment->next = temp;
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

struct pte {
	uint32_t vpn;
	uint32_t ppn;
	int permission;
	bool is_valid;		//if 1 then in memory else in disk
	bool is_referenced;
	struct pte *next;
}

void add_pte(uint32_t vpn, uint32_t ppn, int permission, struct pte **head, struct pte **tail)
{
	struct pte *temp = (struct pte *) kmalloc(sizeof(struct pte));
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
		return;
	}

	(*tail)->next = temp;
	(*tail) = (*tail)->next;
	return;
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
		free(iter_pte);
		iter_pte = temp_next;
	}
    *head = NULL;
    *tail = NULL;
}
struct addrspace {
	struct segment * segment_head;
	struct pte * pagetable_head;
	struct pte * pagetable_tail;
	paddr_t heap_start;
	paddr_t heap_break;
	paddr_t max_heap;
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->segment_head = NULL;
	as->agetable_head = NULL;
	as->pagetable_tail = NULL;
	as->heap_start = 0;
	as->heap_break = 0;
	as->max_heap = 0;
}

// delete all pages from memory present in pagetable
void delete_pages(struct pte *pagetable_head)
{
	struct pte *temp = pagetable_head;
	while(temp != NULL)
	{
		if(temp->is_valid == 1) // if page in physical memory
		{
			free_kpages(PADDR_TO_KVADDR(temp->ppn << 12));
		}
		else {
			// remove physical pages
		}
		temp = temp->next;
	}
}

void
as_destroy(struct addrspace *as)
{
	delete_segment_list(&as->segment_head);
	delete_pages(&as->pagetable_head);
	delete_pagetable(&as->pagetable_head);
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
	/* nothing */
}


int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages;

	dumbvm_can_sleep();

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	add_segment(vaddr, npages, readable | writeable | executable, &(as->segment_head));
	return 0;

}

int as_prepare_load(struct addrspace *as)
{
	struct segment * temp_segment = as->segment_head;
	while(temp_segment != NULL)
	{
		temp_segment->permission = 7;
		temp_segment = temp_segment->next;
	}
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
}

static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}


int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}


int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;
	struct segment * temp_segment;
	struct pte * temp_pte;
	paddr_t temp_paddr;

	//dumbvm_can_sleep();
	
	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	//copy the segment list
	temp_segment = old->segment_head;
	while(temp_segment != NULL)
	{
		add_segment(temp->as_vbase, temp->as_npages, temp->permission, &(new->segment_head));
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
			add_pte(temp_pte->vpn, (temp_paddr>>12), temp_pte->permission, &(new->pagetable_head), &(new->pagetable_tail));
			memmove((void *) PADDR_TO_KVADDR(temp_paddr), (const void *)PADDR_TO_KVADDR((temp_pte->ppn)<<12))
			
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

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	struct segment * temp_segment;
	struct pte * temp_pte;
	int spl;
	int is_segment_found = 0;
	int page_permission;
	uint32_t vpn, ppn;
	

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
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
	if(is_segment_found == 0)
		return EFAULT;
	
	//now check permission
	if(faulttype == VM_FAULT_READ && (page_permission & PF_R == 0))
		return EFAULT;
	if(faulttype == VM_FAULT_WRITE && (page_permission & PF_W == 0))
		return EFAULT;

	// now look for vpn in pagetable
	vpn = faultaddress >> 12;
	temp_pte = search_pagetable(&(as->pagetable_head), vpn);

	// if we find vpn in pagetable
	if(temp_pte != NULL)
	{
		//check permission again? not sure if this is necessary
		if(faulttype == VM_FAULT_READ && (temp_pte->permission & PF_R == 0))
			return EFAULT;
		if(faulttype == VM_FAULT_WRITE && (temp_pte->permission & PF_W == 0))
			return EFAULT;

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
			paddr = alloc_kpages(1);
			if(paddr == 0)
				return ENOMEM;
			paddr = KVADDR_TO_PADDR(paddr);
			paddr = paddr & PAGE_FRAME;
			add_pte(vpn, (paddr>>12), page_permission, &(as->pagetable_head), &(as->pagetable_tail));
			//TODO: bzero
		
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
		if(page_permission & PF_W == PF_W)
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		else
			elo = paddr | TLBLO_VALID;

		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	// if no empty entry found then just overwrite a random entry
	ehi = faultaddress;
	if(page_permission & PF_W == PF_W)
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	else
		elo = paddr | TLBLO_VALID;
	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	tlb_random(ehi, elo);
	splx(spl);
	return 0;
}
//////////////// sbrk //////////////////

int sys_sbrk(intptr_t amount, int32_t *retval)
{
	vaddr_t heap_start = proc_getas()->heap_start;
	vaddr_t heap_break = proc_getas()->heap_break;
	vaddr_t max_heap = proc_getas()->max_heap;
	vaddr_t temp_heap_break = heap_break;
	if(amount & PAGE_FRAME != amount)
		return EINVAL;
	temp_heap_break = temp_heap_break + amount;
	if(temp_heap_break < heap_start)
		return EINVAL;
	if(temp_heap_break>= max_heap)
		return ENOMEM;
	*retval = heap_break;
	proc_getas()->heap_break = temp_heap_break;
	return 0;
}
