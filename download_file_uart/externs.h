//$Id: //depot/159/ASOS/9/externs.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// externs.h, 159

#ifndef _EXTERNS_H_
#define _EXTERNS_H_

#include "types.h"  // q_t, pcb_t, NUM_PROC, USER_STACK_SIZE

#define  SW_INTRS_HD	1
extern char user_stacks[USER_STACK_SIZE]; // proc run-time stacks
extern int hd_intr_occurd;
#endif
