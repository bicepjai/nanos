//$Id: //depot/159/ASOS/9/sys_calls.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $


#ifndef _SYS_CALLS_H_
#define _SYS_CALLS_H_

int GetPid();
void Sleep(int); 
int Spawn(func_ptr_t);
int SemInit();
void SemWait();
void SemPost();
void MsgSnd(int,msg_t*);
void MsgRcv(msg_t*);
int Fork(int,int);
int Wait(int*);
void Exit(int);

#endif

