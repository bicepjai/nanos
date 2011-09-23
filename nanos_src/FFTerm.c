/*****************************************************************************
 *  FFTerm - Simple & Platform independent, Thread-Safe Terminal/Console     *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FFTerm.        *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/ffterm for more information.                       *
 *  Or  http://ffterm.googlecode.com/ for latest releases and the wiki.      *
 *****************************************************************************/

#include <spede/stdlib.h>
#include <spede/stdio.h>
#include <spede/string.h>
#include "FFTerm.h"

#define _FFTERM_CR_CHAR_	-1
#define _FFTERM_LF_CHAR_	-2
#define _FFTERM_DEAD_CHAR_	-4096

/**
 *	@file		FFTerm.c
 *	@author		James Walmsley
 *	@ingroup	FFTERM
 *
 *	@defgroup	FFTERM	FFTerm
 *	@brief		A platform independent console/terminal program.
 *
 *	Provides a simple and extendable terminals running over multiple, thread-safe
 *	independent consoles.
 *
 **/

static FFT_COMMAND	*FFTerm_GetCmd			(FFT_CONSOLE_SET *pConsoleSet, const FF_T_INT8 *pa_cmdName, FF_T_SINT32 *pError);
static FF_T_SINT32	 FFTerm_SortCommands		(FFT_CONSOLE_SET *pConsoleSet);
static FF_T_SINT32	 FFTerm_GenerateArguments	(FF_T_INT8 *strCmdLine, FF_T_INT8 **ppArgv);
static void		 FFTerm_tolower			(FF_T_INT8 *string);


FFT_CONSOLE_SET *FFTerm_CreateConsoleSet(FF_T_SINT32 *pError) {
	FFT_CONSOLE_SET *pConsoleSet = (FFT_CONSOLE_SET *) malloc(sizeof(FFT_CONSOLE_SET));
	
	if(pError) {
		*pError = FFT_ERR_NONE;	// Initialise the Error to no error.
	}

	if(pConsoleSet) {
		pConsoleSet->pCommands		= (FFT_COMMAND *) NULL;
		pConsoleSet->Mode		= FFT_MODE_DEFAULT;
		return pConsoleSet;
	}

	if(pError) {
		*pError = FFT_ERR_NOT_ENOUGH_MEMORY;
	}

	return (FFT_CONSOLE_SET *) NULL;
}

/**
 *	@public
 *	@brief	Adds a simple command to the provided FFTerm console.
 *
 *	Add's commands with functions of the form:
 *	int cmd_function(int argc, char **argv) {
 *		// Command code goes here.
 *		// Just like the main() function of a standard C program.
 *	}	
 *
 *	@param	pConsoleSet		The FFT_CONSOLE_SET object pointer.
 *	@param	pa_cmdName		A string of the command name.
 *	@param	pa_fnCmd		Function pointer to a command handler. (The name of the function for this command).
 *	@param	pa_ErrTable		A pointer to an Error Code Table. This can be NULL.
 *	@param	pa_ErrTable		See the documentation regarding FFT_ERR_TABLE definitions.
 *
 *	@return	FFT_ERR_NONE on Success, a negative error code on failure.
 *	@return	A string describing the Error code can be got by calling FFTerm_GetErrMessage().
 *
 **/
FF_T_SINT32 FFTerm_AddCmd(FFT_CONSOLE_SET *pConsoleSet, const FF_T_INT8 *pa_cmdName, FFT_FN_COMMAND pa_fnCmd, const FFT_ERR_TABLE *pa_ErrTable) {
	
	FFT_COMMAND *pCommand;

	if(!pConsoleSet) {
		return FFT_ERR_NULL_POINTER;
	}

	if(strlen(pa_cmdName) > FFT_MAX_CMD_NAME) {
		return FFT_ERR_CMD_NAME_TOO_LONG;
	}

	if(!FFTerm_GetCmd(pConsoleSet, pa_cmdName, NULL)) {
		
		if(pConsoleSet->pCommands == NULL) {
			pConsoleSet->pCommands = (FFT_COMMAND *) malloc(sizeof(FFT_COMMAND));
			pCommand = pConsoleSet->pCommands;
		} else {
			pCommand = pConsoleSet->pCommands;
			while(pCommand->pNextCmd != NULL) {	// Traverse to the end of the commands list.
				pCommand = pCommand->pNextCmd;
			}
			pCommand->pNextCmd = (FFT_COMMAND *) malloc(sizeof(FFT_COMMAND));
			pCommand = pCommand->pNextCmd;
		}
		
		if(pCommand) {
			pCommand->pNextCmd	= (FFT_COMMAND *) NULL;
			pCommand->fnCmd		= pa_fnCmd;
			pCommand->fnCmdEx	= NULL;
			pCommand->ErrTable  = pa_ErrTable;
			strcpy(pCommand->cmdName, pa_cmdName);
			FFTerm_SortCommands(pConsoleSet);	// Keep commands sorted in Alphanumeric order.
		}

		return FFT_ERR_NONE;
	}

	return FFT_ERR_CMD_ALREADY_EXISTS;
}

