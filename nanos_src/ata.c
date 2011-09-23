/*   ata.c
 *	@(#)ata.c	1.6   06/21/00
 */
								/* ata.c
******************************************************************************
	ata.c - ATA ("IDE") hard drive and ATAPI CD-ROM demo code
	Christopher Giese <geezer[AT]execpc.com>
	htpp://www.execpc.com/~geezer

	Release date 12/6/98. Distribute freely. ABSOLUTELY NO WARRANTY.

	Compile with Turbo C or DJGPP.
	*** USE -fpack-struct OPTION with DJGPP! ***

Sources:
	Linux ide.c, ide.h, hdreg.h
	(C) 1994-1996 Linus Torvalds, et al.
	Mark Lord <mlord@pobox.com>, Gadi Oxman <gadio@netvision.net.il>

	Linux ide-cd.c
	(C) 1994-1996 scott snyder <snyder@fnald0.fnal.gov>

	Linux cdrom.h
	(C) 1992 David Giller <rafetmad@oxy.edu>
		1994, 1995 Eberhard Moenkeberg <emoenke@gwdg.de>

	ide_spec.txt - Integrated Drive Electronics (IDE)
	Harwdare Reference & Information Document. Ver 1.00
	Compiled Jan 19th, 1994, by <cyborg@kauri.vuw.ac.nz>

	atadrvr.zip - ATA and ATAPI demo code
	No copyright - Hale Landis <landis@sugs.tware.com>

	sff8020.ps - ATAPI spec

	Table 0575 (format of partition record) from Ralf Brown's
	interrupt list (INTERRUP.D; listed under INT 19).

Fixes/changes:
	- CD-ROM now works properly after ATAPI 'identify'.

Bugs/to do:
	- ATAPI code does not work reliably if byte count is not a
	  multiple of 2048.

	- Write a new ataXfer() that
	  * allows reads to start in the middle of an ATA or ATAPI sector
		(no odd writes until a buffer cache system is added)
		(change Blk field of DriveCmd [sectors] to Offset [bytes])
	  * allows odd-length Count in DriveCmd

	- Buffer cache

	- Queued drive operations

	- TEST drives on 2nd interface (0x170, IRQ15).

	- Still some arbitrary timeout values.

	- atapiCmd2() reads only 2048 bytes at a time. Is this normal?

	- False detection of slave ATA drive with Seagate ST3144A master
	  (it's jumpered properly, and Linux and the BIOS work OK).

	- Lots of blocking delays. This code is only partially interrupt-
	  driven, and not yet suitable for a high-performance OS.

	- Code needs more error checking and error-recovery.
*****************************************************************************/

#define  SW_READONLY	0

/*  Extra //DEBUGging output! */
#define  SW_WATCH	1

#include	"ata.h"

#define  IOBASE_HDC1	0x1F0
#define  IOBASE_HDC2	0x170

/* ATA drive command bytes */
#define	ATA_CMD_RD	0x20		/* read one sector */
#define	ATA_CMD_WR	0x30		/* write one sector */
#define	ATA_CMD_PKT	0xA0		/* ATAPI packet cmd */
#define	ATA_CMD_PID	0xA1		/* ATAPI identify */
#define	ATA_CMD_RDMUL	0xC4		/* read multiple sectors */
#define	ATA_CMD_WRMUL	0xC5		/* write multiple sectors */
#define	ATA_CMD_ID	0xEC		/* ATA identify */

/* ATA/ATAPI drive register file */
#define	ATA_REG_DATA	0		/* data (16-bit) */
#define	ATA_REG_FEAT	1		/* write: feature reg */
#define	ATA_REG_ERR	ATA_REG_FEAT	/* read: error */
#define	ATA_REG_CNT	2		/* ATA: sector count */
#define	ATA_REG_REASON	ATA_REG_CNT	/* ATAPI: interrupt reason */
#define	ATA_REG_SECT	3		/* sector */
#define	ATA_REG_LOCYL	4		/* ATA: LSB of cylinder */
#define	ATA_REG_LOCNT	ATA_REG_LOCYL	/* ATAPI: LSB of transfer count */
#define	ATA_REG_HICYL	5		/* ATA: MSB of cylinder */
#define	ATA_REG_HICNT	ATA_REG_HICYL	/* ATAPI: MSB of transfer count */
#define	ATA_REG_DRVHD	6		/* drive select; head */
#define	ATA_REG_CMD	7		/* write: drive command */
#define	ATA_REG_STAT	7		/* read: status and error flags */
#define	ATA_REG_SLCT	0x206		/* write: device control */
#define	ATA_REG_ALTST	0x206		/* read: alternate status/error */


int	hd_intr_occurd;


