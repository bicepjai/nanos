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
#include "proc.h"

q_t* t;
mbox_t* t1;

void SpawnISR(int pid,func_ptr_t addr)
{
  
//clear run-time stack by zero-filling it (privacy + security)
	bzero((void *)user_stacks[pid], USER_STACK_SIZE);

// 1st. point to just above of user stack, then drop by 64 bytes (tf_t)
   	pcbs[pid].tf_p = (tf_t *)&user_stacks[pid][USER_STACK_SIZE];
   	pcbs[pid].tf_p--;    // pointer arithmetic, now points to trapframe

// fill in CPU's register context
   	pcbs[pid].tf_p->eflags = EF_DEFAULT_VALUE|EF_INTR;
   	pcbs[pid].tf_p->eip = (unsigned int)addr; // new process code
   	pcbs[pid].tf_p->cs = get_cs();
   	pcbs[pid].tf_p->ds = get_ds();
   	pcbs[pid].tf_p->es = get_es();
   	pcbs[pid].tf_p->fs = get_fs();
   	pcbs[pid].tf_p->gs = get_gs();

   	pcbs[pid].tick_count = pcbs[pid].total_tick_count = 0;
   	pcbs[pid].state = READY;
	pcbs[pid].cr3 = kernel_pd_addr;

   	if(pid != 0) EnQ(pid, &ready_q);  // IdleProc (PID 0) is not queued
	bzero(&mboxes[pid],sizeof(mbox_t));

}

void TimerISR()
{
   int wake_pid;
   //outportb(0x20,SPECIFIC_EOI(0));	//dismiss IRQ 0
   irq_ack(IRQ_TIMER);
   sys_tick++; //increment systick
 

   while(!(EmptyQ(&sleep_q)) && (pcbs[sleep_q.q[sleep_q.head]].wake_tick==sys_tick)) 
   {
        wake_pid=DeQ(&sleep_q);
        pcbs[wake_pid].state=READY;
        EnQ(wake_pid,&ready_q);
            
   }

   if(cur_pid == 0)
   {
         return; // if Idle process, no need to do this on it
   }

    pcbs[cur_pid].tick_count++;  //increment tick_count

   if(pcbs[cur_pid].tick_count == TIME_SLICE) // running up time, preempt it?
   {
      pcbs[cur_pid].tick_count = 0; // reset (roll over) usage time
      pcbs[cur_pid].total_tick_count += TIME_SLICE; // sum to total
      pcbs[cur_pid].state = READY;  // change its state
      EnQ(cur_pid, &ready_q);       // move it to ready_q
      cur_pid = -1;                 // no longer running
   }
	
}

void SleepISR(int sleep_secs)
{
  pcbs[cur_pid].state=SLEEP;
  pcbs[cur_pid].wake_tick=sys_tick + sleep_secs * 100; //calculate wake tick for the processes 
  AddSleepQ();
  cur_pid=-1;
}

void SemInitISR(int sid,int passcount)
{
  sems[sid].pass_count=passcount; // set the pass count
  InitQ(&sems[sid].wait_q); // Reset the Wait Queue

}

void SemWaitISR(int sid)
{

 if(sems[sid].pass_count==0) //Check if the passcount is zero
  {
  EnQ(cur_pid,&sems[sid].wait_q); //Enqueue the current pid to Ready Queue
  pcbs[cur_pid].state = WAIT; // Change the status to WAIT
  cur_pid=-1; //Reset the current pid
  }
  if(sems[sid].pass_count>0)
    sems[sid].pass_count--;

}

void SemPostISR(int sid)
{
  int local_pid;
  if(EmptyQ(&sems[sid].wait_q)) // if wait queue is empty increment the pass count 
   {
     sems[sid].pass_count++;
   }
  else
   {
     local_pid = DeQ(&sems[sid].wait_q);
     EnQ(local_pid,&ready_q); // if the wait queue is not empty dequeue the pid 
     pcbs[local_pid].state = READY;
   }
}


void MsgSndISR()
{
  int pid,mid = pcbs[cur_pid].tf_p->eax;
  msg_t *mp = (msg_t*)pcbs[cur_pid].tf_p->ebx;
  
  mp->sender = cur_pid;
  mp->send_tick = sys_tick;

  if(EmptyQ(&mboxes[mid].wait_q))
  	MsgEnQ(mp,&mboxes[mid].msg_q);
  else
    {
	pid = DeQ(&mboxes[mid].wait_q);
	pcbs[pid].state = READY;
	EnQ(pid,&ready_q);
	memcpy((msg_t*)pcbs[pid].tf_p->ebx,mp,sizeof(msg_t));
    }
}