/**
 *	@public
 *	@brief	Adds an extended command to the provided FFTerm console.
 *
 *	Add's commands with functions of the form:
 *	int cmd_function(int argc, char **argv, void *pParam) {
 *		// Command code goes here.
 *		// pParam is passed as an argument.
 *	}
 *
 *	@param	pConsoleSet		The FFT_CONSOLE_SET object pointer.
 *	@param	pa_cmdName		A string of the command name.
 *	@param	pa_fnCmd		Function pointer to a command handler. (The name of the function for this command).
 *	@param	pa_ErrTable		A pointer to an Error Code Table. This can be NULL.
 *	@param	pa_ErrTable		See the documentation regarding FFT_ERR_TABLE definitions.
 *	@param	pParam			A pointer to anything that the command might need.
 *
 *	@return	FFT_ERR_NONE on Success, a negative error code on failure.
 *	@return	A string describing the Error code can be got by calling FFTerm_GetErrMessage().
 *
 **/
FF_T_SINT32 FFTerm_AddExCmd(FFT_CONSOLE_SET *pConsoleSet, const FF_T_INT8 *pa_cmdName, FFT_FN_COMMAND_EX pa_fnCmd, const FFT_ERR_TABLE *pa_ErrTable, void *pParam) {
	
	FFT_COMMAND *pCommand;

	if(!pConsoleSet) {
		return FFT_ERR_NULL_POINTER;
	}

	if(strlen(pa_cmdName) > FFT_MAX_CMD_NAME) {
		return FFT_ERR_CMD_NAME_TOO_LONG;
	}

	if(!FFTerm_GetCmd(pConsoleSet, pa_cmdName, NULL)) {
		
		if(pConsoleSet->pCommands == NULL) {
			pConsoleSet->pCommands = (FFT_COMMAND *) malloc(sizeof(FFT_COMMAND));
			pCommand = pConsoleSet->pCommands;
		} else {
			pCommand = pConsoleSet->pCommands;
			while(pCommand->pNextCmd != NULL) {	// Traverse to the end of the commands list.
				pCommand = pCommand->pNextCmd;
			}
			pCommand->pNextCmd = (FFT_COMMAND *) malloc(sizeof(FFT_COMMAND));
			pCommand = pCommand->pNextCmd;
		}
		
		if(pCommand) {
			pCommand->pNextCmd	= (FFT_COMMAND *) NULL;
			pCommand->fnCmdEx	= pa_fnCmd;
			pCommand->fnCmd		= NULL;
			pCommand->ErrTable	= pa_ErrTable;
			pCommand->CmdExParam	= pParam;
			strcpy(pCommand->cmdName, pa_cmdName);
			FFTerm_SortCommands(pConsoleSet);	// Keep commands sorted in Alphanumeric order.
		}
		return FFT_ERR_NONE;
	}
	return FFT_ERR_CMD_ALREADY_EXISTS;
}

/**
 *	@public
 *	@brief	Removes a command from the specified FFTerm console.
 *
 *	@param	pConsoleSet		The FFT_CONSOLE_SET object pointer.
 *	@param	pa_cmdName		String of the command to remove.
 *
 **/
FF_T_SINT32 FFTerm_RemoveCmd(FFT_CONSOLE_SET *pConsoleSet, const FF_T_INT8 *pa_cmdName) {

	FFT_COMMAND *pCommand;
	FFT_COMMAND *pRmCmd;
	FF_T_SINT32	Error = FFT_ERR_NONE;
	
	if(!pConsoleSet) {
		return FFT_ERR_NULL_POINTER;
	}

	pCommand = pConsoleSet->pCommands;
	pRmCmd = FFTerm_GetCmd(pConsoleSet, pa_cmdName, &Error);

	if(Error) {
		return Error;
	}

	if(pRmCmd) {

		if(pCommand == pRmCmd) {
			pConsoleSet->pCommands = pRmCmd->pNextCmd;
			free(pRmCmd);
			return FFT_ERR_NONE;
		}

		while(pCommand != NULL) {
			if(pCommand->pNextCmd == pRmCmd) {
				pCommand->pNextCmd = pRmCmd->pNextCmd;
				free(pRmCmd);
				return FFT_ERR_NONE;
			}
			pCommand = pCommand->pNextCmd;
		}
	}

	return FFT_ERR_CMD_NOT_FOUND;
}

