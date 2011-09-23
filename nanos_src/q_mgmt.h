//$Id: //depot/159/ASOS/9/q_mgmt.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// q_mgmt.h, 159

#ifndef _Q_MGMT_H_
#define _Q_MGMT_H_

#include "types.h" 

int EmptyQ(q_t *);
int FullQ(q_t *);
void InitQ(q_t *);
int DeQ(q_t *);
void EnQ(int, q_t *);
void AddSleepQ();
void MsgEnQ(msg_t*, msg_q_t*);
msg_t* MsgDeQ(msg_q_t*);
int MsgFullQ(msg_q_t*);
int MsgEmptyQ(msg_q_t*);
 

#endif

