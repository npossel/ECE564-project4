#include <xinu.h>

char* vmalloc (uint32 nbytes){
    return 0;
}

syscall vfree (char* ptr, uint32 nbytes){
    return OK;
}