/*****************************************************************************
	name:	nsleep
	action:	precision delay for Count nanoseconds
		(not yet implemented)
*****************************************************************************/
#define  nsleep(nanosecs)	microsec_wait(1 + (nanosecs) / 1000)


/*****************************************************************************
	name:	insw
	action:	reads Count words (16-bit) from I/O port Adr to
		memory location Data
*****************************************************************************/
void insw(unsigned Adr, u16 *Data, unsigned Count)
{
	for(; Count; Count--)
		*Data++=inw(Adr);
}

/*****************************************************************************
	name:	outsw
	action:	writes Count words (16-bit) from memory location Data
		to I/O port Adr
*****************************************************************************/
void outsw(unsigned Adr, u16 *Data, unsigned Count)
{
	for(; Count; Count--)
		outw(Adr, *Data++);
}


/*****************************************************************************
	name:	awaitInterrupt
	action:	waits with Timeout (mS) until interrupt(s) given by bitmask
		IRQMask occur. Won't work if operation on other controller
		is in progress while waiting on other HDC.
	returns:nonzero Mask value if interrupt occured, zero if timeout
*****************************************************************************/
int awaitInterrupt(u16 IRQMask, unsigned MicrosecTimeout)
{
	u16 Intr =0;
	//printf(" =>eflags=$%x =>%x \n (%04x,%u) %04x <=", get_eflags(),(0x0200 & get_eflags()),IRQMask, MicrosecTimeout, hd_intr_occurd);
	assert( 0 != (0x0200 & get_eflags()) );
	//DEBUG(("awaitInterrupt(%04x,%u) %04x <", IRQMask, MicrosecTimeout, hd_intr_occurd));
#if SW_INTRS_HD
	MicrosecTimeout /= 10;  	/* Each loop waits 10 milliseconds */
	for(; MicrosecTimeout; MicrosecTimeout--) {
		Intr = hd_intr_occurd & IRQMask;
		if(Intr) {
		    break;
		}
		/* Don't make pause(); maybe intr already occured! */
		msleep(10);
	}
	//DEBUG((">"));
# if SW_WATCH
	{
	    unsigned irr;

	    outportb(0x0A0, 0x0A);	/* Ask OCW3 for IIR */
	    irr = inportb(0x0A0);
	    //printf(" slave PIC: IIR = %08x, mask = %08x, eflags=$%x\n", irr, inportb(0x0A1), get_eflags());
	}
# endif	/* if SW_WATCH */
	if(MicrosecTimeout == 0) return(0);
	hd_intr_occurd &= ~Intr;	/* XXX - THREAD-UNSAFE UPDATE! */
	return(Intr);
#else
	for( ; Intr == 0 && MicrosecTimeout; MicrosecTimeout--) {
	    unsigned   bits;

	    /*  Check primary HDC */
	    if( IRQMask & BIT(14) ) {
		bits = inportb(IOBASE_HDC1 + ATA_REG_STAT);
		if( !(bits & WDS_BUSY) &&
		    (bits & WDS_DRQ) ) {
		    Intr = BIT(14);
		    //printf("HDC1 stat = %04x ", bits);
		}
	    }
	    /*  Check secondary HDC */
	    if( IRQMask & BIT(15) ) {
		bits = inportb(IOBASE_HDC2 + ATA_REG_STAT);
		if( !(bits & WDS_BUSY) &&
		    (bits & WDS_DRQ) ) {
		    Intr = BIT(15);
		    //printf("HDC2 stat = %04x ", bits);
		}
	    }
	    msleep(1);
	}
	//printf("> TO=%d, Intr=$%x\n", MicrosecTimeout, Intr);
	if(MicrosecTimeout == 0) return(0);
	return( Intr );
#endif	/* else don't use interrupts */

}   /* end awaitInterrupt() */

/*----------------------------------------------------------------------------
				ATA/ATAPI STUFF
----------------------------------------------------------------------------*/

/*  Controller 'identify' structure, as per ANSI ATA2 rev.2f spec.
 *  Read out by "Identify Drive" command, 0xEC.
 */
