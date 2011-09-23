//$Id: //depot/159/ASOS/9/entry.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// entry.h, 159

#ifndef _ENTRY_H_
#define _ENTRY_H_

#include <spede/machine/pic.h>

#define IRQ3_INTR 0x23 // UART RS232, COM2/4/6/8 (DOS names)
#define IRQ4_INTR 0x24 // UART RS232, COM1/3/5/7 (DOS names)

#define HDC_INTR 0x3B
#define HDC2_INTR 0x3C

#define SEL_KCODE 0x08    // kernel's code segment
#define SEL_KDATA 0x10    // kernel's data segment
#define KERNAL_STACK_SIZE 8192  // kernel's stack size, in chars

// ISR Entries
#ifndef ASSEMBLER

__BEGIN_DECLS

#include "types.h" // for tf_t below

extern void IRQ3Entry();
extern void IRQ4Entry(); 

// hd entries
extern void  take_hdc_inter();
extern void  take_hdc2_inter();
__END_DECLS

#endif

#endif
