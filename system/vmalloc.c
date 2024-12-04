#include <xinu.h>

/*------------------------------------------------------------------------
 *  vmalloc  -  Allocates enough page tables/directories for a private heap using lazy allocation
 *------------------------------------------------------------------------
 */
char* vmalloc (uint32 nbytes){
    intmask	mask;
    unsigned long user_cr3; // user process cr3 variable
    char *addr; // page address
    int i, j, k; // page dir/table entry
    uint32 counter; // counts the number of consecutive free entries
    int pde_counter, initial_pde_counter; // minimum number of pde entries needed to accommodate nbytes

    pd_t *pd; // page dir addr
    pt_t *pt; // page table addr
    uint32 frames; // number of frames needed
    unsigned int base_addr; // base addr of the page dir entry
    unsigned int va_pt_addr; // calculated virtual address for the page table, also the return value


    user_cr3 = read_cr3(); // store the user's cr3
    write_cr3(PAGE_DIR_ADDR_START | (user_cr3 & 0x00000FFF)); // switch cr3 to a system process to update PDPT area

    frames = nbytes/PAGE_SIZE; // number of frames needed
    if(frames < 1){
        frames = 1;
    }
    initial_pde_counter = frames/MAX_PT_SIZE;
    frames = frames - (initial_pde_counter * MAX_PT_SIZE);

    i = 8;
// iterate over FFS area and check for pres, if pres, go through the tables and see
// if there are enough empty tables to allocate for the private heap.
    base_addr = proctab[currpid].page_addr;
    addr = (char *)base_addr;

    pd = (pd_t *)addr;
    // kprintf("user process base addr is %x\n", base_addr);

    if((initial_pde_counter + (frames > 0)) < MAX_PT_SIZE){

        while(i != MAX_PT_SIZE){
            if(pd[i].pd_pres == 0){
                if((initial_pde_counter == 0) || ((i + initial_pde_counter) + (frames > 0) < MAX_PT_SIZE)){
                    va_pt_addr = i * PAGE_SIZE * MAX_PT_SIZE;
//                    kprintf("va_pt_addr is %x, i is %d\n", va_pt_addr, i);

                    mask = disable();
                    for(pde_counter = initial_pde_counter + (frames > 0); pde_counter > 0; pde_counter--){
                        for(base_addr = PAGE_DIR_ADDR_START; base_addr < PD_PT_AREA_END; base_addr += PAGE_SIZE){
//                            kprintf("base_addr is %x, i is %d\n", base_addr, i);
                            addr = (char *)base_addr;
                            pt = (pt_t *) addr;
                            if(pt[0].pt_avail == 0){
//                             kprintf("pt addr is %x\n", pt);

                                // allocate page dir entry
                                pd[i].pd_pres = 1;
                                pd[i].pd_write = 1;
                                pd[i].pd_user = 0;
                                pd[i].pd_pwt = 1;
                                pd[i].pd_pcd = 1;
                                pd[i].pd_acc = 1;
                                pd[i].pd_mbz = 0;
                                pd[i].pd_fmb = 0;
                                pd[i].pd_global = 0;
                                pd[i].pd_avail = 3;
                                pd[i].pd_base = (base_addr >> 12) & 0xFFFFF;
//                    // kprintf("pd[%d].pd_base is %x\n", i, pd[i].pd_base);

                                for(j = 0; j < MAX_PT_SIZE; j++){
                                    pt[j].pt_pres = 0;
                                    pt[j].pt_write = 1;
                                    pt[j].pt_user = 0;
                                    pt[j].pt_pwt = 1;
                                    pt[j].pt_pcd = 1;
                                    pt[j].pt_acc = 1;
                                    pt[j].pt_dirty = 0;
                                    pt[j].pt_mbz = 0;
                                    pt[j].pt_global = 0;
                                    pt[j].pt_avail = 2;
                                    pt[j].pt_base = 0;
                                }
                                base_addr = PD_PT_AREA_END;
                            }
                        }

                        base_addr = (pd[i].pd_base << 12);
                        addr = (char *)base_addr;
                        pt = (pt_t *) addr;
//                     kprintf("starting base address for PT page: %x, base_addr is %x, i is %d\n", va_pt_addr, base_addr, i);
                        if(initial_pde_counter > 0){
                            for(j = 0; j < MAX_PT_SIZE; j++){
                                pt[j].pt_avail = 3;
                            }
                        }else{
                            for(j = 0; j < frames; j++){
                                pt[j].pt_avail = 3;
                            }
                        }
                        i++;
                    }
                    write_cr3(user_cr3);
                    restore(mask);
                    return (char *)va_pt_addr;
                }else if((i + initial_pde_counter) + (frames > 0) >= MAX_PT_SIZE){
                    kprintf("vmalloc(): not enough page table space available!\n");
                    write_cr3(user_cr3);
                    return SYSERR;
                }
            }else{
                // for loop through page table
                // check if i have to allocate a pde
                // if not, search through the pt table
                base_addr = (pd[i].pd_base << 12);
                addr = (char *)base_addr;
                pt = (pt_t *)addr;
//            kprintf("pt addr is %x\n", pt);
                if(initial_pde_counter == 0 || (initial_pde_counter == 1 && frames == 0)){
                    frames = frames + initial_pde_counter * MAX_PT_SIZE;
                    for(j = 0; j < MAX_PT_SIZE; j++){
                        if(pt[j].pt_avail != 3){
//                    kprintf("pt[%d].pt_avail is %d\n", j, pt[j].pt_avail);
                            counter = 0;
                            int next_index; // next index to start the page table search from
                            if(j + frames <= MAX_PT_SIZE){
//                        kprintf("%d + %d is <= %d\n", j, frames, MAX_PT_SIZE);
                                for(k = 0; k < frames; k++){
                                    if(pt[j+k].pt_avail != 3){
                                        counter++;
                                    }else{
                                        next_index = j+k;
                                    }
                                }
                                if(counter == frames){
                                    va_pt_addr = (i * PAGE_SIZE * MAX_PT_SIZE) + (PAGE_SIZE * j);
                                    // kprintf("starting address for PT page: %x\n", va_pt_addr);
                                    mask = disable();
                                    for(k = 0; k < frames; k++){
                                        pt[j+k].pt_avail = 3;
                                    }
                                    write_cr3(user_cr3);
                                    restore(mask);
                                    return (char *)va_pt_addr;
                                }else{
                                    j = next_index;
                                }
                            }else{
//                            kprintf("%d + %d is > %d\n", j, frames, MAX_PT_SIZE);
                                // makes sure the rest of the pt is empty before moving on
                                for (k = j; k < MAX_PT_SIZE; k++) {
                                    if (pt[k].pt_avail != 3) {
                                        counter++;
                                    }
                                }
                                if (counter == (MAX_PT_SIZE - j)) {
//                                kprintf("%d == %d\n", counter, (MAX_PT_SIZE-j));
                                    // make another for loop that goes through the next table
                                    // if pres is 0, initialize it and stuff
                                    // if pres is 1, make sure the first few slots are empty
                                    // if not empty, change the pd entry and start over,
                                    // ie, i++ and so on
                                    if (i != (MAX_PT_SIZE - 1)) {
                                        if (pd[i + 1].pd_pres == 0) {
                                            frames -= counter;
                                            //initialize the last couple page tables of the page dir entry
                                            va_pt_addr = (i * PAGE_SIZE * MAX_PT_SIZE) + (PAGE_SIZE * j);
                                            mask = disable();
                                            for (; j < MAX_PT_SIZE; j++) {
                                                pt[j].pt_avail = 3;
                                            }
                                            //initialize the new page dir entry and finish initializing the tables
                                            // allocate page dir entry
                                            for (base_addr = PAGE_DIR_ADDR_START;
                                                 base_addr < PD_PT_AREA_END; base_addr += PAGE_SIZE) {
                                                addr = (char *) base_addr;
                                                pt = (pt_t *) addr;
                                                if (pt[0].pt_avail == 0) {
                                                    // kprintf("pt addr is %x\n", pt);
                                                    pd[i + 1].pd_pres = 1;
                                                    pd[i + 1].pd_write = 1;
                                                    pd[i + 1].pd_user = 0;
                                                    pd[i + 1].pd_pwt = 1;
                                                    pd[i + 1].pd_pcd = 1;
                                                    pd[i + 1].pd_acc = 1;
                                                    pd[i + 1].pd_mbz = 0;
                                                    pd[i + 1].pd_fmb = 0;
                                                    pd[i + 1].pd_global = 0;
                                                    pd[i + 1].pd_avail = 3;
                                                    pd[i + 1].pd_base = (base_addr >> 12) & 0xFFFFF;

                                                    for (j = 0; j < MAX_PT_SIZE; j++) {
                                                        pt[j].pt_pres = 0;
                                                        pt[j].pt_write = 1;
                                                        pt[j].pt_user = 0;
                                                        pt[j].pt_pwt = 1;
                                                        pt[j].pt_pcd = 1;
                                                        pt[j].pt_acc = 1;
                                                        pt[j].pt_dirty = 0;
                                                        pt[j].pt_mbz = 0;
                                                        pt[j].pt_global = 0;
                                                        pt[j].pt_avail = 2;
                                                        pt[j].pt_base = 0;
                                                    }
                                                }
                                                base_addr = PD_PT_AREA_END;
                                            }

                                            base_addr = (pd[i].pd_base << 12);
                                            addr = (char *) base_addr;
                                            pt = (pt_t *) addr;
                                            // kprintf("pt addr is %x\n", pt);

                                            for (j = 0; j < frames; j++) {
                                                pt[j].pt_avail = 3;
                                            }
                                            write_cr3(user_cr3);
                                            restore(mask);
                                            return (char *) va_pt_addr;
                                        }else{
                                            addr = (char *) (pd[i + 1].pd_base << 12);
                                            pt_t *temp_pt = (pt_t *) addr;
                                            // kprintf("temp addr is %x\n", temp_pt);
                                            int counter_v2 = 0;
                                            for (k = 0; k < frames; k++) {
                                                if (temp_pt[k].pt_avail != 3) {
                                                    counter_v2++;
                                                }
                                            }
                                            if ((counter + counter_v2) == frames) {
                                                // initialize last couple free pages before moving on
                                                va_pt_addr = (i * PAGE_SIZE * MAX_PT_SIZE) + (PAGE_SIZE * j);
                                                mask = disable();
                                                for (; j < MAX_PT_SIZE; j++) {
                                                    pt[j].pt_avail = 3;
                                                }

                                                for (j = 0; j < counter_v2; j++) {
                                                    temp_pt[j].pt_avail = 3;
                                                }
                                                write_cr3(user_cr3);
                                                restore(mask);
                                                return (char *) va_pt_addr;
                                            }else{
                                                j = MAX_PT_SIZE;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }else{
                    j = 0;
                    while(j < initial_pde_counter + (frames > 0)){
                        for(k = 0; k < MAX_PT_SIZE; k++){
                            if(pt[k].pt_avail != 3){
                                counter++;
                            }else{
                                counter = 0;
                            }
                            if(counter == (frames + initial_pde_counter * MAX_PT_SIZE)){
                                k = MAX_PT_SIZE;
                            }
                        }
                        if(counter > 0 || (counter == (frames + initial_pde_counter * MAX_PT_SIZE))){
                            j++;
                            if(j == 1){
                                va_pt_addr = (i * PAGE_SIZE * MAX_PT_SIZE) + (PAGE_SIZE * (k - counter));
                            }
                            if(i + j < MAX_PT_SIZE){
                                if(pd[i+j].pd_pres == 0){
                                    if (j > 2){
                                        if(counter > (j-2) * MAX_PT_SIZE){
                                            // this else means there will be consecutive free entries
                                            counter = frames + initial_pde_counter * MAX_PT_SIZE;
                                            j = initial_pde_counter;
                                        }
                                    }else{
                                        // this else means there will be consecutive free entries
                                        counter = frames + initial_pde_counter * MAX_PT_SIZE;
                                        j = initial_pde_counter;
                                    }
                                }else{
                                    addr = (char *)(pd[i+j].pd_base << 12);
                                    pt = (pt_t *)addr;
                                }
                            }
                        }else{
                            j = initial_pde_counter;
                        }
                    }
                    if(counter == (frames + initial_pde_counter * MAX_PT_SIZE)){
                        j = (va_pt_addr - (i * PAGE_SIZE * MAX_PT_SIZE))/PAGE_SIZE;
                        addr = (char *)(pd[i].pd_base << 12);
                        pt = (pt_t *)addr;
                        for(;j < MAX_PT_SIZE; j++){
                            pt[j].pt_avail = 3;
                            counter--;
                        }
                        while(counter > 0){
                            i++;
                            if(pd[i].pd_pres == 0){
                                for(base_addr = PAGE_DIR_ADDR_START; base_addr < PD_PT_AREA_END; base_addr += PAGE_SIZE){
                                    addr = (char *)base_addr;
                                    pt = (pt_t *) addr;
                                    if(pt[0].pt_avail == 0){
//                                    kprintf("pt addr is %x\n", pt);

                                        // allocate page dir entry
                                        pd[i].pd_pres = 1;
                                        pd[i].pd_write = 1;
                                        pd[i].pd_user = 0;
                                        pd[i].pd_pwt = 1;
                                        pd[i].pd_pcd = 1;
                                        pd[i].pd_acc = 1;
                                        pd[i].pd_mbz = 0;
                                        pd[i].pd_fmb = 0;
                                        pd[i].pd_global = 0;
                                        pd[i].pd_avail = 3;
                                        pd[i].pd_base = (base_addr >> 12) & 0xFFFFF;
//                                    kprintf("pd[%d].pd_base is %x\n", i, pd[i].pd_base);

                                        for(j = 0; j < MAX_PT_SIZE; j++){
                                            pt[j].pt_pres = 0;
                                            pt[j].pt_write = 1;
                                            pt[j].pt_user = 0;
                                            pt[j].pt_pwt = 1;
                                            pt[j].pt_pcd = 1;
                                            pt[j].pt_acc = 1;
                                            pt[j].pt_dirty = 0;
                                            pt[j].pt_mbz = 0;
                                            pt[j].pt_global = 0;
                                            pt[j].pt_avail = 2;
                                            pt[j].pt_base = 0;
                                        }
                                        base_addr = PD_PT_AREA_END;
                                    }
                                }
                                base_addr = (pd[i].pd_base << 12);
                                addr = (char *)base_addr;
                                pt = (pt_t *) addr;
                                for(j = 0; j < MAX_PT_SIZE; j++){
                                    pt[j].pt_avail = 3;
                                    counter--;
                                    if(counter == 0){
                                        j = MAX_PT_SIZE;
                                    }
                                }
                            }else{
                                addr = (char *)(pd[i].pd_base << 12);
                                pt = (pt_t *)addr;
                                for(j = 0;j < MAX_PT_SIZE; j++){
                                    pt[j].pt_avail = 3;
                                    counter--;
                                    if(counter == 0){
                                        j = MAX_PT_SIZE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            i++;
        }
    }else{
        kprintf("vmalloc(): not enough contiguous heap space available!\n");
    }

    write_cr3(user_cr3);
    return SYSERR;
}
