#include <xinu.h>
#include <paging.h>

/* NOTE: 
  - set QUANTUM to 10ms 
  - the FFS and swap frame numbers in the reference output are not the physical addresses of those frames (those physical addresses depend on the memory layout chosen, and can vary from student to student). Instead, they are measured from the beginning of the FFS and swap areas, respectively. In other words, swap frame #0 is the first frame in the FFS area, FFS frame #1 is the second frame in the FFS area, SWAP frame # 0 is the first frame in the SWAP area, and so on.
  - the next frame pointer used in the Approxmate LRU algrorithm should not be reset in every test case.
  - the test case generates a long output and takes a couple/few minutes to complete.
*/

#ifdef ECE465
#define PREALLOCATED_PAGES (XINU_PAGES+MAX_PT_SIZE+MAX_FFS_SIZE)
#else
#define PREALLOCATED_PAGES XINU_PAGES
#endif

unsigned wait_flag = 0;

void sync_printf(char *fmt, ...)
{
        intmask mask = disable();
        void *arg = __builtin_apply_args();
        __builtin_apply((void*)kprintf, arg, 100);
        restore(mask);
}

void process_info(pid32 pid){
        sync_printf("[P%d] virtual pages allocated = %d\n", pid, allocated_virtual_pages(pid));
        sync_printf("[P%d] FFS frames used         = %d\n", pid, used_ffs_frames(pid));
}

void ffs_and_swap_info(){
	sync_printf("[P%d] # FFS  frames in use:: %d/%d\n", currpid, MAX_FFS_SIZE-free_ffs_pages(), MAX_FFS_SIZE);
	sync_printf("[P%d] # SWAP frames in use:: %d/%d\n", currpid, MAX_SWAP_SIZE-free_swap_pages(), MAX_SWAP_SIZE);
} 

void test(uint32 numPages, uint32 numInitPages, uint32 numReadPages, uint32 readOffset, unsigned wait){
    char *ptr = NULL;

    debug_swapping=50;
    
    sync_printf("\n===> [P%d] starting... \n", currpid);
    
    process_info(currpid);
    ffs_and_swap_info();
    if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES || used_ffs_frames(currpid)!=0){
	sync_printf("[P%d] aborting...\n", currpid); 
	return;	
    }

    sync_printf("\n===> [P%d] allocating %d pages ...\n", currpid, numPages);

    // allocate virtual heap
    ptr = vmalloc(numPages * PAGE_SIZE);

    process_info(currpid);
    ffs_and_swap_info();
    
   if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES+numPages || used_ffs_frames(currpid)!=0){
	sync_printf("[P%d] aborting...\n", currpid); 
   	return; 
   }

    if (ptr==(char *)SYSERR){
	sync_printf("[P%d] aborting...\n", currpid); 
	sync_printf("[P%d] vmalloc failed\n", currpid);
   	return; 
    }

    sync_printf("\n===> [P%d] initializing %d pages ...\n", currpid, numInitPages);
    
    uint32 i=0, j=0;

    // write data
    for(i = 0; i<numInitPages; i++){
	ptr[i*PAGE_SIZE] = 'A';
	for(j=0;j<100000;j++); //delaying for output clarity    
    }
    
    sync_printf("[P%d] %d pages initialized...\n", currpid, numInitPages);
    process_info(currpid);
    ffs_and_swap_info();
    
    if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES+numPages){
	sync_printf("[P%d] aborting...\n", currpid); 
   	return; 
    }

    // read data
    sync_printf("\n===>[P%d] reading %d pages starting from page %d ...\n", currpid, numReadPages, readOffset);
    
    char c = 0;
    for(i=readOffset; i<numReadPages+readOffset; i++){
        c =  ptr[i*PAGE_SIZE];
	c++;
	for(j=0;j<100000;j++); //delaying for output clarity    
    }
    
    wait_flag = wait;
    while(wait_flag==1){
	sleep(5);
    }
    sleep(3); //let the other process terminate

    sync_printf("\n===>[P%d] about to complete ...\n\n", currpid);
    
    process_info(currpid);
    ffs_and_swap_info();
   
    sync_printf("\n===>[P%d] returning ...\n\n", currpid);
}


/**************   MAIN *********************/
process	main(void)
{

	pid32 p1, p2;

	sync_printf("\npreallocated pages = %d\n", PREALLOCATED_PAGES);

	ffs_and_swap_info();

        sync_printf("\n================== TEST 1 ===================\n\n");

	p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, MAX_FFS_SIZE, MAX_FFS_SIZE, 0, 0);
	resume(p1);

	receive();

	ffs_and_swap_info();
        
	sync_printf("\n================== TEST 2 ===================\n\n");
	
	p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, MAX_FFS_SIZE+5, 10, 0, 0);
	resume(p1);

	receive();

	ffs_and_swap_info();

	sync_printf("\n================== TEST 3 ===================\n\n");
	
	p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, MAX_FFS_SIZE+5, 10, 0, 0);
	resume(p1);

	receive();

	ffs_and_swap_info();


	sync_printf("\n================== TEST 4 ===================\n\n");

        p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, MAX_FFS_SIZE+10, 100, 110, 0);
        resume(p1);

        receive();

        ffs_and_swap_info();

        sync_printf("\n================== TEST 5 ===================\n\n");

        p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, 2*MAX_FFS_SIZE, 20, MAX_FFS_SIZE, 0);
        resume(p1);

        receive();

        ffs_and_swap_info();

        sync_printf("\n================== TEST 6 ===================\n\n");

        p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, 2*MAX_FFS_SIZE, 20, 0, 0);
        resume(p1);

        receive();

        ffs_and_swap_info();

        sync_printf("\n================== TEST 7 ===================\n\n");

        p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, 2*MAX_FFS_SIZE, 2*MAX_FFS_SIZE, 0, 0);
        resume(p1);

        receive();

        ffs_and_swap_info();

        sync_printf("\n================== TEST 8 ===================\n\n");

        p1 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, MAX_FFS_SIZE, MAX_FFS_SIZE, 0, 1);
        resume(p1);

	sleep(5);

        p2 = vcreate(test, 2000, 50, "test", 5, 2*MAX_FFS_SIZE, 100, 100, 0, 0);
        resume(p2);

        receive();
	receive();

        ffs_and_swap_info();

	return OK;
}


