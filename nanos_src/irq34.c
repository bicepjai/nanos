//$Id: //depot/159/ASOS/7/irq34.c#4 $
//$DateTime: 2009/04/24 20:33:39 $
//$Revision: #4 $

#include "irq34.h"
#include "q_mgmt.h"
#include "spede.h"
#include "op_codes.h"
#include "cdefs.h"

// set the first 2 fields of structure, the rest is initialized by
// C-compiler as null values.
// COM1~8 io_base: 0x3f8 0x2f8 0x3e8 0x2e8 0x2f0 0x3e0 0x2e0 0x260
// COM1~8 irq: 4 3 4 3 4 3 4 3
/*
#define   IRQ_COM2	3
#define   IRQ_COM1	4

#define  COM1_IOBASE	0x03F8
#define  COM2_IOBASE	0x02F8
#define  COM3_IOBASE	0x03E8
*/
terminal_t terminals[NUM_TERM]=
{
   {COM2_IOBASE, 3}, // IO port base and IRQ #
   //{COM3_IOBASE, 4},
   //{COM1_IOBASE, 4},// for system used at home
};

// setup data structures and UART of a terminal. Clear char queues,
// and preset the bounded buffer semaphore. Put UART to interrupt mode:
// clear IER, enable TXRDY & RXRDY for interrupts.
void TerminalInit(int term_num)
{
   int BAUD_RATE = 9600;
   int divisor = 115200 / BAUD_RATE;

// first setup our vars
   terminals[term_num].echo_mode = TRUE;
   terminals[term_num].missed_intr = TRUE;

// Use a pair of sems. One limits available space in the output queue
// (terminal display), the other limits chars that are typed from the
// terminal. As part of initialization, the count of the output queue
// is set to the capacity of the char queue
   terminals[term_num].out_sid = SemInit(0); // a circular q, capacity CHAR_Q_SIZE
   terminals[term_num].in_sid = SemInit(0);

   InitCharQ(&terminals[term_num].in_q);   // initially empty
   InitCharQ(&terminals[term_num].out_q);  // initially empty
   InitCharQ(&terminals[term_num].echo_q); // initially empty

// then setup the terminal for 7-E-1 at 9600 baud
// abbrevs:
// CFCR Char Format Control Reg, MSR Modem Status Reg, IIR Intr Indicator Reg
// MCR Modem Control Reg, IER Intr Enable Reg, LSR Line Status Reg
// ERXRDY Enable Recv Ready, ETXRDY Enable Xmit Ready
// LSR_TSRE Line Status Reg Xmit+Shift Regs Empty
   outportb(terminals[term_num].io_base + CFCR, CFCR_DLAB); // CFCR_DLAB is 0x80
   outportb(terminals[term_num].io_base + BAUDLO,LOBYTE(divisor));
   outportb(terminals[term_num].io_base + BAUDHI,HIBYTE(divisor));
   //outportb(terminals[term_num].io_base + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS); //7-E-1
    outportb(terminals[term_num].io_base + CFCR, CFCR_8BITS); //8-N-1
   outportb(terminals[term_num].io_base + IER,0);
// raise DTR & RTS of the serial port to start read/write
   outportb(terminals[term_num].io_base + MCR, MCR_DTR | MCR_RTS | MCR_IENABLE);
   IO_DELAY();
   outportb(terminals[term_num].io_base + IER, IER_ERXRDY | IER_ETXRDY);
   IO_DELAY();
// A null-terminated test message is sent to display to the terminal.
// "ESC *" will clear screen on TVI 910, but we use newlines for portability
   //StdoutString(term_num, "\n\n\nWill I dream, Dave? (2001 Space Odyssey)\n\n\n\0");
}

//*********************************************************************
// this serves as a shell's stdout, the terminal output/display process
//*********************************************************************

//put_serial_char test process
void Stdout()
{
   msg_t msg;
   int term_num, shell_pid;
   int i=0;

   MsgRcv(&msg); // msg provided by Init() for this process to know:
   term_num = msg.numbers[1];  // need info which terminal structure to use
   shell_pid = msg.numbers[2]; // need to know mbox ID of shell servicing
   printf("\nStd out ready");
   while(1) // service loop, servicing shell_pid only actually
   {
      //MsgRcv(&msg);
       //printf(".");
      //if(msg.numbers[3]==ECHO_OFF)
	//terminals[term_num].echo_mode=FALSE;
      
      put_serial_char('A'+i); // bytes is str to display
      i++;
      if(i>24) i=0;
      //MsgSnd(shell_pid, &msg);                // completion msg, content not important
   }
}

void put_serial_buf(UCHAR *p1, int n)
{
   UCHAR *p;
   printf("\n put_serial_buf");
   if(p1){
     printf("\n put_serial_buf not null");
    for(p = p1; n>=0 ; p++,n--)
      StdoutChar(FT_TERM, *p);
   }
}

void put_serial_char(char ch)
{
      StdoutChar(FT_TERM, ch);
}

void StdoutChar(int term_num, char ch)
{
    EnCharQ(ch, &terminals[term_num].out_q);
    if(terminals[term_num].missed_intr) IRQ34ISROutChar(term_num);
    SemWait(terminals[term_num].out_sid);   
}

