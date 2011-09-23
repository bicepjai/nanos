//$Id: //depot/159/ASOS/9/externs.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// externs.h, 159

#ifndef _EXTERNS_H_
#define _EXTERNS_H_

#include "types.h"  // q_t, pcb_t, NUM_PROC, USER_STACK_SIZE

#define  SW_INTRS_HD	1

extern int cur_pid,sys_tick; // PID of running process, -1 is none running
extern q_t ready_q, avail_q,sleep_q,avail_sem_q;    // ready to run, not used proc IDs
extern pcb_t pcbs[NUM_PROC];    // process table
extern char user_stacks[NUM_PROC][USER_STACK_SIZE]; // proc run-time stacks

extern sem_t sems[NUM_SEM];
extern mbox_t mboxes[NUM_PROC];
extern int print_sid;
extern page_t pages[NUM_PAGE];
extern int kernel_pd_addr;

extern int hd_intr_occurd;

extern char ch_frm_uart;
extern int ch_frm_uart_status;

extern char* currrent_directory;
#endif
