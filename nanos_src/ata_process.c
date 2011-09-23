

#include "structs.h"
#include <spede/unistd.h>
#include <spede/sys/bloader.h>

#include "ata_process.h"

#define SW_INSPECT_HD	1
#define MODE_FORMAT_BOOT_BLK 1

BYTE   temp_buffer[2048];

FF_T_SINT32 FF_READ_BLOCKS_INTERFACE(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void* pParam){
	FF_T_SINT32 retVal;
	retVal = DFS_ReadSector (pBuffer, (u32)SectorAddress, (u32)Count, NANOD_DEV_UNIT);
	return retVal;
}

int DFS_ReadSector (u8 *buffer, u32 sector, u32 count, u8 unit)
{
	DriveCmd	cmd;
	int		result;
	u32		current_sector,i;
	//printf("\n DFS_ReadSector ..... sector %d count %d unit %d",sector,count,unit);    
	for (current_sector = 0; current_sector < count; current_sector++) {
	      //cmd.Blk = Drive[unit].Sects + (sector + current_sector);
	      cmd.Blk = sector + current_sector;
	      cmd.Count = ATA_SECTSIZE;
	      cmd.Cmd = DRV_CMD_RD;
	      cmd.Dev = unit;
	      cmd.Data = temp_buffer;
	      result = ataCmd(& cmd);
	      if( result == 0 ) {
			//printf("\n_____ Reading Drive[%d] sector#%ld \n", unit,sector + current_sector);
	      
			for (i = 0; i < ATA_SECTSIZE; i++) {
				//printf ("%x ",temp_buffer[i]);
				*buffer = temp_buffer[i];
				buffer++;
			}
	      } else {
		      printf("\n... Reading Drive[%d] sector#%ld failed \n",unit,sector + current_sector );
		      return 0;
	      }
	}
	return 1;
}

int DFS_ReadSector_hexdump (u8 unit, u8 *buffer, u32 sector, u32 count)
{
	DriveCmd	cmd;
	int		result;
	u32		current_sector,i;
	printf("\n DFS_ReadSector_hexdump .....sector %d",sector);    
	for (current_sector = 0; current_sector < count; current_sector++) {
	      //cmd.Blk = Drive[unit].Sects + (sector + current_sector);
	      cmd.Blk = sector + current_sector;
	      cmd.Count = ATA_SECTSIZE;
	      cmd.Cmd = DRV_CMD_RD;
	      cmd.Dev = unit;
	      cmd.Data = temp_buffer;
	      result = ataCmd(& cmd);
	      if( result == 0 ) {
			printf("\n_____ Reading Drive[%d] sector#%d _____\n", unit,sector + current_sector);
	      
			for (i = 0; i < ATA_SECTSIZE; i++) {
				//printf ("%d ",i);
				*buffer = temp_buffer[i];
				buffer++;
			}
	      
			hexdump(temp_buffer, ATA_SECTSIZE, 0, putstr);
	      } else {
		      printf("\n... Reading Drive[%d] sector#%d failed ...",unit,sector + current_sector );
		      return 0;
	      }
	}
	return 1;
}

FF_T_SINT32 FF_WRITE_BLOCKS_INTERFACE(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void* pParam){
	FF_T_SINT32 retVal;
	retVal = DFS_WriteSector (pBuffer, (u32)SectorAddress, (u32)Count, NANOD_DEV_UNIT);
	return retVal;
}

int DFS_WriteSector (u8 *buffer, u32 sector, u32 count, u8 unit)
{
	DriveCmd	cmd;
	int		result;
	u32		current_sector;
	//printf("\n DFS_WriteSector ...sector %ld",sector);    
	for (current_sector = 0; current_sector < count; current_sector++) {
	      //cmd.Blk = Drive[unit].Sects + (sector + current_sector);
	      cmd.Blk = sector + current_sector;
	      cmd.Count = ATA_SECTSIZE;
	      cmd.Cmd = DRV_CMD_WR;
	      cmd.Dev = unit;
	      cmd.Data = buffer;
	      result = ataCmd(& cmd);
	      if( result == 0 ) {
			//printf("\n_____ Writing Drive[%d] sector#%ld Success ", unit,sector + current_sector);
	      } else {
		      //printf("\n... Writing Drive[%d] sector#%ld failed ",unit,sector + current_sector );
		      return 0;
	      }
	}
	return 1;
}

