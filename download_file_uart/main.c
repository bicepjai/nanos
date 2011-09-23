//$Id: //depot/159/ASOS/9/main.c#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// main.c, 159

#include "spede.h"
#include "types.h"
#include "main.h"
#include "q_mgmt.h"
#include "isr.h"
#include "irq34.h"
#include "entry.h"

// globals:
int cur_pid; // current running PID, -1 means no process
char user_stack[USER_STACK_SIZE]; // run-time stacks for processes
struct i386_gate *idt_table;
int hd_intr_occurd;

void SetIDTEntry(int entry_num, func_ptr_t entry_addr)  //routine to set an entry in the interrupt descriptor table
{
        struct i386_gate *gateptr = &idt_table[entry_num];
        fill_gate(gateptr, (int)entry_addr, get_cs(), ACC_INTR_GATE,0);
}

void main()
{
	InitControl();
   	InitData();  // initialize needed data for kernel
	//helloworld();
	Stdin_polledio();
}

void helloworld(){
  cons_printf("hello world");
  printf("hello world");
}

void InitData()
{
  int i;
  i = 0;
}

void InitControl()
{
	idt_table = get_idt_base(); //get base address of IDT in RAM
        //cons_printf("Base Address of IDT is at %d\n",idt_table);   //print base address of IDT
        
	irq_enable(IRQ_IDE1);
	irq_enable(IRQ_IDE2);
	irq_enable(IRQ_COM2);
	
	SetIDTEntry(IRQ_VECTOR(IRQ_COM2),IRQ3Entry);
	SetIDTEntry(IRQ_VECTOR(IRQ_IDE1),  take_hdc_inter);	/* 0x01Fx - Primary HDC */
	SetIDTEntry(IRQ_VECTOR(IRQ_IDE2), take_hdc2_inter);	/* 0x017x - Secondary HDC */
}

// kernel begins its control upon/after interrupt
void ISRHandle(tf_t *tf_p) {
  /*
        int pid,sid;
	pcbs[cur_pid].tf_p = tf_p; //save tf_p to pcbs[cur_pid].tf_p
	switch(tf_p->intr_id)
   	{
		case IRQ3_INTR:
                     IRQ34ISR();
                     outportb(0x20,SPECIFIC_EOI(3));
                      break;
		case IRQ4_INTR:
                     IRQ34ISR();
                     outportb(0x20,SPECIFIC_EOI(4));   
                      break;

                case FORK_INTR:
                     ForkISR();
                     break;
                case EXIT_INTR:
                     ExitISR();
                     break;
               case  WAIT_INTR :
                     WaitISR();
                     break;   
               case  HDC_INTR :
                     hdc_handler();
		     // Generic EOI for slave, then master PIC
		     irq_ack(IRQ_IDE1);
		     irq_ack(IRQ_IDE2);
                     break;
	       case  HDC2_INTR :
                     hdc2_handler();
		     // Generic EOI for slave, then master PIC
		     irq_ack(IRQ_IDE1);
		     irq_ack(IRQ_IDE2);
                     break;
	}
*/
}