typedef struct
{	u16	Config;				// 0 see spec
	u16	PhysCyls;			// 1 Nof Logical Cylinders
	u16	Res2;				// 2
	u16	PhysHeads;			// 3 Nof Logical Heads
	u32	Vendor4_5;			// 4 to 5
	u16	PhysSects;			// 6 Nof Logical Sectors per Logical track
	u16	Vendor7_9[3];			// 7 to 9
	u8 	SerialNum[10*2];		// 10 to 19  ASCII serial number
        u32	Obs20_21;			// 20 to 21
	u16	NofVendorBytesRWLongCmd;	// 22 Nof Vendor Specifc bytes available on Read/Write Long Cmds
	u8 	FirmwareRev[4*2];		// 23 to 26 Firmware Revision ASCII
	u8	Model[20*2];			// 27 to 46 Model No ASCII
	u16	RWMultSupp;			// 47 RWMultipleSupport;
	u16	Res48;				// 48
	u16	Capability;			// 49 see spec
	u16	Res50;				// 50
	u16	PIODataTxCycTimeMode;		// 51 PIO Data Transfer Cycle Timing mode
	u16	Obs52;				// 52
	u16	FieldValidity;			// 53 Field Value
	u16	LogCyls;			// 54 Number of current logical cylinders
	u16	LogHeads;			// 55 Number of current logical heads
	u16	LogSectsPerTrack;		// 56 Number of current logical sectors per logical track
	u32	LBASects;			// 57 to 58 Current capacity in sectors
	u16	MultSect;			// 59 Multiple sector setting
	u32	UsrAddrSects;			// 60 to 61 Total number of user addressable sectors
	u16	Obs62;				// 62
	u16	DMAInfoMult;			// 63 Multiword DMA transfer
	u16	FCPIOMode;			// 64 Flow control PIO transfer modes supported
	u16	MinMultiWrdDMATxCycTime;	// 65 Minimum multiword DMA transfer cycle time per word
	u16	DevMultiWrdDMACycTime;		// 66 Device recommended multiword DMA cycle time
	u16	MinPIOTxCycTimeNoFC;		// 67 Minimum PIO transfer cycle time without flow control
	u16	MinPIOTxCycTimeIORDY;		// 68 Minimum PIO transfer cycle time with IORDY
	u16	Res69_79[10];			// 69 to 79
	u16	MajVerNo;			// 80 Major version number
	u16	MinorVerNo;			// 81 Minor version number
	u32	CmdSetSup;			// 82 to 83 Command sets supported see spec
	u16	Res84_127[43];			// 84 to 127
	u16	SecurityStat;			// 128
	u16	Vendor129_159[30];		// 129 to 159
	u16	Res160_255[49];			// 160 to 255
} ataid;

DriveSpec	Drive[4];
PartitionInfo	Partitions[4];

