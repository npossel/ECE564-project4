#include <xinu.h>

/*------------------------------------------------------------------------
 *  vfree  -  Frees the amount of physical
 *------------------------------------------------------------------------
 */
syscall vfree (char* ptr, uint32 nbytes){
    intmask	mask;
    unsigned long user_cr3; // user process cr3 variable
    char *addr; // page address
    int i, j; // page dir/table entry
    uint32 counter; // counts the number of consecutive page entries

    pd_t *pd; // page dir addr
    pt_t *pt; // page table addr
    uint32 pages; // number of frames needed
    unsigned int base_addr; // base addr of the page dir entry

    mask = disable();
    user_cr3 = read_cr3(); // store the user's cr3
    write_cr3(PAGE_DIR_ADDR_START | (user_cr3 & 0x00000FFF)); // switch cr3 to a system process to update PDPT area
    restore(mask);

    base_addr = proctab[currpid].page_addr;
    addr = (char *)base_addr;
    pd = (pd_t *)addr;

    pages = nbytes/PAGE_SIZE; // number of pages to be free
    counter = 0;
    // get the upper bound of valid page tables to make sure the process can't free a page outside its directory
    for(i = 8; i < MAX_PT_SIZE; i++){
        if(pd[i].pd_pres == 0){
            addr = (char *)(pd[i-1].pd_base << 12);
            // kprintf("i is %d\n", i);
            j = i;
            i = MAX_PT_SIZE;
        }else if(i == 1023 && (pd[i].pd_pres == 1)){
            addr = (char *)(pd[i].pd_base << 12);
            j = i;
        }
    }
    pt = (pt_t *)addr;
    for(i = (MAX_PT_SIZE - 1); i >= 0; i--){
        if(pt[i].pt_avail == 3){
            addr = (char *)((j * PAGE_SIZE * MAX_PT_SIZE) + (PAGE_SIZE * i));
            // kprintf("i is %d\n", i);
            i = -1;
        }
    }
    // kprintf("max addr %x\n", addr);
    // kprintf("ptr addr %x\n", ptr);


    if(ptr < addr){
        i = (int)ptr/(PAGE_SIZE * MAX_PT_SIZE);
        // kprintf("pd index is %x\n", i);
        j = (int)ptr - (i * PAGE_SIZE * MAX_PT_SIZE);
        if(j != 0){
            j = j/PAGE_SIZE;
        }
        addr = (char *)(pd[i].pd_base << 12);
        pt = (pt_t *)addr;

        for(i = 0; i < pages; i++){
            if(pt[j+i].pt_avail == 3){
                counter++;
            }
        }
        // kprintf("counter is %d\n", counter);

        if(counter == pages){
            for(i = j; i < (j + pages); i++){
                pt[i].pt_pres = 0;
                pt[i].pt_avail = 0;
                pt[i].pt_base = 0;
            }
            mask = disable();
            write_cr3(user_cr3);
            restore(mask);
            return OK;
        }
    }
    mask = disable();
    write_cr3(user_cr3);
    restore(mask);
    return SYSERR;
}