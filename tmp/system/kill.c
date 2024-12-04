/* kill.c - kill */

#include <xinu.h>

local void kill_vm();

/*------------------------------------------------------------------------
 *  kill  -  Kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
syscall	kill(
	  pid32		pid		/* ID of process to kill	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	int32	i;			/* Index into descriptors	*/
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;

	mask = disable();

	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		restore(mask);
		return SYSERR;
	}

	if (--prcount <= 1) {		/* Last user process completes	*/
		xdone();
	}

	send(prptr->prparent, pid);
	for (i=0; i<3; i++) {
		close(prptr->prdesc[i]);
	}
	freestk(prptr->prstkbase, prptr->prstklen);

	switch (prptr->prstate) {
	case PR_CURR:
		prptr->prstate = PR_FREE;	/* Suicide */

		kill_vm(pid);
		resched();

	case PR_SLEEP:
	case PR_RECTIM:
		unsleep(pid);
		prptr->prstate = PR_FREE;
		break;

	case PR_WAIT:
		semtab[prptr->prsem].scount++;
		/* Fall through */

	case PR_READY:
		getitem(pid);		/* Remove from queue */
		/* Fall through */

	default:
		prptr->prstate = PR_FREE;
	}
	restore(mask);
	return OK;
}

void kill_vm(
	pid32 pid
) {
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	int32	i, j, k;			/* Index into descriptors	*/
	unsigned long base_address;
	unsigned long ffs_area_pt_base;
	char *address;
	pt_t *pt;
    pd_t *pd;
	pt_t *ffs_pt;
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;


	prptr = &proctab[pid];
	base_address = prptr->page_addr;
	address = (char *)base_address;
	pd = (pd_t *)address;
	ffs_area_pt_base = PAGE_DIR_ADDR_START+XINU_PAGES*4+PAGE_SIZE;

	
	cr3_read_val = read_cr3();
	cr3_write_val = (PAGE_DIR_ADDR_START & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);

	if(base_address != PAGE_DIR_ADDR_START){
		for(i=0; i<1024; i++) {
			if(pd[i].pd_pres==1) {
				if(i >= XINU_PAGES/MAX_PT_SIZE) {
					base_address = pd[i].pd_base << 12;
					address = (char *)base_address;
					pt = (pt_t *)address;

					for(j=0; j<1024; j++) {
						if(pt[j].pt_pres == 1) {
							// iterate through the PTEs pointing to the FFS area and set valid of 
							// matching base to 0

							address = (char *)ffs_area_pt_base;
							ffs_pt = (pt_t *)address;
							for(k=0; k<MAX_FFS_SIZE; k++) {
								if(ffs_pt[k].pt_base == pt[j].pt_base) {
									ffs_pt[k].pt_avail = 2;
								}
							}
						}
						pt[j].pt_pres = 0;
						pt[j].pt_write = 0;
						pt[j].pt_user = 0;
						pt[j].pt_pwt = 0;
						pt[j].pt_pcd = 0;
						pt[j].pt_acc = 0;
						pt[j].pt_dirty = 0;
						pt[j].pt_mbz = 0;
						pt[j].pt_global = 0;
						pt[j].pt_avail = 0;
						pt[j].pt_base = 0;
					}
				}
			}
			pd[i].pd_pres = 0;
			pd[i].pd_write = 0;
			pd[i].pd_user = 0;
			pd[i].pd_pwt = 0;
			pd[i].pd_pcd = 0;
			pd[i].pd_acc = 0;
			pd[i].pd_mbz = 0;
			pd[i].pd_fmb = 0;
			pd[i].pd_global = 0;
			pd[i].pd_avail = 0;
			pd[i].pd_base = 0;
		}
	}
}
