#include <xinu.h>

/* NOTE: set QUANTUM to 10ms */

/* enables debugging information */
#define DEBUG

#ifdef ECE465
#define PREALLOCATED_PAGES (XINU_PAGES+MAX_PT_SIZE+MAX_FFS_SIZE)
#else
#define PREALLOCATED_PAGES XINU_PAGES
#endif

/* comment to skip test cases */ 
// #define TEST1
// #define TEST2
#define TEST3
// #define TEST4
// #define TEST5
// #define TEST6
// #define TEST7
// #define TEST8

/* =========================================================================== */

uint32 error = 0;
uint32 done = 0;
uint32 passed = 0;
uint32 failed = 0;

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

void outcome(uint32 testcase){
    if(error || !done){
        sync_printf("\n=== Test case %d FAIL ===\n", testcase);
	failed++;
    }else{
        sync_printf("\n=== Test case %d PASS ===\n", testcase);
	passed++;
    }
}

void test(uint32 numPages, uint32 numInitPages){
    char *ptr = NULL;
    
#ifdef DEBUG
    process_info(currpid);
#endif
    if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES || used_ffs_frames(currpid)!=0){
	error = 1;
    }

#ifdef DEBUG
    sync_printf("\n[P%d] allocating %d pages ...\n", currpid, numPages);
#endif

    // allocate virtual heap
    ptr = vmalloc(numPages * PAGE_SIZE);

#ifdef DEBUG
    process_info(currpid);
#endif
    if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES+numPages || used_ffs_frames(currpid)!=0){
	error = 1;
    }

    if (ptr==(char *)SYSERR){
	sync_printf("[P%d] vmalloc failed\n", currpid);
	kill(currpid);
    }else{
#ifdef DEBUG
	sync_printf("[P%d] allocated starts at address 0x%08x\n", currpid, ptr);
#endif
    }

    uint32 i=0;

    // write data
    for(i =0; i<numInitPages; i++){
	ptr[i*PAGE_SIZE] = 'A';
    }
    
#ifdef DEBUG
    sync_printf("\n[P%d] %d pages initialized...\n", currpid, numInitPages);
    process_info(currpid);
#endif
    if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES+numPages || used_ffs_frames(currpid)!=numInitPages){
	error = 1;
    }

    // read data
    char c = 0;
    for(i=0; i<numInitPages; i++){
        c =  ptr[i*PAGE_SIZE];
        if(c!='A'){
	    sync_printf("[P%d] fail to read %d-th page\n", currpid, i);
            error = 1;
            break;
        }
    }

    if (vfree(ptr, numPages*PAGE_SIZE)==SYSERR){
	sync_printf("[P%d] vfree failed\n", currpid);
	kill(currpid);
    }
    
#ifdef DEBUG
    sync_printf("\n[P%d] %d pages freed...\n", currpid, numPages);
    process_info(currpid);
#endif
    if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES || used_ffs_frames(currpid)!=0){
	error = 1;
    }

    done = 1;
}

/* vmalloc is supposed to fail */
void test2(uint32 numPages){
    char *ptr = NULL;

#ifdef DEBUG
    process_info(currpid);
#endif 
    
    if(allocated_virtual_pages(currpid)!= PREALLOCATED_PAGES || used_ffs_frames(currpid)!=0){
        error = 1;
    }
    
#ifdef DEBUG
    sync_printf("\n[P%d] trying to allocate %d pages...\n", currpid, numPages);
#endif

    // allocate virtual heap
    ptr = vmalloc(numPages * PAGE_SIZE); 

    if (ptr!=(char *)SYSERR){
        sync_printf("[P%d] allocation should have failed!\n", currpid);
	error=1;
    	kill(currpid);
    }

    done=1;
}


/*
 * Test 1: A single process that uses only portion of FFS space
 */
void test1_run(void){

    error = 0; done = 0;

    pid32 p1 = vcreate(test, 2000, 50, "test", 2, MAX_FFS_SIZE/2, MAX_FFS_SIZE/2);
    resume(p1);

    receive();

    outcome(1);
}


/*
 * Test2: A single process that exhausts the FFS space
 */
void test2_run(void){

    error = 0; done = 0;
    pid32 p1 = vcreate(test, 2000, 50, "test", 2, MAX_FFS_SIZE, MAX_FFS_SIZE);
    resume(p1);

    receive();
    
    outcome(2);
}

/*
 * Test 3: 2 processes execute in sequence
 */