void MsgRcvISR()
{
  int mid = cur_pid;
  msg_t *mp = (msg_t*)pcbs[cur_pid].tf_p->ebx;
 
  if(!MsgEmptyQ(&mboxes[mid].msg_q))
   memcpy(mp,MsgDeQ(&mboxes[mid].msg_q),sizeof(msg_t));
  else
  {
	EnQ(cur_pid,&mboxes[mid].wait_q);
 	pcbs[cur_pid].state = WAIT;
	cur_pid = -1;
  }
}

void ForkISR()
{

   int cpid,addr,size,i,j,pd_addr;
   int *p;
   addr = pcbs[cur_pid].tf_p->eax;
   size = pcbs[cur_pid].tf_p->ebx;

   cpid=DeQ(&avail_q);
   
   for(j=0;j<NUM_PROCESS;j++)
   {
   	for(i=0;i<NUM_PAGE;i++)
   	{
		if(pages[i].owner==-1)
        	{
			pages[i].owner = cpid;
			pcbs[cpid].ram_pages[j] = i;
			bzero((void*)pages[i].addr,4096);
			break;
		}
   	}
   }
   
   pd_addr = pages[pcbs[cpid].ram_pages[0]].addr;
   pcbs[cpid].cr3 = pd_addr;  
   memcpy((int*)pd_addr,(int*)kernel_pd_addr,(16*sizeof(int)));
   p = (int*)pd_addr;
   p+=256;
   *p = pages[pcbs[cpid].ram_pages[1]].addr + 3;
   p+=511;
   *p = pages[pcbs[cpid].ram_pages[2]].addr + 3;
   p = (int*)pages[pcbs[cpid].ram_pages[1]].addr;
   *p = pages[pcbs[cpid].ram_pages[3]].addr + 3;
   p = (int*)pages[pcbs[cpid].ram_pages[2]].addr;
   p+= 1023;
   *p = pages[pcbs[cpid].ram_pages[4]].addr + 3;
   set_cr3(pd_addr); 
   pcbs[cpid].ppid = cur_pid;
   memcpy((int*)G1,(int*)addr,size);
   
   pcbs[cpid].tf_p = (tf_t*)(G3 - sizeof(tf_t));
   pcbs[cpid].tf_p->eflags = EF_DEFAULT_VALUE|EF_INTR;
   pcbs[cpid].tf_p->eip = G1;
   pcbs[cpid].tf_p->cs = get_cs();
   pcbs[cpid].tf_p->ds = get_ds();
   pcbs[cpid].tf_p->es = get_es();
   pcbs[cpid].tf_p->fs = get_fs();
   pcbs[cpid].tf_p->gs = get_gs();

   pcbs[cpid].tick_count = pcbs[cpid].total_tick_count = 0;
   pcbs[cpid].state = READY;
   EnQ(cpid, &ready_q);

   pcbs[cur_pid].tf_p->ebx = cpid;
}  

void WaitISR()
{
   int* p;
   int cpid,i;
   int zombie_found;

   zombie_found = 0;
   
  for(i=0;i<NUM_PROC;i++)
  {
	if(pcbs[i].ppid == cur_pid)
	{
		cpid = i;
   		if(pcbs[cpid].state == ZOMBIE)
   		{
			zombie_found = 1;
			pcbs[cur_pid].tf_p->ebx=cpid;
			p = (int*)pcbs[cur_pid].tf_p->eax;
			*p = pcbs[cpid].exit_code;
			EnQ(cpid,&avail_q);
			pcbs[cpid].state = AVAIL;
                        break;  
   		}
	}
   }   	
 
  if(zombie_found == 0)
   {
	pcbs[cur_pid].state = WAIT;
	cur_pid = -1;
   } 
}

void ExitISR()
{


   int exit_code,i;
   exit_code = pcbs[cur_pid].tf_p->eax;
   set_cr3(kernel_pd_addr);
   
   for(i=0;i<NUM_PROCESS;i++)
   {
	pages[pcbs[cur_pid].ram_pages[i]].owner = -1;
   }
   
   if(pcbs[pcbs[cur_pid].ppid].state == WAIT)
   {
	pcbs[pcbs[cur_pid].ppid].state = READY;
	EnQ(pcbs[cur_pid].ppid,&ready_q);
	pcbs[pcbs[cur_pid].ppid].tf_p->ebx = cur_pid;
	*((int*)pcbs[pcbs[cur_pid].ppid].tf_p->eax) = exit_code;
	pcbs[cur_pid].state = AVAIL;
	EnQ(cur_pid,&avail_q);
	cur_pid = -1;
   }
   else
   {
	pcbs[cur_pid].state = ZOMBIE;
        pcbs[cur_pid].exit_code=exit_code;
	cur_pid = -1;
   }
}	

extern volatile uint16 *  _vidmembase;


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