/*****************************************************************************
	name:	ataProbe2
*****************************************************************************/
void ataProbe2(unsigned WhichDrive)
{
	unsigned Temp1, Temp2;
	unsigned IOAdr, Temp;
	u16	buff[ATA_SECTSIZE/sizeof(u16)];
	ataid DriveInfo;

	assert( WhichDrive < sizeof(Drive)/sizeof(Drive[0]) );
	IOAdr = Drive[WhichDrive].IOAdr;
	Temp1 = inb(IOAdr + ATA_REG_CNT);
	Temp2 = inb(IOAdr + ATA_REG_SECT);
	if(Temp1 != 0x01 || Temp2 != 0x01)
	{
		//printf("HDC%d: nothing there\n", WhichDrive);
 NO_DRIVE:	Drive[WhichDrive].IOAdr=0;
		return;
	}

	Temp1 = inb(IOAdr + ATA_REG_LOCYL);
	Temp2 = inb(IOAdr + ATA_REG_HICYL);
	Temp  = inb(IOAdr + ATA_REG_STAT);
	if(Temp1 == 0xFF && Temp2 == 0x0FF && Temp == 0xFF)
	{
		//printf("Controller not present\n");
		goto NO_DRIVE;
	} else
	if(Temp1 == 0x14 && Temp2 == 0xEB)
	{	//printf("ATAPI CD-ROM, ");
		Drive[WhichDrive].Flags |= ATA_FLG_ATAPI;
/* issue ATAPI 'identify drive' command */
		Temp1 = ATA_CMD_PID;
		outb(IOAdr + ATA_REG_CMD, Temp1);
		Temp = WAIT_PID;
	}
	else if(Temp1 == 0 && Temp2 == 0 && Temp) {
		//printf("ATA hard drive, ");
/* issue ATA 'identify drive' command */
		Temp1 = ATA_CMD_ID;
		outb(IOAdr + ATA_REG_CMD, Temp1);
		Temp = WAIT_ID; 
	}
	else
	{	//printf("unknown drive type\n");
		goto NO_DRIVE;
	}

/* ATA or ATAPI: get results of of identify */
	msleep(2);
	if(awaitInterrupt(0xC000, Temp) == 0)
/* XXX - could be old drive that doesn't support 'identify'.
 *	Read geometry from partition table? Use (* gag *) CMOS?
 */
	{
		(void) inb(IOAdr + ATA_REG_STAT);	/* Clear interrupt. */
		//printf("'identify' failed\n");
		goto NO_DRIVE;
	}

/* grab info returned by controller 'identify' cmd. Clear intr, too. */
	(void)inb(IOAdr + ATA_REG_STAT);
/* for ATAPI CD-ROM, you MUST read 512 bytes here, or
 * drive will go comatose.
 */
	insw(IOAdr + ATA_REG_DATA, buff, 512/2);
	memcpy( & DriveInfo, buff, sizeof(DriveInfo) );

	Temp2=1;
	if(Temp1 == ATA_CMD_PID)
/* model name is not byte swapped for NEC, Mitsumi, and Pioneer drives */
	{	if((DriveInfo.Model[0] == 'N' && DriveInfo.Model[1] == 'E') ||
			(DriveInfo.Model[0] == 'F' && DriveInfo.Model[1] == 'X') ||
			(DriveInfo.Model[0] == 'P' && DriveInfo.Model[1] == 'i'))
				Temp2=0;
	}

	//printf("\"");
	for(Temp=0; Temp < 40; Temp += 2)
	{	//printf("%c", DriveInfo.Model[Temp ^ Temp2]);
		//printf("%c", DriveInfo.Model[Temp ^ Temp2 ^ 1]);
	}
	//printf("\"\n"" CHS=%u:%u:%u, ", DriveInfo.PhysCyls, DriveInfo.PhysHeads,DriveInfo.PhysSects);

	Drive[WhichDrive].Sects=DriveInfo.PhysSects;
	Drive[WhichDrive].Heads=DriveInfo.PhysHeads;
	Drive[WhichDrive].Cyls=DriveInfo.PhysCyls;
	if(DriveInfo.Capability & BIT(8))
	{	//printf("DMA, ");
		Drive[WhichDrive].Flags |= ATA_FLG_DMA;
	}
	if(DriveInfo.Capability & BIT(9) )
	{	//printf("LBA, ");
		Drive[WhichDrive].Flags |= ATA_FLG_LBA;
	}
	if( DriveInfo.Config & BIT(7) ) {
	    //printf("Removable, ");
	}
	if( DriveInfo.Config & BIT(6) ) {
	    //printf("Hard disk, ");
	}
#if 0
/* By Dobbs, I'll figure this out yet. Linux ide.c requires
 *	(DriveInfo.MultSectValid & 1) && DriveInfo.MultSect
 * The magic value then comes from DriveInfo.MaxMult
 *
 *  QUANTUM FIREBALL ST2.1A
 *	MaxMult=16	MultSect=16	MultSectValid=1
 *  Conner Peripherals 850MB - CFS850A
 *	MaxMult=16	MultSect=0	MultSectValid=1
 *  (Seagate) st3144AT
 *	MaxMult=0	MultSect=0	MultSectValid=0 
 */
	if((DriveInfo.MultSectValid & 1) && DriveInfo.MultSect)
	{	Temp=DriveInfo.MaxMult;
		//printf("MaxMult=%u, ", Temp);
	} else {
		Temp=1;
	}
	Drive[WhichDrive].MultSect=Temp;
#else
	Drive[WhichDrive].MultSect = 1;
#endif
#if SW_WATCH
	//printf("\n------IDENTIFY OUTPUT------");
	/*//printf("\n0 Configuration  %u",DriveInfo.Config);
	//printf("\n1 Nof Logical Cylinders %u", DriveInfo.PhysCyls);
	//printf("\n3 Nof Logical Heads %u", DriveInfo.PhysHeads);
	//printf("\n6 Nof Logical Sectors per Logical track %u", DriveInfo.PhysSects);
	//printf("\n10 to 19  ASCII serial number ");
	for(Temp=0; Temp < 20; Temp += 1)
	      //printf("%c", DriveInfo.SerialNum[Temp]);
	//printf("\n22 Nof Vendor Specifc bytes available on Read/Write Long Cmds %u", DriveInfo.NofVendorBytesRWLongCmd);
	//printf("\n23 to 26 Firmware Revision ASCII ");
	for(Temp=0; Temp < 8; Temp += 1)
	      //printf("%c", DriveInfo.FirmwareRev[Temp]);
	      
	//printf("\n27 to 46 Model No ASCII %u", DriveInfo.Model[20*2]);
	for(Temp=0; Temp < 40; Temp += 1)
	      //printf("%c", DriveInfo.Model[Temp]);	
	//printf("\n47 RWMultipleSupport 0x%x", DriveInfo.RWMultSupp);
	//printf("\n49 see spec 0x%x", DriveInfo.Capability);
	//printf("\n51 PIO Data Transfer Cycle Timing mode 0x%x", DriveInfo.PIODataTxCycTimeMode);
	//printf("\n53 Field Value 0x%x", DriveInfo.FieldValidity);
	//printf("\n54 Number of current logical cylinders %u", DriveInfo.LogCyls);
	//printf("\n55 Number of current logical heads %u", DriveInfo.LogHeads);
	//printf("\n56 Number of current logical sectors per logical track %u", DriveInfo.LogSectsPerTrack);
	//printf("\n57 to 58 Current capacity in sectors %lu", DriveInfo.LBASects);
	//printf("\n59 Multiple sector setting 0x%x", DriveInfo.MultSect);
	//printf("\n60 to 61 Total number of user addressable sectors %lu", DriveInfo.UsrAddrSects);
	//printf("\n63 Multiword DMA transfer 0x%x", DriveInfo.DMAInfoMult);
	//printf("\n64 Flow control PIO transfer modes supported 0x%x", DriveInfo.FCPIOMode);
	//printf("\n65 Minimum multiword DMA transfer cycle time per word %u", DriveInfo.MinMultiWrdDMATxCycTime);
	//printf("\n66 Device recommended multiword DMA cycle time %u", DriveInfo.DevMultiWrdDMACycTime);
	//printf("\n67 Minimum PIO transfer cycle time without flow control %u", DriveInfo.MinPIOTxCycTimeNoFC);
	//printf("\n68 Minimum PIO transfer cycle time with IORDY %u", DriveInfo.MinPIOTxCycTimeIORDY);
	//printf("\n80 Major version number 0x%x", DriveInfo.MajVerNo);
	//printf("\n81 Minor version number 0x%x", DriveInfo.MinorVerNo);
	//printf("\n82 to 83 Command sets supported see spec 0x%x", DriveInfo.CmdSetSup);
	//printf("\n128 Security Status 0x%x", DriveInfo.SecurityStat);*/
#endif
}   /* end ataProbe2() */

