//$Id: //depot/159/ASOS/7/proc.c#5 $
//$DateTime: 2009/04/24 20:33:39 $
//$Revision: #5 $

// proc.c, 159
// processes are here

#include "spede.h"   // for IO_DELAY() needed here below
#include "externs.h" // for cur_pid needed here below
#include "sys_calls.h"
#include "proc.h"
#include "isr.h" // for ShowStatusISR
#include "irq34.h"
#include "op_codes.h"
#include "ata_process.h"
#include "file_transfer.h"

void IdleProc() // a simulated user process for idle process 
{
        int i;
        while(1)
        {
	//cons_printf("0 ");
        for(i=0; i<1666000; i++) IO_DELAY(); // delay for about 1 sec
        }
}

void Init()
{
   char key;
   int print_pid,shell_pid,my_pid;
   int stdin_pid,stdout_pid,filesys_pid, file_txfr_pid;
   msg_t msg;
   
   my_pid = GetPid();
   //stdin_pid = Spawn(StdConsoleIn);
   //stdout_pid = Spawn(StdConsoleOut);
   //shell_pid = Spawn(Shell);
   //filesys_pid = Spawn(fs_process);

   printf("\n Setting TerminalInit");
   //TerminalInit(0);
   TerminalInit(0);

   //printf("\n Spawning Std in&out");
   //stdin_pid = Spawn(Stdin);
   //stdout_pid = Spawn(Stdout);
   
    //msg.numbers[1]=0; //terminal 0 or 1
    //msg.numbers[2]=0; // shell_pid but for now it means nothing
    //printf("\n Sending from Init to Stdin");
    //MsgSnd(stdin_pid,&msg);
 
    file_txfr_pid = Spawn(file_transfer);
    //msg.numbers[1]=0; //terminal 0 or 1
    //msg.numbers[2]=0; // shell_pid but for now it means nothing
    //printf("\n Sending from Init to Stdout");
    //MsgSnd(stdout_pid,&msg);
    
   /*
   // send shell the fs,stdin,stdout pid
   msg.numbers[0]=SPAWN2SHELL_IDS_req;
   msg.numbers[1]=stdin_pid;
   msg.numbers[2]=stdout_pid;
   msg.numbers[3]=filesys_pid;
   MsgSnd(shell_pid,&msg);
   
   // send file process the shell pid  
   msg.numbers[0]=SPAWN2FS_SHELLID_req;
   msg.numbers[1]=shell_pid;
   MsgSnd(filesys_pid,&msg);
  */
   while(1)
   {
	Sleep(1);
   }
}

void StdConsoleIn()
{
   msg_t msg;
   int shell_pid, echo_mode, my_pid, nof_chars;
   char inChar;
   char *inStr;
   my_pid = GetPid();
   while(1) // loop of service, servicing shell_pid only actually
   {
      nof_chars = 0;
      MsgRcv(&msg); // request from shell to get input
      shell_pid = msg.numbers[0];
      echo_mode = msg.numbers[1];
      inStr = &msg.bytes;
      do{
	    inChar = cons_getchar();
	    if(echo_mode) cons_putchar(inChar);
	    if(inChar == 13){
	       *inStr = '\0';
		cons_println();
	    	break;
	    } else {
	       *inStr = inChar;
	       inStr = inStr + 1;
	       nof_chars++;
	    }
      }while(1);
      msg.numbers[0] = my_pid;
      msg.numbers[1] = 1; //input attached
      msg.numbers[2] = nof_chars;
      MsgSnd(shell_pid, &msg); 	// serve user shell

   }
}


void StdConsoleOut()
{
	msg_t msg;
	int shell_pid, my_pid;
	my_pid = GetPid();
	while(1) // loop of service, servicing shell_pid only actually
	{
		MsgRcv(&msg); // request from shell to print on the console
		shell_pid = msg.numbers[0];
		cons_putstr (&msg.bytes);
		msg.numbers[0] = my_pid;
		msg.numbers[1] = 1; // operation completed
		MsgSnd(shell_pid, &msg); // send message to shell
	}
}

