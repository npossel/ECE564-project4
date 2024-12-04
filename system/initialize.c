/* initialize.c - nulluser, sysinit */

/* Handle system initialization and become the null process */

#include <xinu.h>
#include <string.h>

extern	void	start(void);	/* Start of Xinu code			*/
extern	void	*_end;		/* End of Xinu code			*/

/* Function prototypes */

extern	void main(void);	/* Main is the first process created	*/
static	void sysinit(); 	/* Internal system initialization	*/
extern	void meminit(void);	/* Initializes the free memory list	*/
local	process startup(void);	/* Process to finish startup tasks	*/
local	unsigned long initialize_page_table(void);	/* Initializes the page table used by system processes	*/

/* Declarations of major kernel variables */

struct	procent	proctab[NPROC];	/* Process table			*/
struct	sentry	semtab[NSEM];	/* Semaphore table			*/
struct	memblk	memlist;	/* List of free memory blocks		*/

/* Active system status */

int	prcount;		/* Total number of live processes	*/
pid32	currpid;		/* ID of currently executing process	*/

/* Control sequence to reset the console colors and cusor positiion	*/

#define	CONSOLE_RESET	" \033[0m\033[2J\033[;H"

/*------------------------------------------------------------------------
 * nulluser - initialize the system and become the null process
 *
 * Note: execution begins here after the C run-time environment has been
 * established.  Interrupts are initially DISABLED, and must eventually
 * be enabled explicitly.  The code turns itself into the null process
 * after initialization.  Because it must always remain ready to execute,
 * the null process cannot execute code that might cause it to be
 * suspended, wait for a semaphore, put to sleep, or exit.  In
 * particular, the code must not perform I/O except for polled versions
 * such as kprintf.
 *------------------------------------------------------------------------
 */

void	nulluser()
{	
	struct	memblk	*memptr;	/* Ptr to memory block		*/
	uint32	free_mem;		/* Total amount of free memory	*/
	
	/* Initialize the system */

	sysinit();

	/* Output Xinu memory layout */
	free_mem = 0;
	for (memptr = memlist.mnext; memptr != NULL;
						memptr = memptr->mnext) {
		free_mem += memptr->mlength;
	}
	kprintf("%10d bytes of free memory.  Free list:\n", free_mem);
	for (memptr=memlist.mnext; memptr!=NULL;memptr = memptr->mnext) {
	    kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)memptr, ((uint32)memptr) + memptr->mlength - 1);
	}

	kprintf("%10d bytes of Xinu code.\n",
		(uint32)&etext - (uint32)&text);
	kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)&text, (uint32)&etext - 1);
	kprintf("%10d bytes of data.\n",
		(uint32)&ebss - (uint32)&data);
	kprintf("           [0x%08X to 0x%08X]\n\n",
		(uint32)&data, (uint32)&ebss - 1);

	/* Enable interrupts */

	enable();

	/* Initialize the network stack and start processes */

	net_init();

	/* Create a process to finish startup and start main */

	resume(create((void *)startup, INITSTK, INITPRIO,
					"Startup process", 0, NULL));

	/* Become the Null process (i.e., guarantee that the CPU has	*/
	/*  something to run when no other process is ready to execute)	*/

	while (TRUE) {
		;		/* Do nothing */
	}

}


/*------------------------------------------------------------------------
 *
 * startup  -  Finish startup takss that cannot be run from the Null
 *		  process and then create and resume the main process
 *
 *------------------------------------------------------------------------
 */