/*****************************************************************************
	name:	ataSelect
*****************************************************************************/
int ataSelect(unsigned IOAdr, unsigned Sel)
{
	unsigned Temp;

	Temp=inb(IOAdr + ATA_REG_DRVHD);
	if(((Temp ^ Sel) & 0x10) == 0) return(0); /* already selected */
	outb(IOAdr + ATA_REG_DRVHD, Sel);
	nsleep(400);
	for(Temp=WAIT_READY; Temp; Temp--) {
		if((inb(IOAdr + ATA_REG_STAT) & WDS_BUSY) == 0) break;
		msleep(1);
	}	/* this _must_ be polled, I guess (sigh) */
	//printf("ataSelect: selected\n");
	return(Temp == 0); 
}

/*****************************************************************************
	name:	ataProbe
*****************************************************************************/
void ataProbe(void)
{
	unsigned Temp1, Temp2, WhichDrive;
	unsigned IOAdr, Temp;

#if SW_INTRS_HD
	cons_printf("\nHARD DISK -- INTERRUPTS");
#else
	cons_printf("\nHARD DISK -- PIO");
#endif
	cons_printf("\nataProbe:");
/* set initial values */
	Drive[0].DrvSel=Drive[2].DrvSel=0xA0;
	Drive[1].DrvSel=Drive[3].DrvSel=0xB0;
	Drive[0].IOAdr=Drive[1].IOAdr = IOBASE_HDC1;
	Drive[2].IOAdr=Drive[3].IOAdr = IOBASE_HDC2;

	for(WhichDrive=0; WhichDrive < 4; WhichDrive += 2)
	{	IOAdr=Drive[WhichDrive].IOAdr;
/* poke at the interface to see if anything's there */
		cons_printf("\nataProbe: poking interface 0x%03X", IOAdr);
		outb(IOAdr + ATA_REG_CNT, 0x55);
		outb(IOAdr + ATA_REG_SECT, 0xAA);
		Temp1=inb(IOAdr + ATA_REG_CNT);
		Temp2=inb(IOAdr + ATA_REG_SECT);
		if(Temp1 != 0x55 || Temp2 != 0xAA) {
/* no master: clobber both master and slave */
 NO_DRIVES:		Drive[WhichDrive + 1].IOAdr=
				Drive[WhichDrive].IOAdr=0;
			continue;
		}
/* soft reset both drives on this I/F (selects master) */
		/*  VSTa has 0.1 sec sleep before/after reset. */
		cons_printf("\nataProbe: found something at 0x%03X, doing soft reset...", IOAdr);
		outb(IOAdr + ATA_REG_SLCT, CTLR_4BIT|CTLR_RESET|CTLR_IDIS);
		msleep(100);
/* release soft reset AND enable interrupts from drive */
		outb(IOAdr + ATA_REG_SLCT, CTLR_4BIT);
		msleep(100);
/* wait for master */
		for(Temp=WAIT_SELECT; Temp; Temp--)
		{	if((inb(IOAdr + ATA_REG_STAT) & WDS_BUSY) == 0) break;
			msleep(1);
		}	/* XXX - blocking delay */
		if(Temp == 0)
		{	
			cons_printf("\nataProbe: no master at 0x%03X", IOAdr);
			goto NO_DRIVES;
		}
/* identify master */
		cons_printf("\nhd%1u (0x%03X, master): ", WhichDrive, IOAdr);
		ataProbe2(WhichDrive);
/* select slave */
		if(ataSelect(IOAdr, 0xB0))
/* no response from slave: re-select master and continue */
		{	ataSelect(IOAdr, 0xA0);
			Drive[WhichDrive + 1].IOAdr=0;
			cons_printf("\nataProbe: no slave at 0x%03X", IOAdr);
			continue; 
		}
/* identify slave */
		cons_printf("  hd%1u (0x%03X,  slave): ", WhichDrive + 1, IOAdr);
		ataProbe2(WhichDrive + 1); 
	}
}