int isFileSystemPresent () {
    BYTE buffer[512];
    u32 startLBA;
    PLBR nanosLBR;
    u8 pactive, pptype;
    u32 psize;
    
    startLBA = DFS_GetPtnStart(NANOD_DEV_UNIT, buffer, NANOS_PARTITION, &pactive, &pptype, &psize);
    
    nanosLBR = (PLBR)malloc(sizeof(LBR));
    DFS_ReadSector_hexdump(NANOD_DEV_UNIT,nanosLBR,startLBA,1);

    printf("\nnanosLBR->sig_55 %x nanosLBR->sig_aa %x",nanosLBR->sig_55,nanosLBR->sig_aa);
    if(nanosLBR->sig_55 == 0x55 && nanosLBR->sig_aa == 0xaa) {
      cons_printf("\nFile System Present!");
      return 1;
    } else {
      cons_printf("\nFile System Not Present!");
      return 0;
    }
}

FF_IOMAN* fs_init( )
{
	//File System mounting
	FF_PARTITION	*pPartition;		// Pointer to a partition description.
	FF_BLK_DEVICE	*pBlkDevice;
	FF_BUFFER	*pBuffers;		// Pointer to the first buffer description.
	
	FF_IOMAN	*pIoman;		// FullFAT I/O Manager Pointer, to be created.
	FFT_CONSOLE_SET	*pConsoleSet;		// FFTerm ConsoleSet
	FF_ENVIRONMENT	Env;			// Special Micro-Environment for the Demo
	
	FF_ERROR	Error = FF_ERR_NONE;	// ERROR code value.
	
	#if SW_INSPECT_HD
		/*  Inspect hard disk controller. */
		cons_printf("\nHardDrive Check ATA Check!");
		ataProbe();
		cons_printf("\nHardDrive Check PARTITION Check!");
		partProbe();
	#endif	
	
	//if (isFileSystemPresent() == 0) {
		cons_printf("\nInitializing File System!");
		initialize_NANOS_FS();
		cons_printf("\nFormating File System!");
		format_NANOS_FS (0);
	//}
	
	cons_printf("\nFilesystem Loading .... !");
	pIoman = FF_CreateIOMAN(NULL, 4096, 512, &Error);	// Using the default BlockSize from the Device Driver
	if(pIoman) {
		//---------- Register a Block Device with FullFAT.
		cons_printf("\nFilesystem Registering Block Device .... !");
		Error = FF_RegisterBlkDevice(pIoman, 512, (FF_WRITE_BLOCKS) FF_WRITE_BLOCKS_INTERFACE, (FF_READ_BLOCKS) FF_READ_BLOCKS_INTERFACE, 0);
		if(Error) {
			cons_printf("\nError Registering Device\nFF_RegisterBlkDevice() function returned with Error %ld.\nFullFAT says: %s\n", Error, FF_GetErrMessage(Error));
			return pIoman;
		}

		//---------- Try to Mount the Partition with FullFAT.
		cons_printf("\nFilesystem Mounting .... !");
		Error = FF_MountPartition(pIoman, NANOS_PARTITION);
		if(Error) {
			FF_DestroyIOMAN(pIoman);
			cons_printf("\nFullFAT Couldn't mount the specified parition!");
			cons_printf("\nFF_MountPartition() function returned with Error %ld\nFullFAT says: %s\n", Error, FF_GetErrMessage(Error));
			//getchar();
			return pIoman;
		}
	}
	/*printf("\nfs_init: NumFATS->%d",		pIoman->pPartition->NumFATS);
	printf("\nfs_init: SectorsPerFAT->%d",		pIoman->pPartition->SectorsPerFAT);
	printf("\nfs_init: SectorsPerCluster->%d",	pIoman->pPartition->SectorsPerCluster);
	printf("\nfs_init: pIoman->pPartition->RootDirCluster: %d",pIoman->pPartition->RootDirCluster);*/
	
	return pIoman;
}

