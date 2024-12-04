#include <xinu.h>

uint32 free_ffs_pages(){
    unsigned long base_address;
	struct	procent *prptr;		/* Ptr to process's table entry	*/
    uint32 i;
    uint32 free;
    pt_t *pt;
    char *address;
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;
	intmask	mask;

    // set CR3 to the system PD
    mask = disable();
	cr3_read_val = read_cr3();
	cr3_write_val = (PAGE_DIR_ADDR_START & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);

    base_address = PAGE_DIR_ADDR_START+XINU_PAGES*4+PAGE_SIZE*1;
    address = (char *)base_address;
    pt = (pt_t *)address;
    free = 0;
    for(i=0; i<MAX_FFS_SIZE; i++) {
        if(pt[i].pt_avail & 1) {}
        else {
            free++;
        }
    }

    prptr = &proctab[currpid];
	cr3_read_val = read_cr3();
	cr3_write_val = (prptr->page_addr & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);
	restore(mask);

    return free;
}

uint32 free_swap_pages(){
    return 0;
}

uint32 allocated_virtual_pages(pid32 pid){
    struct procent *prptr;
    unsigned long base_address;
    char *address;
    pt_t *pt;
    pd_t *pd;
    uint32 i, j;
    uint32 allocated;
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;
	intmask	mask;

    // set CR3 to the system PD
    mask = disable();
	cr3_read_val = read_cr3();
	cr3_write_val = (PAGE_DIR_ADDR_START & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);

    allocated = 0;
    prptr = &proctab[pid];
    base_address = prptr->page_addr;
    address = (char *)base_address;
    pd = (pd_t *)address;

    for(i=0; i<1024; i++) {
        if(pd[i].pd_pres==1) {
            base_address = pd[i].pd_base << 12;
            address = (char *)base_address;
            pt = (pt_t *)address;

            for(j=0; j<1024; j++) {
                if(pt[j].pt_avail & 1) {
                    allocated++;
                }
            }
        }
    }

    prptr = &proctab[currpid];
	cr3_read_val = read_cr3();
	cr3_write_val = (prptr->page_addr & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);
	restore(mask);
    
    return allocated;
}

uint32 used_ffs_frames(pid32 pid){
    struct procent *prptr;
    unsigned long base_address;
    char *address;
    pt_t *pt;
    pd_t *pd;
    uint32 i, j, start;
    uint32 allocated;
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;
	intmask	mask;

    // set CR3 to the system PD
    mask = disable();
	cr3_read_val = read_cr3();
	cr3_write_val = (PAGE_DIR_ADDR_START & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);

    allocated = 0;
    prptr = &proctab[pid];
    base_address = prptr->page_addr;
    address = (char *)base_address;
    pd = (pd_t *)address;
    start = XINU_PAGES/MAX_PT_SIZE;

    for(i=start; i<1024; i++) {
        if(pd[i].pd_pres==1) {
            base_address = pd[i].pd_base << 12;
            address = (char *)base_address;
            pt = (pt_t *)address;

            for(j=0; j<1024; j++) {
                if(pt[j].pt_pres == 1) {
                    allocated++;
                }
            }
        }
    }

    prptr = &proctab[currpid];
	cr3_read_val = read_cr3();
	cr3_write_val = (prptr->page_addr & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);
    restore(mask);

    return allocated;
}

