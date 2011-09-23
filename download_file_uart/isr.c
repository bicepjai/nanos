//$Id: //depot/159/ASOS/9/isr.c#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// isr.c, 159
// ISRs called from Kernel()

#include "spede.h"
#include "types.h"
#include "isr.h"
#include "q_mgmt.h"
#include "externs.h"


/*****************************************************************************
	name:	irq14
	action:	IRQ 14 handler
*****************************************************************************/
void hdc_handler(void)
{
    hd_intr_occurd |= 0x4000;
    //printf("\nhd intr 1 occurd");
}

/*****************************************************************************
	name:	irq15
	action:	IRQ 15 handler
*****************************************************************************/
void hdc2_handler(void)
{
    hd_intr_occurd |= 0x8000;
    //printf("\nhd intr 2 occurd");
}

void irq_enable(unsigned irq) {
  assert ( irq < NR_IRQS);
  if ( irq >= 8 ) {
      outportb(0x21, inportb(0x21) & ~BIT(IRQ_CASCADE));
      outportb(0xA1, inportb(0xA1) & ~BIT(irq-8));
  } else {
      outportb(0x21, inportb(0x21) & ~BIT(irq));
  }
}

void irq_ack(unsigned irq) {
  assert ( irq < NR_IRQS);
  if ( irq >= 8 ) {
      outportb(0x20, SPECIFIC_EOI(IRQ_CASCADE));
      outportb(0xA0, SPECIFIC_EOI(irq-8));
  } else {
      outportb(0x20, SPECIFIC_EOI(irq));
  } 
}
