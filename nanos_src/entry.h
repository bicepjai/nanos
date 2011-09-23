//$Id: //depot/159/ASOS/9/entry.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// entry.h, 159

#ifndef _ENTRY_H_
#define _ENTRY_H_

#include <spede/machine/pic.h>

#define TIMER_INTR 0x20
#define IRQ3_INTR 0x23 // UART RS232, COM2/4/6/8 (DOS names)
#define IRQ4_INTR 0x24 // UART RS232, COM1/3/5/7 (DOS names)
//#define IRQ7_INTR 0x27
#define GETPID_INTR 0x30
#define SLEEP_INTR 0x31
#define SPAWN_INTR 0x32
#define SEMINIT_INTR 0x33
#define SEMWAIT_INTR 0x34
#define SEMPOST_INTR 0x35
#define MSGSND_INTR 0x36
#define MSGRCV_INTR 0x37
#define FORK_INTR 0x38
#define EXIT_INTR 0X39
#define WAIT_INTR 0x3A

#define HDC_INTR 0x3B
#define HDC2_INTR 0x3C

#define SEL_KCODE 0x08    // kernel's code segment
#define SEL_KDATA 0x10    // kernel's data segment
#define KERNAL_STACK_SIZE 8192  // kernel's stack size, in chars

// ISR Entries
#ifndef ASSEMBLER

__BEGIN_DECLS

#include "types.h" // for tf_t below

extern void TimerEntry();     // code defined in entry.S
extern void Loader(tf_t *);   
extern void GetPidEntry();
extern void SleepEntry();
extern void SpawnEntry();
extern void SemInitEntry();
extern void SemWaitEntry();
extern void SemPostEntry();
extern void MsgSndEntry();
extern void MsgRcvEntry();
//extern void IRQ7Entry();
extern void IRQ3Entry();
extern void IRQ4Entry(); 
extern void ForkEntry();
extern void ExitEntry();
extern void WaitEntry();
// hd entries
extern void  take_hdc_inter();
extern void  take_hdc2_inter();
__END_DECLS

#endif

#endif
