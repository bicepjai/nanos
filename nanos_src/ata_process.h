
#ifndef _ATA_PROCESS_H
#define _ATA_PROCESS_H

#include "ata.h"
#include "ff_ioman.h"
#include "ff_cmd.h"
#include "FFTerm.h"
#include "dosfs.h"

#define MSDOS_PARTITION 0
#define NANOS_PARTITION 1
#define NANOD_DEV_UNIT 0

char* currrent_directory;

int DFS_ReadSector (u8 *buffer, u32 sector, u32 count, u8 unit);
int DFS_ReadSector_hexdump (u8 unit, u8 *buffer, u32 sector, u32 count);

int DFS_WriteSector (u8 *buffer, u32 sector, u32 count, u8 unit);

FFT_CONSOLE_SET*  filesystem_consoleSet(FF_IOMAN *pIoman);
int  isFileSystemPresent ();
FF_IOMAN* fs_init( );
void fs_process();

#endif