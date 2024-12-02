/* kill.c - kill */

#include <xinu.h>

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
	int32	i, j;			/* Index into descriptors	*/
	unsigned long cr3;
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;
	unsigned long base_address;
	char *address;
	pt_t *pt;
    pd_t *pd;

	kprintf("\nkilling process! %d", pid);
	mask = disable();

	cr3_read_val = read_cr3();
	cr3_read_val = cr3_read_val & 0x00000FFF;
	cr3 = PAGE_DIR_ADDR_START & 0xFFFFF000;
	cr3_write_val = cr3 | cr3_read_val;
	write_cr3(cr3_write_val);

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

	base_address = prptr->page_addr;
	address = (char *)base_address;
	pd = (pd_t *)address;
	for(i=0; i<1024; i++) {
		if(pd[i].pd_pres==1) {
			base_address = pd[i].pd_base << 12;
			address = (char *)base_address;
			pt = (pt_t *)address;

			for(j=0; j<1024; j++) {
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

	restore(mask);
	return OK;
}
