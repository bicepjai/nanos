/*   DiskIO/structs.h
 *	@(#)structs.h	2.1   02/13/00
 */

#ifndef _TEST_STRUCTS_H
#define _TEST_STRUCTS_H

/*  These are needed by "main.c" */
#ifdef HARNESS
# include <stdio.h>
# include <stdlib.h>
# include <strings.h>
# include <assert.h>

#include <spede/flames.h>

# undef   cli
# undef   sti
# undef   DI
# undef   EI
# define  TRUE  1
# define  FALSE 0

struct GeneralRegs { int regs[6]; };

#else
# include <spede/stdio.h>
# include <spede/stdlib.h>
# include <spede/string.h>
# include <spede/assert.h>

# include <spede/flames.h>
# include <spede/util.h>
# include <spede/machine/proc_reg.h>	/* For "struct GeneralRegs" */
# include <spede/machine/seg.h>

#endif	/* ifdef HARNESS */

__BEGIN_DECLS

/* -------------------------------------------------------------- */

#ifndef MAX
# define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
# define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif



#ifndef __cplusplus
typedef int	bool;
#endif


/* -------------------------------------------------------------- */


/*  PIC vectors.. 	*/
#define  BASE_PIC0_VECTOR	0x20
#define  BASE_PIC1_VECTOR	(BASE_PIC0_VECTOR + 8)

#define  PIC_INDEX_TIMER_0	0
#define  PIC_MASK_TIMER_0	BIT(PIC_INDEX_TIMER_0)

#define  PIC_INDEX_SLAVE	2
#define  PIC_MASK_SLAVE 	BIT(PIC_INDEX_SLAVE)

#define  PIC_INDEX_FLOPPY	6
#define  PIC_MASK_FLOPPY	BIT(PIC_INDEX_FLOPPY)

#define  PIC_INDEX_HDC		14
#define  PIC_MASK_HDC		BIT(PIC_INDEX_HDC)

#define  PIC_INDEX_HDC2		15
#define  PIC_MASK_HDC2		BIT(PIC_INDEX_HDC2)

#define  TICKS_SECOND		20
#define  MICROSEC_TICK	  ((unsigned) (1000000/TICKS_SECOND))

/*  Kernel code and data segment descriptors (as setup by BOOT.COM) */
#define  KSEG_CODE	0x08
#define  KSEG_DATA	0x10

typedef void    (*PFV)(void);
/* used for hd*/
extern void	microsec_wait(unsigned microsecs);

/*  Routines from "helper.S"  */
extern void	pause();

__END_DECLS

/* -------------------------------------------------------------- */

#endif