void Shell()
{
	char login[50], passwd[50], cmd_str[50];
	int stdin_pid, stdout_pid,filesys_pid,my_pid,dest_process_id;
	int cmd;
        msg_t msg;
        char temp_str[101];
	char prompt_str[101];
	// recieve message from SPAWN regarding fs information
	MsgRcv(&msg);
	my_pid = GetPid();
	cmd = msg.numbers[0]; //SPAWN2SHELL_IDS_res
	stdin_pid = msg.numbers[1];
	stdout_pid = msg.numbers[2];
        filesys_pid=msg.numbers[3];
	//msg.numbers[0] = SPAWN2SHELL_IDS_rsp;
	//MsgSnd(shell_pid, &msg);
	MsgRcv(&msg);
	cmd = msg.numbers[0];
	while (cmd != FS2SHELL_INITDONE_req) {
		MsgRcv(&msg);
		cmd = msg.numbers[0];
	}
	
	msg.numbers[0] = FS2SHELL_INITDONE_rsp;
	MsgSnd(filesys_pid, &msg);
	
	cons_printf ("\nCommand Console Started .... %d",my_pid);
	while(1)
	{
		while(1) // for login
		{
                        strcpy(msg.bytes,"\nLogin: \0");
			msg.numbers[0] = my_pid;
			MsgSnd(stdout_pid,&msg);
		        MsgRcv(&msg);
			        
                        strcpy(msg.bytes,"\0");
			msg.numbers[0] = my_pid;
			msg.numbers[1] = 1; //echo_mode on
		        MsgSnd(stdin_pid,&msg);
         		MsgRcv(&msg);
			strcpy(login,msg.bytes);
			
                        strcpy(msg.bytes,"Password: \0");
			msg.numbers[0] = my_pid;
         		MsgSnd(stdout_pid,&msg);
         		MsgRcv(&msg);

                        strcpy(msg.bytes,"\0");
			msg.numbers[0] = my_pid;
			msg.numbers[1] = 0; //echo_mode off
         		MsgSnd(stdin_pid,&msg);
  		        MsgRcv(&msg);
			strcpy(passwd,msg.bytes);

			if((strcmp(passwd,login)==0)) break;
                        strcpy(msg.bytes,"Illegal Login and Password!\n\0");
			msg.numbers[0] = my_pid;
         		MsgSnd(stdout_pid,&msg);
         		MsgRcv(&msg);
      		}
		strcpy(msg.bytes,"Login Success!\n\0");
		msg.numbers[0] = my_pid;
		MsgSnd(stdout_pid,&msg);
		MsgRcv(&msg);
		
		while(1){ //prompt
			strcpy(temp_str,"==>\0");
			strcpy(prompt_str,"\n\0");
			strcat(prompt_str,currrent_directory);
			strcat(prompt_str,temp_str);
			strcpy(msg.bytes,prompt_str);
			msg.numbers[0] = my_pid;
			MsgSnd(stdout_pid,&msg);
		        MsgRcv(&msg);
			        
                        strcpy(msg.bytes,"\0");
			msg.numbers[0] = my_pid;
			msg.numbers[1] = 1; //echo_mode on
		        MsgSnd(stdin_pid,&msg);
        		MsgRcv(&msg);
			strcpy(cmd_str,msg.bytes);
			printf("\nshell cmd str: %s",cmd_str);
			//command string recieved
			//check the string in wat command set
			switch (consoleCommandSet(cmd_str)) {
				case FS_CONSOLE_CMDS:	
					msg.numbers[0] = SHELL2FS_CMD_req;
					//strcpy(msg.bytes,cmd_str);
					MsgSnd(filesys_pid , &msg);
					//request from shell to print on 
					MsgRcv(&msg); 
					cmd = msg.numbers[0];//SHELL2FS_CMD_rsp

					break;
				case SYS_CONSOLE_CMDS:	break;
				default:		cons_printf("\nNot an Internal or External Function");
			}
			
		}
	}
}

int consoleCommandSet(char* bytes) {
	int i;
	char *cmd;
	char* rest_cmd;
	char filesystem_cmds[100][10] = {
	  {'l','s','\0'}, {'p','w','d','\0'},
	  {'c','d','\0'}, {'c','p','\0'},
	  {'m','k','d','i','r','\0'}, {'v','i','e','w','\0'},
	  {'i','n','f','o','\0'}, {'r','m','\0'},
	  {'m','o','v','e','\0'}, {'m','k','f','i','l','e','\0'},
	  {'t','i','m','e','\0'}, {'d','a','t','e','\0'}
	};//14
	int nofFcmds = 12;
	char system_cmds[100][10] = {};
	int nofScmds = 0;
	
	cmd = strtok_r(bytes," \n\t\f",&rest_cmd);
	for(i=0;i<nofFcmds;i++){
	  if(strcmp(cmd,filesystem_cmds[i]) == 0){
	    return FS_CONSOLE_CMDS;
	  }
	}
	
	for(i=0;i<nofScmds;i++){
	  if(strcmp(cmd,system_cmds[i]) == 0){
	    return SYS_CONSOLE_CMDS;
	  }
	}
	return 0;
}

void fs_process() {
	FF_IOMAN *pIoman;
	FFT_CONSOLE_SET	*pConsoleSet;		// FFTerm ConsoleSet
	
	char fscmd_str[100];
	char fscmd_str1[100];
	char* fscmd;
	char* rest_fscmd_str = NULL;
	
	int cmd,cmd_id;
	msg_t msg;
	int shell_pid, my_pid;
	my_pid = GetPid();
	// recieve message from SPAWN regarding shell information
	MsgRcv(&msg);
	cmd = msg.numbers[0]; //SPAWN2FS_SHELLID_res
	shell_pid = msg.numbers[1];
	//msg.numbers[0] = SPAWN2FS_SHELLID_rsp;
	//MsgSnd(shell_pid, &msg);
	if (pIoman=fs_init()) {
		cons_printf ("\nFile System Started .... %d",my_pid);
		cons_printf ("\nFile System console Commands Set ....");
		pConsoleSet = filesystem_consoleSet(pIoman);

		// Send message from SPAWN regarding shell information
		msg.numbers[0] = FS2SHELL_INITDONE_req;
		MsgSnd(shell_pid, &msg);
	
		MsgRcv(&msg);
		cmd = msg.numbers[0];
		while (cmd != FS2SHELL_INITDONE_rsp /*|| sender != shell_pid*/) {
			MsgRcv(&msg);
			cmd = msg.numbers[0];
		}

		while(1) // loop of service, servicing commands given to file system process
		{
			MsgRcv(&msg); // request from shell to print on the console
			cmd = msg.numbers[0];
			if(cmd == SHELL2FS_CMD_req) {
				strcpy(fscmd_str,msg.bytes);
				strcpy(fscmd_str1,msg.bytes);
				fscmd = strtok_r(fscmd_str1," \n\t\f",&rest_fscmd_str);
				FFTerm_ExecCommand(pConsoleSet,fscmd_str);
			
				msg.numbers[0] = SHELL2FS_CMD_rsp;
				MsgSnd(shell_pid, &msg);
			}
		}
	} else {
		cons_printf ("\n File System Issues \n");
	}
}