//$Id: //depot/159/ASOS/9/q_mgmt.c#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// q_mgmt.c, 159

#include "spede.h"
#include "types.h"
#include "externs.h"

int EmptyQ(q_t *p)
{
   	return (p->count == 0); //return 1 if queue is empty, else return 0
}

int FullQ(q_t *p)
{
	return(p->count==Q_SIZE-1); //return 1 if queue is full, else return 0

}

void InitQ(q_t *p)
{
	p->count = 0;	//initialize count, head and tail of the queue  to 0
	p->head = 0;
	p->tail = 0;

}

int DeQ(q_t *p)// return -1 if q is empty
{
   	int pid;
	if(!EmptyQ(p))//if Queue is not empty
	{
		pid = p->q[p->head];
		p->head = p->head + 1;
		if(p->head == Q_SIZE)//if head is at index 20 of queue then reset head to 0
			p->head = 0;
		p->count = p->count - 1;
		return pid;
	}
	else return -1;	//return -1 if q is empty


}

void EnQ(int element, q_t *p)
{
	if(!FullQ(p))//if Queue is not full
	{
		p->q[p->tail] = element;
		p->tail = p->tail + 1;
		if(p->tail==Q_SIZE)//if tail is at index 20 then reset tail to 0
			p->tail = 0;
		p->count = p->count + 1;
	}
}

