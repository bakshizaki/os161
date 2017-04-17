struct segment {
	vaddr_t as_vbase;
	paddr_t as_pbase;
	size_t as_npages;
	int permission;
	int backup_permission;
	struct semgent *next;

}

void add_segment(vaddr_t as_vbase, paddr_t as_pbase, size_t as_npages, int permission, struct segment **head)
{
	struct segment *temp = (struct segment *)kmalloc(sizeof(struct segment));
	struct segment *iter_segment = *head;
	temp->as_vbase = as_vbase;
	temp->as_pbase = as_pbase;
	temp->as_npages = as_npages;
	temp->permission = permission;
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
	while(temp_next != NULL)
	{
		temp_next = iter_segment->next;
		kfree(iter_segment);
		iter_segment = temp_next;
	}

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
	*tail->next = temp;
	*tail = *tail->next;
	return;
}

void delete_pagetable(struct pte **head)
{
	struct pte *iter_pte = *head;
	struct pte *temp_next;
	if(iter_pte != NULL)
	{
		temp_next = iter_pte->next;
	}
	else
		return;
	while(temp_next != NULL)
	{
		temp_next = iter_pte->next;
		kfree(iter_pte);
		iter_pte = temp_next;
	}
}

void delete_pte(struct pte **head, struct ptr **tail) 
struct addrspace {
	struct segment * segment_head;
	struct pte * pagetable_head;
	struct pte * pagetable_tail;
	paddr_t as_stackpbase;
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
	as->as_stackpbase = 0;
	as->heap_start = 0;
	as->heap_break = 0;
	as->max_heap = 0;
}

void
as_destroy(struct addrspace *as)
{

}
