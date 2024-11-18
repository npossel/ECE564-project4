/* pagefault_handler_disp.s - pagefault_handler_disp (x86) */

/*------------------------------------------------------------------------
 * pagefault_handler_disp  -  Interrupt dispatcher for page faults
 *------------------------------------------------------------------------
 */
		.text
		.globl	pagefault_handler_disp		# Page fault interrupt dispatcher 
pagefault_handler_disp:
		pushal			# Save registers
		cli			# Disable further interrupts
		
		call	pagefault_handler	# Call high level handler

		sti			# Restore interrupt status
		popal			# Restore registers
		add $4, %esp		# Skip error message	
		iret			# Return from interrupt
