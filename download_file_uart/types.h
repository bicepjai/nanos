//$Id: //depot/159/ASOS/9/types.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// types.h, 159

#ifndef _TYPES_H_
#define _TYPES_H_

#include "trap.h"            // defines tf_t, trap frame type
#include "spede.h"

#define Q_SIZE 20            // queuing capacity
#define USER_STACK_SIZE 524288 // # of bytes for runtime stack
#define G1 0x40000000 // code at 1G
#define G3 0xC0000000 // stack ends at 3G-1

typedef unsigned char	u8;	/* 8 bits */
typedef unsigned short	u16;	/* 16 bits */
typedef unsigned long	u32;	/* 32 bits */

typedef struct // queue type: head & tail pointing array of PIDs
{
   int count, head, tail, q[Q_SIZE];
} q_t;

typedef void (* func_ptr_t)(); // void-returning function pointer type

#endif