local process	startup(void)
{
	uint32	ipaddr;			/* Computer's IP address	*/
	char	str[128];		/* String used to format output	*/


	/* Use DHCP to obtain an IP address and format it */

	ipaddr = getlocalip();
	if ((int32)ipaddr == SYSERR) {
		kprintf("Cannot obtain an IP address\n");
	} else {
		/* Print the IP in dotted decimal and hex */
		ipaddr = NetData.ipucast;
		sprintf(str, "%d.%d.%d.%d",
			(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
			(ipaddr>>8)&0xff,        ipaddr&0xff);
	
		kprintf("Obtained IP address  %s   (0x%08x)\n", str,
								ipaddr);
	}

	/* Create a process to execute function main() */

	resume(create((void *)main, INITSTK, INITPRIO,
					"Main process", 0, NULL));

	/* Startup process exits at this point */

	return OK;
}


/*------------------------------------------------------------------------
 *
 * sysinit  -  Initialize all Xinu data structures and devices
 *
 *------------------------------------------------------------------------
 */
static	void	sysinit()
{
	int32	i;
	struct	procent	*prptr;		/* Ptr to process table entry	*/
	struct	sentry	*semptr;	/* Ptr to semaphore table entry	*/

	/* Reset the console */

	kprintf(CONSOLE_RESET);
	kprintf("\n%s\n\n", VERSION);

	/* Initialize the interrupt vectors */

	initevec();
	
	/* Initialize free memory list */
	
	meminit();

	/* Initialize system variables */

	/* Count the Null process as the first process in the system */

	prcount = 1;

	/* Scheduling is not currently blocked */

	Defer.ndefers = 0;

	/* Initialize process table entries free */

	for (i = 0; i < NPROC; i++) {
		prptr = &proctab[i];
		prptr->prstate = PR_FREE;
		prptr->prname[0] = NULLCH;
		prptr->prstkbase = NULL;
		prptr->prprio = 0;
	}

	/* Initialize the Null process entry */	

	prptr = &proctab[NULLPROC];
	prptr->prstate = PR_CURR;
	prptr->prprio = 0;
	strncpy(prptr->prname, "prnull", 7);
	prptr->prstkbase = getstk(NULLSTK);
	prptr->prstklen = NULLSTK;
	prptr->prstkptr = 0;
	currpid = NULLPROC;
	
	/* Initialize semaphores */

	for (i = 0; i < NSEM; i++) {
		semptr = &semtab[i];
		semptr->sstate = S_FREE;
		semptr->scount = 0;
		semptr->squeue = newqueue();
	}

	/* Initialize buffer pools */

	bufinit();

	/* Create a ready list for processes */

	readylist = newqueue();


	/* initialize the PCI bus */

	pci_init();

	/* Initialize the real time clock */

	clkinit();

	for (i = 0; i < NDEVS; i++) {
		init(i);
	}

	/* Enable paging */
	prptr->page_addr = initialize_page_table();
	enable_paging();

	/* Page fault handler */
	set_evec(14, (uint32)pagefault_handler_disp);

	return;
}

int32	stop(char *s)
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* Empty */;
}

int32	delay(int n)
{
	DELAY(n);
	return OK;
}

unsigned long 	initialize_page_table()
{
	// ended up setting this to unsigned int. Not sure if it NEEDS to be char *, but 
	// I am not able to do the bit shift if it is the char * type. Soooooo decided to
	// make it int to allow for bit shift.
	unsigned int base_address;
	unsigned int pt_base_address;
	char *address;
	unsigned long cr3;
	unsigned long cr3_read_val;
	unsigned long cr3_write_val;
	uint32 i, j;
	uint32 pd_entries;
	pd_t *pd;
	pt_t *pt;
	intmask	mask;
	struct	memblk	*memptr;

	mask = disable();

	base_address = PAGE_DIR_ADDR_START;
	address = (char *)base_address;
	// kprintf("\nbase address for PD: %x", address);
	cr3 = base_address;
	pd = (pd_t *) address;

	// initialize the page directory for system processes by setting present bits to 0
	// avail bits are as follows: [2] unused [1] does this page exist? [0] valid bit
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

	pt_base_address = 0;

	// Since PD for system processes must cover all areas, add all area sizes (in frames) and divide
	// by 1024 since each PDE points to a PT page of 1024.
	pd_entries = (XINU_PAGES+MAX_FFS_SIZE+MAX_PT_SIZE)/1024;
	// kprintf("\nNumber of PD entries to initialize: %d", pd_entries);

	// assign first XINU_AREA, FFS_AREA, and PT_AREA to PD
	for(i=0; i<pd_entries; i++) {
		base_address = base_address + PAGE_SIZE;
		pd[i].pd_base = (base_address >> 12) & 0xFFFFF;
		pd[i].pd_pres = 1;
		pd[i].pd_pwt = 1;
		pd[i].pd_pcd = 1;
		pd[i].pd_acc = 1;
		pd[i].pd_avail = 3; // set pd_avail from 010 to 011
		address = (char *)base_address;

		// kprintf("\nEntry %d stored base: %x", i, pd[i].pd_base);
		// kprintf("\nbase address for PT page: %x", address);

		pt = (pt_t *) address;

		// assign each PTE to a physical frame of the areas
		for(j=0; j<1024; j++) {
			pt[j].pt_pres = 1;
			pt[j].pt_write = 1;
			pt[j].pt_user = 0;
			pt[j].pt_pwt = 1;
			pt[j].pt_pcd = 1;
			pt[j].pt_acc = 1;
			pt[j].pt_dirty = 0;
			pt[j].pt_mbz = 0;
			pt[j].pt_global = 0;
			if(i<8) pt[j].pt_avail = 3; // set the pt_avail from 000 to 011
			else pt[j].pt_avail = 2; // set the pt_avail from 000 to 010
			pt[j].pt_base = (pt_base_address >> 12) & 0xFFFFF;

			pt_base_address = pt_base_address + PAGE_SIZE;
		}
    }

	cr3_read_val = read_cr3();
	cr3_write_val = (cr3 & 0xFFFFF000) | (cr3_read_val & 0x00000FFF);
	write_cr3(cr3_write_val);
	restore(mask);
	return cr3_write_val;
}