/*****************************************************************************
	name:	ataCmd2
*****************************************************************************/
void ataCmd2(DriveCmd *Cmd, u8 Count, u8 CmdByte)
{	u8 Sect, DrvHd;
	u16 Cyl, IOAdr;
	u32 Temp;

	IOAdr=Drive[Cmd->Dev].IOAdr;
/* compute CHS or LBA register values */
	Temp=Cmd->Blk;
	if(Drive[Cmd->Dev].Flags & ATA_FLG_LBA)
	{	Sect=Temp;		/* 28-bit sector adr: low byte */
		Cyl=Temp >> 8;		/* middle bytes */
		DrvHd=Temp >> 24;	/* high nybble */
		DrvHd=(DrvHd & 0x0F) | 0x40;/* b6 enables LBA */
		//DEBUG(("ataCmd2: LBA=%lu\n", Temp));
	}
	else
	{	Sect=Temp % Drive[Cmd->Dev].Sects + 1;
		Temp /= Drive[Cmd->Dev].Sects;
		DrvHd=Temp % Drive[Cmd->Dev].Heads;
		Cyl=Temp / Drive[Cmd->Dev].Heads;
		//DEBUG(("ataCmd2: CHS=%u:%u:%u\n", Cyl, DrvHd, Sect));
	}
	DrvHd |= Drive[Cmd->Dev].DrvSel;
	//DEBUG(("ataCmd2: writing register file\n"));
	outb(IOAdr + ATA_REG_CNT, Count);
	outb(IOAdr + ATA_REG_SECT, Sect);
	outb(IOAdr + ATA_REG_LOCYL, Cyl);	/* low-byte */
	Cyl >>= 8; /* compiler bug work-around */
	outb(IOAdr + ATA_REG_HICYL, Cyl);	/* high byte */
	outb(IOAdr + ATA_REG_DRVHD, DrvHd);
	hd_intr_occurd=0;
	outb(IOAdr + ATA_REG_CMD, CmdByte);
	nsleep(400);
}

