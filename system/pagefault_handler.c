/* pagefault_handler.c - pagefault_handler  */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  pagefault_handler   -   Handler pagefaults caused by user processes
 *------------------------------------------------------------------------
 */
syscall pagefault_handler(
    pid32   pid     /* ID of process committing pagefault   */
)
{
    // set CR3 to point to system process page table

    // In here, present is 0, if valid is 0 (00[0]) then this is a segmentation fault.
    // print out whatever statement and kill process.

    // end with resetting CR3 to point to PDBR of process that was executing when page fault occured
}