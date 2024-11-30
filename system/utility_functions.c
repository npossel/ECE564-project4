#include <xinu.h>

uint32 free_ffs_pages(){
    unsigned long base_address;
    uint32 i;
    uint32 free;
    pt_t *pt;
    char *address;

    base_address = PAGE_DIR_ADDR_START+XINU_PAGES*4+PAGE_SIZE*1;
    // kprintf("Base of FFS area page tables: %x\n", base_address);
    address = (char *)base_address;
    pt = (pt_t *)address;
    free = 0;
    for(i=0; i<MAX_FFS_SIZE; i++) {
        if(pt[i].pt_avail & 1) {}
        else {
            free++;
        }
    }
    return free;
}

uint32 free_swap_pages(){
    return 0;
}

uint32 allocated_virtual_pages(pid32 pid){
    struct procent *prptr;
    unsigned long cr3;
    char *address;
    pt_t *pt;
    pd_t *pd;
    uint32 i, j;
    uint32 allocated;

    allocated = 0;
    prptr = &proctab[pid];
    cr3 = prptr->page_addr;
    address = (char *)cr3;
    pd = (pd_t *)address;

    for(i=0; i<1024; i++) {
        if(pd[i].pd_pres==1) {
            cr3 = pd[i].pd_base << 12;
            address = (char *)cr3;
            pt = (pt_t *)address;

            for(j=0; j<1024; j++) {
                if(pt[j].pt_avail & 1) {
                    allocated++;
                }
            }
        }
    }
    return allocated;
}

uint32 used_ffs_frames(pid32 pid){
    return 0;
}

