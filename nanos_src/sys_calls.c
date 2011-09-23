//$Id: //depot/159/ASOS/9/sys_calls.c#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

#include "types.h"

 int GetPid()
   {
      int pid;

      asm("int $0x30; movl %%eax, %0"
          : "=g" (pid)
          :
          : "eax");
    
      return pid;
   }

   void Sleep(int sleep_secs) // sleep for how many centi- (1/100) secs
   {
      asm("movl %0, %%eax; int $0x31"  // copy secs to eax, call intr
          :                            // no output
          : "g" (sleep_secs)           // 1 input
          : "eax");                    // 1 overwritten register
   }

 int Spawn(func_ptr_t addr)
   {
      int cpid;

      asm("movl %1,%%eax; int $0x32; movl %%ebx, %0"
          : "=g" (cpid)
          : "g" ((unsigned int) addr)
          : "eax","ebx");
      return cpid;

   }
  int SemInit(int passcount)
   {
      int sem;
       asm("movl %1,%%eax; int $0x33; movl %%ebx, %0"
          : "=g" (sem)
          : "g" (passcount)
          : "eax","ebx");
      return sem;

   }
  void SemWait(int sid)
  {
      asm("movl %0, %%eax; int $0x34"
        :                            // no output
        : "g" (sid)           // 1 input
        : "eax");                    // 1 overwritten register

  }
 void SemPost(int sid)
 {
      asm("movl %0, %%eax; int $0x35"
        :                            // no output
        : "g" (sid)           // 1 input
        : "eax");                    // 1 overwritten register
 }

void MsgSnd(int mid,msg_t *mp)
{
	asm("movl %0,%%eax; movl %1,%%ebx; int $0x36"
          : 
          : "g"(mid),"g"((unsigned int)mp)
          : "eax","ebx");
}

void MsgRcv(msg_t *mp)
{
        asm("movl %0,%%ebx; int $0x37"
          : 
          : "g"((unsigned int)mp)
          : "ebx");
}

int Fork(int addr,int size)
{
	int cpid;
	asm("movl %1,%%eax; movl %2,%%ebx; int $0x38; movl %%ebx, %0"
          : "=g" (cpid)
          : "g" ((unsigned int)addr),"g" ((unsigned int)size)
          : "eax","ebx");
	return cpid;
}

int Wait(int* exit_code)
{
        int cpid;
        asm("movl %1,%%eax; int $0x3A; movl %%ebx, %0"
          : "=g" ((int)cpid)
          : "g"((unsigned int)exit_code)
          : "eax","ebx");
 	return cpid;       
}

void Exit(int exit_code)
{
	asm("movl %0,%%eax; int $0x39"
          : 
          : "g"((int)exit_code)
          : "eax");
}