//*********************************************************************
// This serves as a shell's stdin, the terminal input/keyboard process
//*********************************************************************
//process to test the get_serial_char
void Stdin()
{
   msg_t msg;
   int term_num, shell_pid;
   char ch;
   MsgRcv(&msg); // sent from Init() for info below:

   term_num = msg.numbers[1];
   shell_pid = msg.numbers[2];
   printf("....\n");
   while(1) // loop of service, servicing shell_pid only actually
   {
      //MsgRcv(&msg); // request from shell to read terminal keyboard input
      //bzero(msg.bytes, NUM_BYTE);       // filled with zeroes (nulls)
      ch = get_serial_char();
      printf("_%c_",ch);
   }
}

char get_serial_char() {
  return StdinChar(FT_TERM);
}

// Fill a string buffer with char data. Depending on echo_mode, can read
// up to capacity of str, or till NEWLINE char (null termination added)
char StdinChar(int term_num)
{
   char ch;
   SemWait(terminals[term_num].in_sid); // wait terminal key-in to SemPostISR()
   ch = DeCharQ(&terminals[term_num].in_q); // get a char from in queue
   return ch;
}


//****************************** LOWER HALF ******************************
//********************************* BELOW ********************************
// upon either IRQ 3 or 4, below called by Kernel()
//************************************************************************
//************************************************************************
void IRQ34ISR()
{
   int status;
// when an interrupt occurs, check all UART's to see which one needs service
// if xmitter empty but no no char queued, we missed an interrupt, disable
// them at serial port
      while( !(IIR_NOPEND & (status = inportb(terminals[FT_TERM].io_base + IIR))))
      {
         switch(status)
         {
            case IIR_RXRDY: IRQ34ISRInChar(FT_TERM); break; // recv buffer has data
            case IIR_TXRDY: IRQ34ISROutChar(FT_TERM); break; // xmit buffer has space
         }
      } // while UART needs servicing

// We might have to echo the char we just received. IRQ34ISRInChar() can insert
// to echo_q, however only call IRQ34ISROutChar() if TBE is true. Can't do it
// inside "while" loop in case trigger includes TXRDY and RXRDY. We might call
// IRQ34ISROutChar() for RXRDY case (to handle echo), then call again for TXRDY
// case, which we must do.
// Calling out here means "after all required interrupt processing is there
// anything to echo?" You can trigger this by typing quickly, when chars are
// being echoed. If LSR_TXRDY gives errors, then try LSR_TSRE instead.
      if(terminals[FT_TERM].missed_intr) IRQ34ISROutChar(FT_TERM);
}

//
// remove char from out_q and send it out via UART
//
void IRQ34ISROutChar(int term_num)
{
   char ch;

   terminals[term_num].missed_intr = FALSE;
    
   if(CharEmptyQ(&terminals[term_num].out_q) /*&&
      CharEmptyQ(&terminals[term_num].echo_q)*/)
   {
// nothing to display but the xmitter is ready, missed a chance to send a char
      terminals[term_num].missed_intr = TRUE; // set it so can use output later
      return;                                 // nothing more to do more
   }
      ch = DeCharQ(&terminals[term_num].out_q); // get a char from out queue
      //printf("\nch <= %c",ch);
      outportb(terminals[term_num].io_base + DATA, ch); // output ch to UART
      SemPostISR(terminals[term_num].out_sid);  // post a space (char)
}

// read incoming char from UART and store into input queue, if there's no
// room, place a bell character to echo queue to send a alarm sound to the
// terminal instead
void IRQ34ISRInChar(int term_num)
{
   char ch,status;
   status = inportb(terminals[term_num].io_base + LSR);
// what's keyed-in, mask with 127, to get 7-bit ASCII char
   ch = inportb(terminals[term_num].io_base + DATA);// & 0x7F;
// Error inside UART. We've cleared the condition by reading it.
// However, the character is messed up, replace it.
   printf("\n0x%x-->%c",ch,ch);
   if(!QBIT_ANY_ON(status, LSR_FE | LSR_OE)) {  		//8-N-1
   //if(!QBIT_ANY_ON(status, LSR_FE | LSR_PE | LSR_OE)) {	//7-E-1
	if(CharFullQ(&terminals[term_num].in_q)) // in queue full (typed too fast?)
	{
	}
	else // add char to in queue, signal data now avail
	{
	      EnCharQ(ch, &terminals[term_num].in_q); // put ch into in queue
	      SemPostISR(terminals[term_num].in_sid); // post data-avail semaphore
	}
   }
}

//******************************************************************************
// q_mgmt.c should have these, but types.h would have to have to include irq34.h for
// the char_q_t types defined in it
//******************************************************************************

char DeCharQ(char_q_t *p) // return 0/null if queue is empty
{
   char ch;

   if(CharEmptyQ(p))
   {
      cons_printf("Char queue is empty, can't dequeue!\n");
      return 0; // return null char instead of -1 since the return type is char
   }
   ch = p->q[p->head];
   p->head++;
   if(p->head == CHAR_Q_SIZE) p->head = 0; // wrap
   p->count--;

   return ch;
}

void EnCharQ(char ch, char_q_t *p) // append ch to p->q[tail] if queue has space
{
   if(CharFullQ(p))
   {
      cons_printf("Char queue is full, can't enqueue!\n");
      return;
   }
   p->q[p->tail] = ch;
   p->tail++;
   if(p->tail == CHAR_Q_SIZE) p->tail = 0; // wrap
   p->count++;
}

int CharEmptyQ(char_q_t *p) // empty when count is zero
{
   return(p->count == 0);
}

int CharFullQ(char_q_t *p) // full when count is max possible
{
   return(p->count == CHAR_Q_SIZE);
}

void InitCharQ(char_q_t *p)
{
   bzero(p, sizeof(char_q_t));
}

