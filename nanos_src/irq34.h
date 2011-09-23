//$Id: //depot/159/ASOS/7/irq34.h#4 $
//$DateTime: 2009/04/24 20:33:39 $
//$Revision: #4 $

// irq34.h  159 phase 6

#ifndef _IRQ34_H_
#define _IRQ34_H_

#include <spede/string.h>
#include <spede/ctype.h>           // isprint(), isspace()
#include <spede/sys/ascii.h>       // CH_BACKSPACE, etc
#include <spede/machine/io.h>
#include <spede/machine/rs232.h>   // for flags needed below

#include "types.h"            // for different types defined
#include "sys_calls.h"        // for SemWait() below
#include "isr.h"              // for SemPostISR() below
#include "q_mgmt.h"
#include "cdefs.h"

#define NUM_TERM 1            // we will at most use 2 terminals
#define CHAR_Q_SIZE 20        // char q size (in/out/echo q in terminal_t)
#define FT_TERM	0	//the one used for file transfers between target and host

typedef struct
{
   char q[CHAR_Q_SIZE];
   int head, tail, count;
} char_q_t;

typedef struct
{
   int
      io_base,      // different in io_base but..
      irq,          // same IRQ # 4 for COM1/3/5/7 & IRQ # 3 for 2/4/6/8
      in_sid,       // IRQ34ISRInChar() SemPostISR() with this
      out_sid,      // IRQ34ISROutChar() SemPostISR() with this
      echo_mode,    // when entering password, disable echo (into echo_q)
      missed_intr;  // mark it for initial/additional ISR putc/getc opeartion

   char_q_t
      in_q,         // 3 terminal queues: keyed-in, display, echo
      out_q,
      echo_q;

} terminal_t;

void TerminalInit(int); // called by Init() to init terminals

char get_serial_char();
void Stdin(); // a process that handles terminal keyboard input
char StdinChar(int term_num);

void put_serial_buf(UCHAR *p1, int n);
void put_serial_char(char ch);
void StdoutChar(int, char); // subroutine of StdoutString()
void Stdout();

void IRQ34ISR(); // ISR for IRQ 3 and 4
void IRQ34ISROutChar(int); // subroutine of IRQ34ISR() to output a char
void IRQ34ISRInChar(int); // subroutine of IRQ34ISR() to input a char

char DeCharQ(char_q_t *);
void EnCharQ(char, char_q_t *);
int CharEmptyQ(char_q_t *);
int CharFullQ(char_q_t *);
void InitCharQ(char_q_t *);

#endif

