//$Id: //depot/159/ASOS/9/main.c#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// main.c, 159

#include "spede.h"
#include "types.h"
#include "main.h"
#include "isr.h"
#include "q_mgmt.h"
#include "proc.h" // SimpleProc()
#include "entry.h"
#include "sys_calls.h"
#include "structs.h"

// globals:
int cur_pid; // current running PID, -1 means no process
q_t ready_q, avail_q,sleep_q,avail_sem_q; // processes ready to run and not used
pcb_t pcbs[NUM_PROC];               // process table
char user_stacks[NUM_PROC][USER_STACK_SIZE]; // run-time stacks for processes
struct i386_gate *idt_table;
int sys_tick; //systick and sleep second for the processes 
sem_t sems[NUM_SEM]; // Array of Semaphore Structure
mbox_t mboxes[NUM_PROC];
int print_sid;
page_t pages[NUM_PAGE]; 
int kernel_pd_addr;

void SetIDTEntry(int entry_num, func_ptr_t entry_addr)  //routine to set an entry in the interrupt descriptor table
{
        struct i386_gate *gateptr = &idt_table[entry_num];
        fill_gate(gateptr, (int)entry_addr, get_cs(), ACC_INTR_GATE,0);
}

void main()
{
   	InitData();  //initialize needed data for kernel
   	InitControl(); //set the address for 'TimerEntry' in IDT
	SpawnISR(0,IdleProc); //create IdleProc for OS to run if no user processes
	SpawnISR(1,Init); //Init Process with pid 1
        cur_pid=0;
        Loader(pcbs[cur_pid].tf_p); //Load the process in cur_pid

}

void InitData()
{
   int i;
   sys_tick=0;//initialize sys tick
   kernel_pd_addr = get_cr3();
   // queue initializations, both queues are empty first
   InitQ(&avail_q);
   InitQ(&ready_q);
   InitQ(&sleep_q);
   InitQ(&avail_sem_q);

   for(i=2; i<NUM_PROC; i++)  // since init is using PID1 is used 
   {
      pcbs[i].state = AVAIL;
      EnQ(i, &avail_q);
   }

   for(i=NUM_SEM-1; i>=0; i--)
   {
      EnQ(i, &avail_sem_q);
   }


   cur_pid = -1;  // no process is running initially

   //initialize RAM pages

   for(i=0;i<NUM_PAGE;i++)
   {
	pages[i].owner = -1;
	pages[i].addr = ((unsigned int) _topHeapMemory + (4096*i));
   }
}

void InitControl()
{
	idt_table = get_idt_base(); //get base address of IDT in RAM
        //cons_printf("Base Address of IDT is at %d\n",idt_table);   //print base address of IDT
        
	irq_enable(IRQ_TIMER);
	irq_enable(IRQ_IDE1);
	irq_enable(IRQ_IDE2);
	irq_enable(IRQ_COM2);
	
	SetIDTEntry(IRQ_VECTOR(IRQ_TIMER),TimerEntry);     //set the address of 'TimerEntry' code in entry number 32 of IDT
	SetIDTEntry(IRQ_VECTOR(IRQ_COM2),IRQ3Entry);
	//SetIDTEntry(39,IRQ7Entry);
	SetIDTEntry(IRQ_VECTOR(IRQ_IDE1),  take_hdc_inter);	/* 0x01Fx - Primary HDC */
	SetIDTEntry(IRQ_VECTOR(IRQ_IDE2), take_hdc2_inter);	/* 0x017x - Secondary HDC */

	    
	SetIDTEntry(48,GetPidEntry);	//set the address of 'GetPidEntry' code in entry number 48 of IDT
        SetIDTEntry(49,SleepEntry);	//set the address of 'SleepEntry' code in entry number 49 of IDT
	SetIDTEntry(50,SpawnEntry);	//set the address of 'SpawnEntry' code in entry number 50 of IDT
        SetIDTEntry(51,SemInitEntry);	//set the address of 'SemInitEntry' code in entry number 51 of IDT
        SetIDTEntry(52,SemWaitEntry);	//set the address of 'SemWaitEntry' code in entry number 52 of IDT
        SetIDTEntry(53,SemPostEntry);	//set the address of 'SemPostEntry' code in entry number 53 of IDT
	SetIDTEntry(54,MsgSndEntry);	//set the address of 'MsgSndEntry' code in entry number 54 of IDT
	SetIDTEntry(55,MsgRcvEntry);	//set the address of 'MsgRcvEntry' code in entry number 55 of IDT
	SetIDTEntry(56,ForkEntry);
	SetIDTEntry(57,ExitEntry);
	SetIDTEntry(58,WaitEntry);
	
}

void Scheduler() // this is kernel's process scheduler, simple round robin
{
   if(cur_pid > 0) return; // when cur_pid is not 0 (IdleProc) or -1 (none)

   if(cur_pid == 0) pcbs[0].state = READY; // skip when cur_pid is -1

   if(EmptyQ(&ready_q)) cur_pid = 0; // ready q empty, use IdleProc
   else cur_pid = DeQ(&ready_q);     // or get 1st process from ready_q

   pcbs[cur_pid].state = RUN;   // RUN state for newly selected process
}

void Kernel(tf_t *tf_p) // kernel begins its control upon/after interrupt
{
        int pid,sid;
	pcbs[cur_pid].tf_p = tf_p; //save tf_p to pcbs[cur_pid].tf_p
	switch(tf_p->intr_id)
   	{
      		case TIMER_INTR:
         		TimerISR(); // this must include dismissal of timer interrupt
         		break;
                case GETPID_INTR:
                        tf_p->eax=cur_pid;
                        break;
                case SLEEP_INTR:
                      SleepISR(tf_p->eax);
                       break;
		case SPAWN_INTR:
                      pid=DeQ(&avail_q);
                      if(pid!=-1)
                      SpawnISR(pid,(func_ptr_t) tf_p->eax);
                      tf_p->ebx=pid;
                     break;
                case SEMINIT_INTR:
                      sid=DeQ(&avail_sem_q);//Get new Semaphore from Semaphore Avail Queue  
                      if(sid!=-1)
                      SemInitISR(sid,tf_p->eax);
                      tf_p->ebx=sid;
                     break;
                case SEMWAIT_INTR:
                     SemWaitISR(tf_p->eax);
                     break;
                case  SEMPOST_INTR:
                     SemPostISR(tf_p->eax);
                    break;

		case  MSGSND_INTR:
                     MsgSndISR();
                    break;

		case  MSGRCV_INTR:
                     MsgRcvISR();
                    break;
		
		/*case IRQ7_INTR:
		     IRQ7ISR();
		      break;*/
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
		     /*  Generic EOI for slave, then master PIC. */
		     irq_ack(IRQ_IDE1);
		     irq_ack(IRQ_IDE2);
                     break;
	       case  HDC2_INTR :
                     hdc2_handler();
		     /*  Generic EOI for slave, then master PIC. */
		     irq_ack(IRQ_IDE1);
		     irq_ack(IRQ_IDE2);
                     break;
	}	

	Scheduler();                // select a process to run
	set_cr3(pcbs[cur_pid].cr3);
   	Loader(pcbs[cur_pid].tf_p); // run the process selected
} // Kernel()

