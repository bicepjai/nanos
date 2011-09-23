//$Id: //depot/159/ASOS/9/isr.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// isr.h, 159

#ifndef _ISR_H_
#define _ISR_H_

void SpawnISR(int,func_ptr_t);
void TimerISR();
void SleepISR(int);

void SemInitISR(int,int);
void SemWaitISR(int);
void SemPostISR(int);

void MsgSndISR();
void MsgRcvISR();
void IRQ34ISR();
void ForkISR();
void WaitISR();
void ExitISR();

void hdc_handler(void);
void hdc2_handler(void);

void irq_ack(unsigned irq);
void irq_enable(unsigned irq);

#endif