/*****************************************************************************
	name:	ataCmd
	action:	ATA hard drive block read/write
	returns: 0 if OK
		-1 if drive could not be selected
		-2 if unsupported command
		-3 if command timed out
		-4 if bad/questionable drive status after command
		-5 if writing to read-only drive.
*****************************************************************************/
int ataCmd(DriveCmd *Cmd)
{	u8 Stat, CmdByte;
	u32 Count, Temp;
	u16 IOAdr;

	IOAdr=Drive[Cmd->Dev].IOAdr;
/* select the drive */
	if(ataSelect(IOAdr, Drive[Cmd->Dev].DrvSel))
	{	//printf("ataCmd: could not select drive\n");
		return(-1); }
	if(Cmd->Cmd == DRV_CMD_RD)
/* convert general block device command code into ATA command byte:
 * ATA_CMD_RDMUL if drive supports multi-sector reads, ATA_CMD_RD if not.
 */
	{	if(Drive[Cmd->Dev].MultSect < 2) CmdByte=ATA_CMD_RD;
		else CmdByte=ATA_CMD_RDMUL;

		while(Cmd->Count)
/* if drive supports multisector read/write, transfer as many sectors as
 * possible (fewer interrupts). We rely on MultSect to limit Temp (the
 * sector count) to < 256.
 */
		{	Temp=(Cmd->Count + ATA_SECTSIZE - 1)
				>> ATA_LG_SECTSIZE;
			Count=MIN(Temp, Drive[Cmd->Dev].MultSect);
			//DEBUG(("ataCmd: ready to read %lu sector(s) of %lu\n", Count, Temp));
/* compute CHS or LBA register values and write them, along with CmdByte */
			ataCmd2(Cmd, Count, CmdByte);
/* await read interrupt */
			if(awaitInterrupt(0xC000, WAIT_CMD) == 0)
			{	//printf("ataCmd: read timed out\n");
				(void)inb(IOAdr + ATA_REG_STAT);
				return(-3); }
/* check status */
			Stat=inb(IOAdr + ATA_REG_STAT);
			if((Stat & (0x81 | 0x58)) != 0x58)
			{	//printf("ataCmd: bad status (0x%02X) during read\n", Stat);
				return(-4); }
/* advance pointers, read data */
			Cmd->Blk += Count;
			Count <<= ATA_LG_SECTSIZE;
			insw(IOAdr + ATA_REG_DATA, (u16 *)Cmd->Data,
				Count >> 1);
			Cmd->Data += Count;
/* XXX - Cmd->Count had better be a multiple of 512... */
			Cmd->Count -= Count; }
		return(0); }
	else if(Cmd->Cmd == DRV_CMD_WR)
/* convert general block device command code into ATA command byte:
 * ATA_CMD_WRMUL if drive supports multi-sector reads, ATA_CMD_WR if not
 */
	{
#if SW_READONLY
	    //printf("ERROR: Software prevents writing!\n");
	    return -5;
#else
		if(Drive[Cmd->Dev].MultSect < 2) CmdByte=ATA_CMD_WR;
		else CmdByte=ATA_CMD_WRMUL;

		while(Cmd->Count)
/* if drive supports multisector read/write, transfer as many sectors as
 * possible (fewer interrupts). We rely on MultSect to limit Count
 * (the sector count) to < 256 
 */
		{	Temp=(Cmd->Count + ATA_SECTSIZE - 1)
				>> ATA_LG_SECTSIZE;
			Count=MIN(Temp, Drive[Cmd->Dev].MultSect);
			//DEBUG(("ataCmd: ready to write %lu sector(s) of %lu\n", Count, Temp));
/* compute CHS or LBA register values and write them, along with CmdByte */
			ataCmd2(Cmd, Count, CmdByte);
/* await DRQ */
			for(Temp=WAIT_DRQ; Temp; Temp--)
			{	if(inb(IOAdr + ATA_REG_ALTST) & 0x08) break;
				msleep(1); }	/* XXX - blocking delay */
			if(Temp == 0)
			{	//printf("ataCmd: no DRQ during write\n");
				(void)inb(IOAdr + ATA_REG_STAT);
				return(-3); }
/* advance pointer, write data */
			Cmd->Blk += Count;
			Count <<= ATA_LG_SECTSIZE;
			outsw(IOAdr + ATA_REG_DATA,
				(u16 *)Cmd->Data, Count >> 1);
/* await write interrupt */
			Temp=awaitInterrupt(0xC000, WAIT_CMD);
			if(Temp == 0)
			{	//printf("ataCmd: write timed out\n");
				(void)inb(IOAdr + ATA_REG_STAT);
				return(-3); }
/* check status */
			Stat=inb(IOAdr + ATA_REG_STAT);
			if((Stat & (0xA1 | 0x50)) != 0x50)
			{	//printf("ataCmd: bad status (0x%02X) during write\n", Stat);
				return(-4); }
/* advance pointers */
			Cmd->Data += Count;
/* XXX - Cmd->Count had better be a multiple of 512... */
			Cmd->Count -= Count; }
		return(0);
#endif
	}
	else
	{	//printf("ataCmd: bad cmd (%u)\n", Cmd->Cmd);
		return(-2);
	}
}

/*----------------------------------------------------------------------------
			GENERAL PARTITION STUFF
----------------------------------------------------------------------------*/

