/*   ata.h
 *	@(#)ata.h	1.6   09/24/99
 */

#ifndef _INCL_ATA_H
#define _INCL_ATA_H

/* delay() inportb() outportb() inport() outport() */
#include	<spede/util.h>
#include	<spede/stdio.h>	/* NULL printf() cprintf() */
#include	<spede/machine/io.h>		/* For inportb() and outportb() */
#include	"structs.h"
#include	"externs.h"

#if SW_WATCH
/* extra debugging messages */
#define	DEBUG(X)	printf X
#else
/* no debug msgs */
#define	DEBUG(X)
#endif

/*----------------------------------------------------------------------------
		LOW LEVEL FUNCTIONS AND BASIC DEFINTIONS
----------------------------------------------------------------------------*/

/*  Sleep for milliseconds.  Use microsecond sleeper routine. */
#define		msleep(mill)		microsec_wait(mill*1000)

#define  outb		outportb
#define  outw		outportw
#define  inb		inportb
#define  inw		inportw

/* ---------------------------------------------------------------- */

/* Delays from Linux ide.h (milliseconds) */
#define	WAIT_READY	30	/* RDY asserted, use 5000 for notebook/APM */
#define	WAIT_ID		3000	/* ATA device responds to 'identify' */
#define	WAIT_PID	3	/* ATAPI device responds to 'identify' */
#define	WAIT_CMD	1000	/* IRQ occurs in response to command */
#define	WAIT_DRQ	20	/* DRQ asserted after ATA_CMD_WR(MUL) */
#define WAIT_SELECT	2000	/* Status.BUSY after drive select/soft-reset. */

/* 'Cmd' field of 'drivecmd' structure */
#define	DRV_CMD_RD	1
#define	DRV_CMD_WR	2

/* ATA sector size */
#define	ATA_LG_SECTSIZE		9	/* 512 byte ATA drive sectors */
#define	ATA_SECTSIZE		(1 << (ATA_LG_SECTSIZE))

/* ATAPI sector size */
#define	ATAPI_LG_SECTSIZE	11	/* 2K CD-ROM sectors */
#define	ATAPI_SECTSIZE		(1 << (ATAPI_LG_SECTSIZE))

/* ATA drive flags */
#define	ATA_FLG_ATAPI	0x0001		/* ATAPI drive */
#define ATA_FLG_LBA	0x0002		/* LBA-capable */
#define ATA_FLG_DMA	0x0004		/* DMA-capable */

/*
 * Status bits (ATA_REG_STAT):
 */
#define WDS_ERROR       0x1     /* Error */
#define WDS_ECC         0x4     /* Soft ECC error */
#define WDS_DRQ         0x8     /* Data request */
#define WDS_BUSY        0x80    /* Busy */

/*
 * Bits for controller port (ATA_REG_SLCT):
 */
#define CTLR_IDIS 	0x2           /* Disable controller interrupts */
#define CTLR_RESET 	0x4          /* Reset controller */
#define CTLR_4BIT 	0x8           /* Use four head bits */

/*
 * Partition no of MSDOS, NANOS
 File Descriptor Details
 */

#define PART_MSDOS	0
#define PART_OUROS	1
#define FD_MSDOS	0x06	/* File Descriptor of MSDOS FAT16 */
#define FD_OUROS	0x05	/* File Descriptor of MSDOS Extended */ 

/* ATA or ATAPI command structure */
typedef struct
{	u32  Blk;	/* in SECTORS */
	u32  Count;	/* in BYTES */
	u8   Dev;
	u8   Cmd;	/* DRV_CMD_RD or DRV_CMD_WR */
	u8 * Data;
} DriveCmd;

/* generalized drive info structure */
typedef struct
{	u16  Flags;
	u8   DrvSel;		/* ATA, ATAPI only (LUN for SCSI?) */
	u8   MultSect;		/* ATA only */
	u16  Sects, Heads, Cyls; /* CHS ATA only */
	u16  IOAdr;
} DriveSpec;

typedef struct
{
	u32	PartNo;		/* Part No */
	u8	FileDescriptor;	/* File Descriptor */
	u32	StartLBA;		/* Starting Sector */
	u32	TotalLBAs; 	/* Total no of Sectors */
} PartitionInfo;

extern volatile int	InterruptOccured;
extern DriveSpec	Drive[4];
extern PartitionInfo	Partitions[4];
/* ---------------------------------------------------------------- */

extern void	ata_irqStart(void);
extern void	ata_irqEnd(void);

extern void	dump(u8 * Data, unsigned Count);

extern int 	awaitInterrupt(u16 IRQMask, unsigned Timeout);
extern int	ataSelect(unsigned IOAdr, unsigned Sel);
extern void	ataProbe(void);
extern void	ataCmd2(DriveCmd * Cmd, u8 Count, u8 CmdByte);
extern int	ataCmd(DriveCmd * Cmd);

extern int	atapiCmd(DriveCmd * Cmd);
extern int	atapiPlay(unsigned WhichDrive, u32 Start, u32 End);
extern int	atapiTOC(unsigned WhichDrive);
extern int	atapiPause(unsigned Play, unsigned WhichDrive);
extern void	partProbe(void);

#endif

/* end ata.h */
