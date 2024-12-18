/* pagefault_handler.c - pagefault_handler  */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  pagefault_handler   -   Handler pagefaults caused by user processes
 *------------------------------------------------------------------------
 */
void pagefault_handler()
{
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	unsigned long v_address;
	unsigned long pd_index;
	unsigned long pt_index;
	unsigned long ffs_area_pt_base;
	char *address;
	pd_t *pd;
	pt_t *pt;
	pt_t *ffs_pt;
	uint32 i;
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;

    // set CR3 to point to system process page table
	cr3_read_val = read_cr3();
	cr3_write_val = (PAGE_DIR_ADDR_START & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);

	prptr = &proctab[currpid];

	// Grab the virtual address of the page causing the fault
	v_address = read_cr2();
	pt_index = (v_address & 0x003FF000) >> 12;
	pd_index = (v_address & 0xFFC00000) >> 22;

    // In here, present is 0. if valid is 0 (00[0]) then this is a segmentation fault.
	address = (char *)prptr->page_addr;
	pd = (pd_t *) address;

	// check if pde is valid
	if(pd[pd_index].pd_avail & 1) {

		// if yes, check if pte is valid
		address = (char *)(pd[pd_index].pd_base << 12);
		pt = (pt_t *)address;

		if(pt[pt_index].pt_avail & 1) {
			// Find a free FFS Frame.
			// Iterate through the FFS area page tables to find the first 
			ffs_area_pt_base = PAGE_DIR_ADDR_START+XINU_PAGES*4+PAGE_SIZE;
			address = (char *)ffs_area_pt_base;
			ffs_pt = (pt_t *)ffs_area_pt_base;
			for(i=0; i<MAX_FFS_SIZE; i++) {
				if(ffs_pt[i].pt_avail & 1) {
				}
				else {
					pt[pt_index].pt_base = ffs_pt[i].pt_base;
					pt[pt_index].pt_pres = 1;
					ffs_pt[i].pt_avail = 3;
					break;
				}
    		}
		}
		else {
			// SEG FAULT: kill process and print out seg fault
			kprintf("P%d:: SEGMENTATION_FAULT\n", currpid);
			kill(currpid);
		}
	}
	else {
		// SEG FAULT: kill process and print out seg fault
		kprintf("P%d:: SEGMENTATION_FAULT\n", currpid);
		kill(currpid);
	}

    // end with resetting CR3 to point to PDBR of process that was executing when page fault occured
	cr3_read_val = read_cr3();
	cr3_write_val = (prptr->page_addr & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);
}