FFT_CONSOLE_SET	* filesystem_consoleSet(FF_IOMAN *pIoman) {
	
	FFT_CONSOLE_SET* pConsoleSet;
	FF_ERROR	Error = FF_ERR_NONE;	// ERROR code value.
	FF_ENVIRONMENT	Env;			// Special Micro-Environment for the Demo
	
	//----------- Initialise the environment
	Env.pIoman = pIoman;		// Initialise the FullFAT I/O Manager to NULL.
	strcpy(Env.WorkingDir, "\\");	// Reset the Working Directory to the root folder.
	currrent_directory = Env.WorkingDir;
	// Create a console with a "FullFAT> prompt.
	pConsoleSet = FFTerm_CreateConsoleSet(&Error);
	if(pConsoleSet) {
		//---------- Add Commands to the console.
		FFTerm_AddExCmd	(pConsoleSet,	"pwd",	(FFT_FN_COMMAND_EX) pwd_cmd,		pwdInfo,	&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"ls",	(FFT_FN_COMMAND_EX) ls_cmd,		lsInfo,		&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"mkdir",(FFT_FN_COMMAND_EX) mkdir_cmd,		mkdirInfo,	&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"mkfile",(FFT_FN_COMMAND_EX) mkfile_cmd,	mkfileInfo,	&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"rm",(FFT_FN_COMMAND_EX) rm_cmd,		rmInfo,		&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"move",(FFT_FN_COMMAND_EX) move_cmd,		moveInfo,	&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"cp",(FFT_FN_COMMAND_EX) cp_cmd,		cpInfo,		&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"cd",(FFT_FN_COMMAND_EX) cd_cmd,		cdInfo,		&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"view",(FFT_FN_COMMAND_EX) view_cmd,		viewInfo,	&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"info",(FFT_FN_COMMAND_EX) info_cmd,		infoInfo,	&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"time",(FFT_FN_COMMAND_EX) time_cmd,		timeInfo,	&Env);
		FFTerm_AddExCmd	(pConsoleSet,	"date",(FFT_FN_COMMAND_EX) date_cmd,		dateInfo,	&Env);
	}
	//FFTerm_ExecCommand(pConsoleSet,"ls\0");
	//FFTerm_ExecCommand(pConsoleSet,"mkfile 1024 1 1 first\0");
	//FFTerm_ExecCommand(pConsoleSet,"pwd\0");
	return pConsoleSet;
}

//void directory_init() {

	/*argv = (char **)malloc(5*sizeof(char*));

	argv[0] = (char *)malloc(50*sizeof(char));
	strcpy(argv[0],"command");

	argv[1] = (char *)malloc(50*sizeof(char));
	strcpy(argv[1],"test");
	mkdir_cmd(2,argv,&Env);
	
	pwd_cmd(1,argv,&Env);
	ls_cmd(1,lsInfo, &Env);	// Directory Listing Command.
	
	ls_cmd(1,lsInfo, &Env);	// Directory Listing Command.	
	argv[1] = (char *)malloc(50*sizeof(char));
	strcpy(argv[1],"1024");
	argv[2] = (char *)malloc(50*sizeof(char));
	strcpy(argv[2],"1");
	argv[3] = (char *)malloc(50*sizeof(char));
	strcpy(argv[3],"1");
	argv[4] = (char *)malloc(50*sizeof(char));
	strcpy(argv[4],"file1");
	mkfile_cmd(5,argv,&Env);
	
	ls_cmd(1,lsInfo, &Env);	// Directory Listing Command.
	
	argv[1] = (char *)malloc(50*sizeof(char));
	strcpy(argv[1],"first_dir1");
	cd_cmd(2,argv,&Env);
	//ls_cmd(1,lsInfo, &Env);	// Directory Listing Command.
	
	argv[1] = (char *)malloc(50*sizeof(char));
	strcpy(argv[1],"first_dir");
	cd_cmd(2,argv,&Env);
	pwd_cmd(1,argv,&Env);
	ls_cmd(1,lsInfo, &Env);	// Directory Listing Command.
	
	argv[1] = (char *)malloc(50*sizeof(char));
	strcpy(argv[1],"..");
	cd_cmd(2,argv,&Env);
	pwd_cmd(1,argv,&Env);
	ls_cmd(1,lsInfo, &Env);	// Directory Listing Command.
	
	argv[1] = (char *)malloc(50*sizeof(char));
	strcpy(argv[1],"first_dir");

	rm_cmd(2,argv,&Env);
	pwd_cmd(1,argv,&Env);
	ls_cmd(1,lsInfo, &Env);	// Directory Listing Command.
	
	mkdir_cmd(2,argv,&Env);  */
