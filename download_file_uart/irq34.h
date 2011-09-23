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

#define NUM_TERM 1            // we will at most use 2 terminals
#define CHAR_Q_SIZE 256        // char q size (in/out/echo q in terminal_t)
#define FT_TERM	0	//the one used for file transfers between target and host

typedef struct
{
   u8 q[CHAR_Q_SIZE];
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
void TerminalInit_polledio(int); // called by Init() to init terminals

u8 get_serial_char();
u8 get_serial_char_polledio();
void Stdin(); // a process that handles terminal keyboard input
void Stdin_polledio();
u8 StdinChar(int term_num);

void put_serial_buf(u8 *p1, int n);
void put_serial_char(u8 ch);
void StdoutChar(int, u8); // subroutine of StdoutString()
void Stdout();

void IRQ34ISR(); // ISR for IRQ 3 and 4
void IRQ34ISROutChar(int); // subroutine of IRQ34ISR() to output a char
void IRQ34ISRInChar(int); // subroutine of IRQ34ISR() to input a char

u8 DeCharQ(char_q_t *);
void EnCharQ(u8, char_q_t *);
int CharEmptyQ(char_q_t *);
int CharFullQ(char_q_t *);
void InitCharQ(char_q_t *);

#endif