/*****************************************************************************
	    name:	partProbe
	action:	analyzes partitions on ATA drives
*****************************************************************************/
void
partProbe(void)
{	u8 Buffer[512], WhichDrive, WhichPart;
	unsigned HiHead, Scale, LHeads, Heads, Sects;
	long int Temp, Track;
	unsigned Offset, Cyls;
	DriveCmd Cmd;
	u8	type;
	
	cons_printf("\npartProbe:");
	hd_intr_occurd = 0;
	for(WhichDrive=0; WhichDrive < 4; WhichDrive++)
	{	if(Drive[WhichDrive].IOAdr == 0) continue;
		if(Drive[WhichDrive].Flags & ATA_FLG_ATAPI) continue;
/* load sector 0 (the partition table) */
		Cmd.Blk=0;
		Cmd.Count=512;
		Cmd.Dev=WhichDrive;
		Cmd.Cmd=DRV_CMD_RD;
		Cmd.Data=Buffer;
		Temp=ataCmd(&Cmd);
		if(Temp < 0) continue;
/* see if it's valid */
		if(Buffer[0x1FE] != 0x55 || Buffer[0x1FF] != 0xAA)
		{
			cons_printf("  hd%1u: invalid partition table",WhichDrive);
			continue; 
		}
/* check all four primary partitions for highest Heads value */
		HiHead=0;
		for(WhichPart=0; WhichPart < 4; WhichPart++)
		{	Offset=0x1BE + 16 * WhichPart;
			if(Buffer[Offset + 1] > HiHead)
				HiHead=Buffer[Offset + 1];
			if(Buffer[Offset + 5] > HiHead)
				HiHead=Buffer[Offset + 5]; 
		}
/* compare highest head value with heads/cylinder value from 'identify'.
 * Check for LARGE mode and determine Scale. This test will fail unless
 * partitions end on a cylinder boundary (hopefully, they do).
 */
		HiHead++;
		LHeads=Drive[WhichDrive].Heads;
		if(HiHead > LHeads)
		{	Scale=HiHead / LHeads;
			cons_printf("\n HiHead= %d LHeads= %d Scale %d",HiHead,LHeads,Scale);
			cons_printf("\nhd%1u: LARGE mode, N=", WhichDrive);
			//if(Scale * LHeads == HiHead){
				LHeads *= Scale;
				//printf("%u, new CHS=%u:%u:%u\n", Scale,Drive[WhichDrive].Cyls / Scale,LHeads,Drive[WhichDrive].Sects);  
			//}
/* HiHead / Drive[WhichDrive].Heads is not an integer. */
			//else //printf("??? (UNKNOWN !!!)\n"); 
		}

		// initialize partition information
		for(WhichPart=0; WhichPart < 4; WhichPart++)
		{
			Partitions[WhichPart].PartNo = WhichPart;
			Partitions[WhichPart].FileDescriptor = 0;
			Partitions[WhichPart].StartLBA = 0;
			Partitions[WhichPart].TotalLBAs = 0;
		}
/* now print geometry info for all primary partitions. CHS values in each
 * partition record may be faked for the benefit of MS-DOS/Win, so we ignore
 * them and use the 32-bit size-of-partition and sectors-preceding-partition
 * fields to compute CHS.
 */
		////printf("\n BUFFER READ \n");
		//for(i=0; i < 512; i++) {
		//	  //printf("%02x ",Buffer[i]);
		//	  if ((i+1) % 8 == 0) //printf("\n");
		//}
		//printf("\n");
		for(WhichPart=0; WhichPart < 4; WhichPart++)
		{
			Partitions[WhichPart].PartNo = WhichPart;
			Offset = 0x1BE + 16 * WhichPart;
/* get 32-bit sectors-preceding-partition; skip if undefined partition */
			Temp = *(u32 *)(Buffer + Offset + 8);
			//printf("\ndecider Temp %lu\n",Temp);
			if(Temp == 0) continue;
			type = Buffer[4 + Offset];
			Partitions[WhichPart].FileDescriptor = type;
			Partitions[WhichPart].StartLBA = Temp;
/* convert to CHS, using LARGE mode value of Heads if necessary */
			Sects = Temp % Drive[WhichDrive].Sects + 1;
			Track = Temp / Drive[WhichDrive].Sects;
			Heads = Track % LHeads;
			Cyls  = Track / LHeads;
			cons_printf("\nhd%1u partition %c: type=$%02x, start LBA=%8lu,start CHS=%4u:%2u:%2u, ",WhichDrive, 'a' + WhichPart, type,Temp,Cyls, Heads, Sects);
/* get 32-bit partition size */
			  Temp = *(u32 *)(Buffer + Offset + 12);
			  //printf("%lu sectors\n", Temp);
			  Partitions[WhichPart].TotalLBAs = Temp;
		}
	}
	for(WhichPart=0; WhichPart < 4; WhichPart++)
	{
		cons_printf ("\nPartitions %d: %ld, %ld, %ld",Partitions[WhichPart].PartNo,Partitions[WhichPart].FileDescriptor,Partitions[WhichPart].StartLBA,Partitions[WhichPart].TotalLBAs);
	}
}


/*----------------------------------------------------------------------------
			IRQ HANDLERS and SETUP
----------------------------------------------------------------------------*/

/*****************************************************************************
	name:	irqEnd
	action:	restores old IRQ handlers
*****************************************************************************/
void ata_irqEnd(void)
{
}

void microsec_wait(unsigned microsecs) {

    assert( 0 != (EF_INTR & get_eflags()) );	/* must have intrs enabled! */
    if( microsecs < 1000 ) {
	while( --microsecs > 0 ) {
	    IO_DELAY();		/* see <spede/flames.h> for details. */
	}
    } else {
	unsigned	done_tick = (microsecs / MICROSEC_TICK) + sys_tick;

#if SW_WATCH_TIMER
	//printf("{%d", done_tick - sys_tick);
#endif
	while( done_tick >= sys_tick ) {
	    pause();
	}
#if SW_WATCH_TIMER
	putchar('}');
#endif
    }
}   /* end microsec_wait() */	