FF_T_SINT32	FFTerm_ExecCommand(FFT_CONSOLE_SET *pConsoleSet, FF_T_INT8 *strCommand) {
	
	FFT_COMMAND *pCommand;
	FF_T_SINT32	Error = FFT_ERR_NONE;
	FF_T_SINT32 Result = FFT_ERR_NONE;
	FF_T_SINT32	Argc;
	FF_T_INT8	*pArgs[FFT_MAX_CMD_LINE_ARGS];
	FF_T_INT8	strCommandModifiable[FFT_MAX_CMD_LINE_INPUT];

	strcpy(strCommandModifiable, strCommand);

	// Process Command Line into Arguments
	Argc = FFTerm_GenerateArguments(strCommandModifiable, (FF_T_INT8**)pArgs);

	pCommand = FFTerm_GetCmd(pConsoleSet, pArgs[0], &Error);
	
	if(pCommand) {
	  //printf("\n im here 1 %s",strCommand);
		pArgs[0] = pCommand->cmdName;	// Ensure argv[0] is formatted like the registered command name.
		if(pCommand->fnCmd || pCommand->fnCmdEx) {
			if(pCommand->fnCmd) {
				// Execute the relevant command!
				Result = (FF_T_SINT32) pCommand->fnCmd((int)Argc, pArgs);
				if(Result == FFT_KILL_CONSOLE) {
					pConsoleSet->bKill = FF_TRUE;
					//break;
				}
			} else if (pCommand->fnCmdEx) {
				Result = (FF_T_SINT32) pCommand->fnCmdEx((int)Argc, pArgs, pCommand->CmdExParam);
				if(Result == FFT_KILL_CONSOLE) {
					pConsoleSet->bKill = FF_TRUE;
					//break;
				}
			} else {
				Result = 0;
				//fprintf(pConsoleSet->pStdOut, "A command was found, but no registered executable address was found.\n");
			}

			if(Result) {
				if(pCommand->ErrTable) {
					//fprintf(pConsoleSet->pStdOut, "Command returned with message: \"%s\"\n", FFTerm_LookupErrMessage(pCommand->ErrTable, Result));
				} else {
					//fprintf(pConsoleSet->pStdOut, "Command returned with Code (%ld)\n", Result);
				}
			}
		}
	} else {
		printf("\n im here 2 %s",strCommand);
		if(Error == FFT_ERR_CMD_NOT_FOUND) {
			if(strlen(strCommand) > 0) {
				//fprintf(pConsoleSet->pStdOut, "'%s' is not recognised as an internal or external command.\n", pArgs[0]);
			}
		} else {
			if(strlen(strCommand) > 0) {
				//fprintf(pConsoleSet->pStdOut, "%s\n", FFTerm_GetErrMessage(Error));
			}
		}
	}

	return Result;

}


/**
 *	@private
 **/
static FFT_COMMAND *FFTerm_GetCmd(FFT_CONSOLE_SET *pConsoleSet, const FF_T_INT8 *pa_cmdName, FF_T_SINT32 *pError) {
	
	FFT_COMMAND *pCommand;
	FF_T_INT8	pa_cmdLower[FFT_MAX_CMD_NAME + 1];
	FF_T_INT8	cmdLower[FFT_MAX_CMD_NAME + 1];

	//printf("\ncommand name %s",pa_cmdName);
	if(pError) {
		*pError = FFT_ERR_NONE;
	}
	
	if(!pConsoleSet) {
		if(pError) {
			*pError = FFT_ERR_NULL_POINTER;
		}
		return NULL;
	}

	if(strlen(pa_cmdName) > (FFT_MAX_CMD_NAME)) {
		if(pError) {
			*pError = FFT_ERR_CMD_NOT_FOUND;
		}
		return NULL;
	}

	strcpy(pa_cmdLower, pa_cmdName);
	FFTerm_tolower(pa_cmdLower);		// Remove case from input command name.

	pCommand = pConsoleSet->pCommands;

	if(pCommand) {
		while(pCommand != NULL) {
			strcpy(cmdLower, pCommand->cmdName);
			FFTerm_tolower(cmdLower);
			if(strcmp(cmdLower, pa_cmdLower) == 0) {
				return pCommand;
			}
			pCommand = pCommand->pNextCmd;
		}
	}

	if(pError) {
		*pError = FFT_ERR_CMD_NOT_FOUND;
	}

	return (FFT_COMMAND *) NULL;
}

/**
 *	@private
 **/
