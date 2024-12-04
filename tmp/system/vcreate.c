/* vcreate.c - vcreate*/

#include <xinu.h>

int newpid();
local unsigned long initialize_user_page_dir();

/*------------------------------------------------------------------------
 *  vcreate  -  Create a user process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32	vcreate(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr;		/* Stack address		*/
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;

	mask = disable();

	// Set CR3 to system PD
	cr3_read_val = read_cr3();
	cr3_write_val = (PAGE_DIR_ADDR_START & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);

	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) ) {
		restore(mask);
		return SYSERR;
	}

	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;
    prptr->page_addr = initialize_user_page_dir();

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	/* Initialize stack as if the process was called		*/

	*saddr = STACKMAGIC;
	savsp = (uint32)saddr;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--)	/* Machine dependent; copy args	*/
		*--saddr = *a--;	/* onto created process's stack	*/
	*--saddr = (long)INITRET;	/* Push on return address	*/

	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	*--saddr = (long)funcaddr;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--saddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
	*--saddr = 0x00000200;		/* New process runs with	*/
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;			/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = 0;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;			/* %esi */
	*--saddr = 0;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);


	// In question @212 prof mentions ending the system function (ie vcreate) with the cr3 being changed to what called said function. 
	// That being said, since the process being created is NOT calling this function, I can only assume cr3 is set to the cr3 for main.
	// I am not sure when the cr3 should be set to the user process cr3, we will have to ask prof.
	prptr = &proctab[currpid];
	cr3_read_val = read_cr3();
	cr3_write_val = (prptr->page_addr & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);

	restore(mask);
	return pid;
}

local   unsigned long    initialize_user_page_dir(void)
{
    unsigned int base_address;
	unsigned int xinu_base_address;
	pd_t *pd;
	char *address;
	uint32 i;
	uint32 pd_entries;
    
	base_address = PAGE_DIR_ADDR_START;
	// change to not be infinite
    while(TRUE) {
        address = (char *)base_address;
		pd = (pd_t *) address;
		if(pd[0].pd_avail & (1<<1)) {
			base_address = base_address + PAGE_SIZE;
		} else {
			for(i=0; i<1024; i++) {
				pd[i].pd_pres = 0;
				pd[i].pd_write = 1;
				pd[i].pd_user = 0;
				pd[i].pd_pwt = 0;
				pd[i].pd_pcd = 0;
				pd[i].pd_acc = 0;
				pd[i].pd_mbz = 0;
				pd[i].pd_fmb = 0;
				pd[i].pd_global = 0;
				pd[i].pd_avail = 2; // set pd_avail from 000 to 010
				pd[i].pd_base = 0;
			}

			pd_entries = XINU_PAGES/1024;
			xinu_base_address = PAGE_DIR_ADDR_START;
			for(i=0; i<pd_entries; i++) {
				xinu_base_address = xinu_base_address + PAGE_SIZE;
				pd[i].pd_base = (xinu_base_address >> 12) & 0xFFFFF;
				pd[i].pd_pres = 1;
				pd[i].pd_pwt = 1;
				pd[i].pd_pcd = 1;
				pd[i].pd_acc = 1;
				pd[i].pd_avail = 3; // set pd_avail from 010 to 011
			}
			return base_address;
		}
    }
}