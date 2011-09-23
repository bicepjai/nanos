
/**
 *  For some CPU generated interrupts, an error code is pushed.
 *  Exceptions 8 and 10-14 push this extra code, we pop off before
 *  the "iret".  The IRQ's push a dummy error code.
 *
 *  The hard disk IDE driver calls into "ata.c" directly.
 *
 *
 */

#include <spede/machine/asmacros.h>

/*  When the applicaiton is ready for another interrupt, call here.
 *  We use sti/halt to atomically enable interrupts and halt the
 *  CPU--this saves a fair amount of power and heat.
 */
ENTRY(pause)
	sti			/* I want interrupts! */
	hlt			/* And I'll wait for them! */
	ret