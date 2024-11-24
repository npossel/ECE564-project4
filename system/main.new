#include <xinu.h>

/* NOTE: set QUANTUM to 10ms */

//#define TEST1
//#define TEST2
//#define TEST3
//#define TEST4

void sync_printf(char *fmt, ...)
{
    intmask mask = disable();
    void *arg = __builtin_apply_args();
    __builtin_apply((void*)kprintf, arg, 100);
    restore(mask);
}

void process_info(pid32 pid){
    sync_printf("P%d:: virtual pages = %d\n", pid, allocated_virtual_pages(pid));
    sync_printf("P%d:: FFS frames = %d\n", pid, used_ffs_frames(pid));
}

process empty_process(){
    process_info(currpid);
    return OK;
}

process vmalloc_process(){

    /* testing vmalloc only */

    sync_printf("P%d:: Allocating 8/4/2/8 pages...\n", currpid);
    char *ptr1 = vmalloc(8 * PAGE_SIZE);
    char *ptr2 = vmalloc(4 * PAGE_SIZE);
    char *ptr3 = vmalloc(2 * PAGE_SIZE);
    char *ptr4 = vmalloc(8 * PAGE_SIZE);

    sync_printf("P%d:: ptr1=0x%x, ptr2=0x%x, ptr3=0x%x, ptr4=0x%x\n", currpid, ptr1, ptr2, ptr3, ptr4);
    process_info(currpid);

    /* testing deallocation */

#ifndef ECE465

    sync_printf("\nP%d:: Freeing 40 pages @ ptr1 (should fail)...\n", currpid);

    syscall vf1 = vfree(ptr1, 40 * PAGE_SIZE);
    if (vf1 == SYSERR)
        sync_printf("vfree failed as expected\n");
    else
        sync_printf("vfree error not handled correctly\n");

    process_info(currpid);

#endif

    sync_printf("\nP%d:: Freeing 6 pages @ ptr2...\n", currpid);
    vfree(ptr2, 6 * PAGE_SIZE);

    process_info(currpid);

    /* testing virtual space handling (first-fit for ECE565, next-fit for ECE465) */

#ifdef ECE465
    sync_printf("\nP%d:: Allocating 8 pages...\n", currpid);
	char *ptr5 = vmalloc(8 * PAGE_SIZE);
#else
    sync_printf("\nP%d:: Allocating 5 pages...\n", currpid);
    char *ptr5 = vmalloc(5 * PAGE_SIZE);
#endif
    sync_printf("P%d:: Allocating 8 pages...\n", currpid);
    char *ptr6 = vmalloc(8 * PAGE_SIZE);

    sync_printf("P%d:: ptr1=0x%x, ptr4=0x%x, ptr5=0x%x, ptr6=0x%x\n", currpid, ptr1, ptr4, ptr5, ptr6);
    process_info(currpid);

    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);

    /* testing FFS allocation */
    sync_printf("P%d:: Accessing 1 page @ ptr1...\n", currpid);
    ptr1[0]=0;
    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);
    sync_printf("P%d:: Accessing again 1 page @ ptr1...\n", currpid);
    ptr1[4]=0;
    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);
    sync_printf("P%d:: Accessing 2nd page from ptr4...\n", currpid);
    ptr4[PAGE_SIZE]=0;
    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);
    process_info(currpid);

    /* testing segmentation fault */
    sync_printf("\nP%d:: Testing segmentation fault...\n", currpid);
    ptr6[8*PAGE_SIZE]=0;

    sync_printf("P%d :: ERROR: process should already be killed!", currpid);

    return OK;
}

process vmalloc_process2(uint32 numPages, bool8 debug){

    uint32 i = 0;

    /* testing vmalloc only */

    if (debug){
        process_info(currpid);

        kprintf("\nP%d:: Making 3 allocations, %d pages each...\n", currpid, numPages);
    }

    char *ptr1 = vmalloc(numPages * PAGE_SIZE);
    char *ptr2 = vmalloc(numPages * PAGE_SIZE);
    char *ptr3 = vmalloc(numPages * PAGE_SIZE);

    if (ptr1==(char *)SYSERR || ptr2==(char *)SYSERR || ptr3==(char *)SYSERR){
        sync_printf("P%d:: allocation failed!\n");
        exit();
    }else if (debug){
        sync_printf("P%d:: ptr1=0x%x, ptr2=0x%x, ptr3=0x%x\n", currpid, ptr1, ptr2, ptr3);
        process_info(currpid);
    }

    /* testing FFS allocation */
    if (debug) kprintf("\nP%d:: Initializing %d pages, 2 elements per page...\n", currpid, numPages/2);
    for (i=0; i<numPages/2; i++){
        ptr1[i*PAGE_SIZE]=i%128;
        ptr1[i*PAGE_SIZE+1]=i%128;
    }

    if (debug) process_info(currpid);

    if (debug) kprintf("\nP%d:: checking the values written in the %d pages...\n", currpid, numPages/2);
    for (i=0; i<numPages/2; i++){
        if (ptr1[i*PAGE_SIZE]!=i%128 || ptr1[i*PAGE_SIZE+1]!=i%128){
            sync_printf("P%d:: ERROR - read incorrect data from page %d!\n",currpid, i);
        }
    }

    process_info(currpid);

    sleepms(200); // waiting so that main can see FFS taken

    return OK;
}

process	main(void)
{

    uint32 i = 0;

    sync_printf("\n\nTESTS START NOW...\n");
    sync_printf("-------------------\n\n");

    /* After initialization */
    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);

#ifdef TEST1

    /* TEST1: 2 processes, no vheap allocation */

    sync_printf("[TEST 1] P%d:: Spawning 2 processes that do not perform any vheap allocations...\n\n", currpid);

    resume(vcreate((void *)empty_process, INITSTK, 1, "p1", 0));
    sleepms(1000);
    resume(vcreate((void *)empty_process, INITSTK, 1, "p2", 0));

    receive();
    receive();

    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);

#endif

#ifdef TEST2

    /* TEST2: 1 process with small allocations */
    sync_printf("[TEST 2] P%d:: Spawning 1 process that performs small allocations...\n\n", currpid);
    resume(vcreate((void *)vmalloc_process, INITSTK, 1, "small", 0));

    receive();
    sleepms(100);

    sync_printf("\nP%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);

#endif

#ifdef TEST3

    /* TEST3: 1 process with allocations requiring multiple pages of PT */
    sync_printf("[TEST 3] P%d:: Spawning 1 process that performs large allocations...\n\n", currpid);
    resume(vcreate((void *)vmalloc_process2, INITSTK, 1, "large", 2, 4*1024, TRUE));

    receive();
    sleepms(100);

    sync_printf("\nP%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);

#endif

#ifdef TEST4

    /* TEST4: 10 concurrent processes that perform small allocations */
    sync_printf("[TEST 4] P%d:: Spawning 10 concurrent processes (interleaving can change from run to run)...\n\n", currpid);
    for (i=0; i<10; i++){
        resume(vcreate((void *)vmalloc_process2, INITSTK, 1, "p", 2, 80, FALSE));
    }

    sleepms(100);

    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);

    sync_printf("P%d:: Letting the processes terminate...\n\n");
    for (i=0; i<10; i++){
        receive();
    }

    sleepms(100);

    sync_printf("P%d:: Free FFS pages = %d out of %d\n\n", currpid, free_ffs_pages(), MAX_FFS_SIZE);

#endif

    return OK;
}