//}

void format_NANOS_FS (int mode) {
    BYTE	buffer[512];
    int i;
    u32 startLBA;
    u8 scratchsector[512];
    u8 pnum ,pactive, pptype;
    u32 psize;
    
    // reading volume info
    PVOLINFO volumeInfo ;
    cons_printf ("\n Formating NANOS_FS ... \n");    

   //format the partition for nanos by writing zeros to fat sectors
    volumeInfo = (PVOLINFO)malloc(sizeof(VOLINFO));
    
    startLBA = DFS_GetPtnStart(NANOD_DEV_UNIT, scratchsector, NANOS_PARTITION, &pactive, &pptype, &psize);
    DFS_GetVolInfo(NANOD_DEV_UNIT, buffer, startLBA, volumeInfo);

    
    DFS_GetVolInfo(NANOD_DEV_UNIT, buffer, startLBA, volumeInfo);
    
   cons_printf ("\nFormating Fat1 %d for 3 sectors==",volumeInfo->fat1);
   for(i=0;i<512;i++)   buffer[i]=0;
   //formating fat sectors
   //formating only 3 sectors of fat
   // but need to write over 512 bytes to wipe it fully
   buffer[0]=0xF8;
   buffer[1]=0xFF;
   DFS_WriteSector(buffer,volumeInfo->fat1,3,NANOD_DEV_UNIT);

   cons_printf ("\n Formating root directory sector %d for 512 sectors==",volumeInfo->rootdir);
   for(i=0;i<512;i++)   buffer[i]=0;
   //formating root directory
   //formating 1 sec for fast
   DFS_WriteSector(buffer,volumeInfo->rootdir,1,NANOD_DEV_UNIT);
   DFS_WriteSector(buffer,(volumeInfo->rootdir-256),5,NANOD_DEV_UNIT);   
  
    //formating 5 data area sec for fast
   cons_printf ("\n Formating data area sector %d for 512 sectors==",volumeInfo->dataarea);
   DFS_WriteSector(buffer,volumeInfo->dataarea,5,NANOD_DEV_UNIT);
   
   if (mode == MODE_FORMAT_BOOT_BLK) {
     cons_printf ("\n Formating boot block sector %d for 512 sectors==",startLBA);
     DFS_WriteSector(buffer,startLBA,1,NANOD_DEV_UNIT);
   }
   
   printf ("\n Formating_NANOS_FS done ... \n");    
   
}

