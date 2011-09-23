//$Id: //depot/159/ASOS/9/types.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// types.h, 159

#ifndef _TYPES_H_
#define _TYPES_H_

#include "trap.h"            // defines tf_t, trap frame type
#include "spede.h"

#define TIME_SLICE 5       // max runtime per run per proc, 2 secs
#define NUM_PROC 20          // max number of processes allowed in the system
#define Q_SIZE 20            // queuing capacity
#define USER_STACK_SIZE 524288 // # of bytes for runtime stack
#define NUM_MSG 20     // # of msgs in msg_q of a mailbox
#define NUM_NUM 4      // # of numbers in int array of a message
#define NUM_BYTE 101   // # of chars in str of a message
#define NUM_PAGE 12800 //Number of RAM pages
#define NUM_PROCESS 5  //No of pages per process 
#define G1 0x40000000 // code at 1G
#define G3 0xC0000000 // stack ends at 3G-1
#define NUM_SEM 20	//Number of semaphores

typedef unsigned char	u8;	/* 8 bits */
typedef unsigned short	u16;	/* 16 bits */
typedef unsigned long	u32;	/* 32 bits */

typedef enum {AVAIL, READY, RUN, SLEEP, WAIT, ZOMBIE} state_t;

typedef struct            // PCB describes proc image
{
   state_t state;         // state of process
   int tick_count,        // accumulative run time since dispatched
       total_tick_count;  // accumulative run time since creation
   tf_t *tf_p;            // process register context
   int wake_tick;         // wake tick
   int ram_pages[5];
   int ppid;
   int exit_code; 
   int cr3;
} pcb_t;

typedef struct // queue type: head & tail pointing array of PIDs
{
   int count, head, tail, q[Q_SIZE];
} q_t;

typedef struct // sem ID is the index into a semaphore array
{
   int pass_count;       // sem pass count
   q_t wait_q;           // process wait q
} sem_t;

typedef struct
   {
      int sender;                 // sender
      int send_tick;              // time sent
      int numbers[NUM_NUM];       // several integers can be useful as msgs
      char bytes[NUM_BYTE];       // some chars can be useful as msgs
   } msg_t;                       // msg type

typedef struct
   {
      int count, head, tail;
      msg_t msgs[NUM_MSG];
   } msg_q_t;

typedef struct
   {
      q_t wait_q;
      msg_q_t msg_q;
   } mbox_t;

typedef struct
  {
    int addr;
    int owner;
 } page_t;

//for command console
typedef struct
   {
      int count, head, tail;
      char cmd[NUM_MSG];
   } cmd_q;
   
typedef void (* func_ptr_t)(); // void-returning function pointer type

#endif