void test3_run(void){
    error = 0; done = 0;
    pid32 p1 = vcreate(test, 2000, 10, "P1", 2,  MAX_FFS_SIZE, MAX_FFS_SIZE);
    resume(p1);
    // wait for the first process to be finished
    receive();

    pid32 p2 = vcreate(test, 2000, 10, "P2", 2, MAX_FFS_SIZE, MAX_FFS_SIZE);
    resume(p2);
    // wait for the second process to be finished
    receive();

    outcome(3);
}


/*
 * Test 4: Multiple concurrent processes exhaust FFS space 
 */
void test4_run(void){

    error = 0; done = 0;	

    pid32 p1 = vcreate(test, 2000, 10, "P1", 2, MAX_FFS_SIZE/4, MAX_FFS_SIZE/4);
    pid32 p2 = vcreate(test, 2000, 10, "P2", 2, MAX_FFS_SIZE/4, MAX_FFS_SIZE/4);
    pid32 p3 = vcreate(test, 2000, 10, "P3", 2, MAX_FFS_SIZE/4, MAX_FFS_SIZE/4);
    pid32 p4 = vcreate(test, 2000, 10, "P4", 2, MAX_FFS_SIZE/4, MAX_FFS_SIZE/4);
    resume(p1);
    resume(p2);
    resume(p3);
    resume(p4);

    receive();
    receive();
    receive();
    receive();

    outcome(4);
}

/*
 * Test 5: A single process that allocate more memory than it uses (and exhausts FFS space)
 */
void test5_run(void){

    error = 0; done = 0;

    pid32 p1 = vcreate(test, 2000, 50, "test", 2, MAX_FFS_SIZE*2 , MAX_FFS_SIZE);
    resume(p1);

    receive();

    outcome(5);
}

/*
 * Test 6: Multiple concurrent processes that allocate more than the FFS space (and exhaust FFS) 
 */
void test6_run(void){

    error = 0; done = 0;

    pid32 p1 = vcreate(test, 2000, 10, "P1", 2, MAX_FFS_SIZE*4, MAX_FFS_SIZE/4);
    pid32 p2 = vcreate(test, 2000, 10, "P2", 2, MAX_FFS_SIZE*4, MAX_FFS_SIZE/4);
    pid32 p3 = vcreate(test, 2000, 10, "P3", 2, MAX_FFS_SIZE*4, MAX_FFS_SIZE/4);
    pid32 p4 = vcreate(test, 2000, 10, "P4", 2, MAX_FFS_SIZE*4, MAX_FFS_SIZE/4);
    resume(p1);
    resume(p2);
    resume(p3);
    resume(p4);

    receive();
    receive();
    receive();
    receive();

    outcome(6);
}

/*
 * Test 7: A single process tries to make allocation exceeding size of virtual address space 
 */
void test7_run(void){

    error = 0; done = 0;

    pid32 p1 = vcreate(test2, 2000, 50, "test2", 1, 1024*1024-1);
    resume(p1);

    receive();

    outcome(7);
}

/*
 * Test 8: A single process tries to make allocation exhausting PT area 
 */
void test8_run(void){

    error = 0; done = 0;

    pid32 p1 = vcreate(test2, 2000, 50, "test2", 1, ((1024*1024)-PREALLOCATED_PAGES-1));
    resume(p1);

    receive();

    outcome(8);
}

/**************   MAIN *********************/
process	main(void)
{
   sync_printf("\npreallocated pages = %d\n", PREALLOCATED_PAGES);

#ifdef TEST1
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST1       \n");
    sync_printf("=======================================\n");
    test1_run();
#endif
#ifdef TEST2
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST2       \n");
    sync_printf("=======================================\n");
    test2_run();
#endif
#ifdef TEST3
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST3       \n");
    sync_printf("=======================================\n");
    test3_run();
#endif
#ifdef TEST4
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST4       \n");
    sync_printf("=======================================\n");
    test4_run();
#endif
#ifdef TEST5
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST5       \n");
    sync_printf("=======================================\n");
    test5_run();
#endif
#ifdef TEST6
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST6       \n");
    sync_printf("=======================================\n");
    test6_run();
#endif
#ifdef TEST7
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST7       \n");
    sync_printf("=======================================\n");
    test7_run();
#endif
#ifdef TEST8
    sync_printf("\n=======================================\n");
    sync_printf("              run TEST8       \n");
    sync_printf("=======================================\n");
    test8_run();
#endif
    sync_printf("\nAll tests are done!\n");
    sync_printf("PASSED=%d FAILED=%d\n", passed, failed);
    return OK;
}