void initialize_NANOS_FS () {
    BYTE	buffer[512];
    
    // for setting Boot Record Information for the partition about FAT FS
    u32 startLBA;
    u8 unit;
    u8 scratchsector[512];
    u8 pnum ,pactive, pptype;
    u32 psize;
    
    PLBR msdosLBR, nanosLBR;
    
    cons_printf ("\nInitialize_NANOS_FS .... \n");
    startLBA = DFS_GetPtnStart(NANOD_DEV_UNIT, scratchsector, NANOS_PARTITION, &pactive, &pptype, &psize);
    cons_printf ("\n startLBA .... %d (%d)",startLBA,sizeof(LBR));
    nanosLBR = (PLBR)malloc(sizeof(PLBR));
    
    // here consider 512 512
    nanosLBR->oemid[0]='F'; nanosLBR->oemid[1]='A'; nanosLBR->oemid[2]='T'; nanosLBR->oemid[3]='1'; nanosLBR->oemid[4]='6';
    nanosLBR->oemid[5]=' '; nanosLBR->oemid[6]=' '; nanosLBR->oemid[7]=' ';
    nanosLBR->bpb.bytepersec_l	= 0x0;
    nanosLBR->bpb.bytepersec_h	= 0x2;
    
    nanosLBR->bpb.secperclus	= 0x40;
    
    nanosLBR->bpb.reserved_l	= 0x1;
    nanosLBR->bpb.reserved_h	= 0x0;
    
    //only one FAT Table
    nanosLBR->bpb.numfats	= 0x1;
    
    // 512 items can be there inside Root Directory
    nanosLBR->bpb.rootentries_l	= 0x0;
    nanosLBR->bpb.rootentries_h	= 0x2;
    
    nanosLBR->bpb.sectors_s_l	= 0x0;
    nanosLBR->bpb.sectors_s_h	= 0x0;
    
    // identofied as hard disk by msdos
    nanosLBR->bpb.mediatype	= 0xf8;
    
    // 512 sectors occupoed by 1 FAT Table (since we x by 2)
    nanosLBR->bpb.secperfat_l	= 0x0;
    nanosLBR->bpb.secperfat_h	= 0x2;
    
    //sectors per track 63 (taken from identify command)
    nanosLBR->bpb.secpertrk_l	= 0x3f;
    nanosLBR->bpb.secpertrk_h	= 0x0;
    
    //heads 255 ??
    nanosLBR->bpb.heads_l	= 0xff;
    nanosLBR->bpb.heads_h	= 0x0;
    
    nanosLBR->bpb.hidden_0	= 0x3f;
    nanosLBR->bpb.hidden_1	= 0x0;
    nanosLBR->bpb.hidden_2	= 0x0;
    nanosLBR->bpb.hidden_3	= 0x0;
    
    //total sectors occupied by file system - partition size
    nanosLBR->bpb.sectors_l_0	= psize;
    nanosLBR->bpb.sectors_l_1	= psize >> 8;
    nanosLBR->bpb.sectors_l_2	= psize >> 16;
    nanosLBR->bpb.sectors_l_3	= psize >> 24;
    
    //printf ("\nNANOS_PARTITION size %d %x; 0=%x 1=%x 2=%x 3=%x\n",psize,psize,nanosLBR->bpb.sectors_l_0,nanosLBR->bpb.sectors_l_1,nanosLBR->bpb.sectors_l_2,nanosLBR->bpb.sectors_l_3);
    
    nanosLBR->ebpb.ebpb.unit	= 0x80;
    nanosLBR->ebpb.ebpb.head	= 0x0;
    nanosLBR->ebpb.ebpb.signature	= 0x29;
    nanosLBR->ebpb.ebpb.serial_0	= 0xea;
    nanosLBR->ebpb.ebpb.serial_1	= 0x8;
    nanosLBR->ebpb.ebpb.serial_2	= 0x66;
    nanosLBR->ebpb.ebpb.serial_3	= 0x25;

    // boot signatire marking end of boot sector
    nanosLBR->sig_55	= 0x55;
    nanosLBR->sig_aa	= 0xaa;
    
    // write boot sector to NANOS_PARTITION 1st block
    cons_printf("\n Boot Block Setting ...");
    DFS_WriteSector(nanosLBR,startLBA,1,NANOD_DEV_UNIT);
    //DFS_ReadSector_hexdump(NANOD_DEV_UNIT,buffer,startLBA,1);
    cons_printf ("\n Initializing NANOS_FS done ... \n");    
}