static FF_T_BOOL FFTerm_isSpace(FF_T_INT8 c) {
	if(c == ' ' || c == '\t') {
		return FF_TRUE;
	}

	return FF_FALSE;
}

/**
 *	@private
 **/
static FF_T_SINT32 FFTerm_GenerateArguments(FF_T_INT8 *strCmdLine, FF_T_INT8 **ppArgv) {
	FF_T_UINT32	iArgc;		// Argument Counter
	FF_T_UINT32	iCmdLineLen	= strlen(strCmdLine);
	FF_T_INT8	*pCmdLineEnd = (strCmdLine +iCmdLineLen);		// Pointer to the end of the CmdLine
	
	for(iArgc = 0; iArgc < FFT_MAX_CMD_LINE_ARGS;) {
		// Skip past any white space at the beginning.
		while((strCmdLine < pCmdLineEnd) && (FFTerm_isSpace(*strCmdLine))) {
			strCmdLine++;
		}

		ppArgv[iArgc] = strCmdLine;
		
		// Allow arguments with spaces in them via inverted commas
		if(*strCmdLine == '\"') {
			strCmdLine++;
			ppArgv[iArgc] = strCmdLine;
			while((*strCmdLine) && (*strCmdLine != '\"')) {
				strCmdLine++;
			}
			// Skip past the " character
			*strCmdLine = '\0';
			strCmdLine++;
		}

		while((*strCmdLine) && (!FFTerm_isSpace(*strCmdLine))) {
			strCmdLine++;
		}

		*strCmdLine = '\0';
		strCmdLine++;

		// Check for end of command line.

		if(strCmdLine >= pCmdLineEnd) {
			break;
		} else {
			iArgc++;
		}
	}

	iArgc++;
	if(iArgc > FFT_MAX_CMD_LINE_ARGS) {
		iArgc = FFT_MAX_CMD_LINE_ARGS;
	}

	return iArgc;
}


/**
 *	@private
 **/
const FF_T_INT8 *FFTerm_LookupErrMessage(const FFT_ERR_TABLE *pa_pErrTable, FF_T_SINT32 iErrorCode) {
	const FFT_ERR_TABLE *pErrTable = pa_pErrTable;
	while (pErrTable->strErrorString){
        if (pErrTable->iErrorCode == iErrorCode) {
            return pErrTable->strErrorString;
        }
		pErrTable++;
    }
	return pa_pErrTable->strErrorString;
}


/**
 *	@private
 **/
FF_T_SINT32	FFTerm_SortCommands(FFT_CONSOLE_SET *pConsoleSet) {
	FFT_COMMAND *pCommand = pConsoleSet->pCommands;
	FFT_COMMAND *pCmd1, *pCmd2;
	FF_T_UINT32	iSwaps;
	FF_T_SINT32	iResult;

	FF_T_INT8	CmdLower[FFT_MAX_CMD_NAME + 1];
	FF_T_INT8	NextCmdLower[FFT_MAX_CMD_NAME + 1];

	do {
		iSwaps = 0;
		pCommand = pConsoleSet->pCommands;
		while(pCommand && pCommand->pNextCmd) {
			strcpy(CmdLower, pCommand->cmdName);
			strcpy(NextCmdLower, pCommand->pNextCmd->cmdName);
			FFTerm_tolower(CmdLower);
			FFTerm_tolower(NextCmdLower);
			iResult = strcmp(CmdLower, NextCmdLower);
			if(iResult == 1) {	// Put pCommand after the next command.
				iSwaps++;
				pCmd1 = pConsoleSet->pCommands;
				if(pCmd1 == pCommand) {
					pCmd2 = pCommand->pNextCmd;
					pConsoleSet->pCommands = pCmd2;
					pCommand->pNextCmd = pCmd2->pNextCmd;
					pCmd2->pNextCmd = pCommand;
				} else {
					while(pCmd1 && pCmd1->pNextCmd) {
						if(pCmd1->pNextCmd == pCommand) {
							pCmd2 = pCommand->pNextCmd;
							pCmd1->pNextCmd = pCmd2;
							pCommand->pNextCmd = pCmd2->pNextCmd;
							pCmd2->pNextCmd = pCommand;
							break;
						}
						pCmd1 = pCmd1->pNextCmd;
					}
				}

			}
			if(iResult == -1) {	// Leave current string where it is.
				

			}
			pCommand = pCommand->pNextCmd;
		}
	} while(iSwaps);
	
	return FFT_ERR_NONE;
}

static void FFTerm_tolower(FF_T_INT8 *string) {
	while(*string) {
		if(*string >= 'A' && *string <= 'Z') {
			*string += ('a' - 'A');		// Difference between the capital and small case letters.
		}
		string++;
	}
}
