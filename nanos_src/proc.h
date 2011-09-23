//$Id: //depot/159/ASOS/9/proc.h#2 $
//$DateTime: 2009/05/08 19:26:30 $
//$Revision: #2 $

// proc.h, 159

#ifndef _PROC_H_
#define _PROC_H_

// defines for command message description
#define SPAWN2FS_SHELLID_req		0
#define SPAWN2FS_SHELLID_rsp		1
#define SPAWN2SHELL_IDS_req		2
#define SPAWN2SHELL_IDS_rsp		3

#define FS2SHELL_INITDONE_req		4
#define FS2SHELL_INITDONE_rsp		5
#define SHELL2FS_CMD_req		6
#define SHELL2FS_CMD_rsp		7

//commands for console sets
#define FS_CONSOLE_CMDS			100
#define SYS_CONSOLE_CMDS		101

void IdleProc();
void Init();
void Producer();
void Consumer();

void Shell();
void StdConsoleOut();
void StdConsoleIn();

int consoleCommandSet(char* bytes);
#endif
