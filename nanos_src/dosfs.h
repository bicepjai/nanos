
#ifndef _DOSFS_H
#define _DOSFS_H

#include	<spede/util.h>
#include	<spede/stdio.h>	/* NULL printf() cprintf() */
#include	<spede/machine/io.h>		/* For inportb() and outportb() */
#include	"externs.h"
#include	"ata_process.h"

#define SECTOR_SIZE 512

// Internal subformat identifiers
#define FAT12			0
#define FAT16			1
#define FAT32			2
/*
	Partition table entry structure
*/
typedef struct _tagPTINFO {
	u8		active;			// 0x80 if partition active
	u8		start_h;		// starting head
	u8		start_cs_l;		// starting cylinder and sector (low byte)
	u8		start_cs_h;		// starting cylinder and sector (high byte)
	u8		type;			// type ID byte
	u8		end_h;			// ending head
	u8		end_cs_l;		// ending cylinder and sector (low byte)
	u8		end_cs_h;		// ending cylinder and sector (high byte)
	u8		start_0;		// starting sector# (low byte)
	u8		start_1;		//
	u8		start_2;		//
	u8		start_3;		// starting sector# (high byte)
	u8		size_0;			// size of partition (low byte)
	u8		size_1;			//
	u8		size_2;			//
	u8		size_3;			// size of partition (high byte)
} PTINFO, *PPTINFO;

/*
	Master Boot Record structure
*/
typedef struct _tagMBR {
	u8 bootcode[0x1be];	// boot sector
	PTINFO ptable[4];			// four partition table structures
	u8 sig_55;				// 0x55 signature byte
	u8 sig_aa;				// 0xaa signature byte
} MBR, *PMBR;

/*
	BIOS Parameter Block structure (FAT12/16)
*/
typedef struct _tagBPB {
	u8 bytepersec_l;	// bytes per sector low byte (0x00)
	u8 bytepersec_h;	// bytes per sector high byte (0x02)
	u8 secperclus;	// sectors per cluster (1,2,4,8,16,32,64,128 are valid)
	u8 reserved_l;		// reserved sectors low byte
	u8 reserved_h;		// reserved sectors high byte
	u8 numfats;		// number of FAT copies (2)
	u8 rootentries_l;	// number of root dir entries low byte (0x00 normally)
	u8 rootentries_h;	// number of root dir entries high byte (0x02 normally)
	u8 sectors_s_l;		// small num sectors low byte
	u8 sectors_s_h;		// small num sectors high byte
	u8 mediatype;		// media descriptor byte
	u8 secperfat_l;		// sectors per FAT low byte
	u8 secperfat_h;		// sectors per FAT high byte
	u8 secpertrk_l;		// sectors per track low byte
	u8 secpertrk_h;		// sectors per track high byte
	u8 heads_l;		// heads low byte
	u8 heads_h;		// heads high byte
	u8 hidden_0;		// hidden sectors low byte
	u8 hidden_1;		// (note - this is the number of MEDIA sectors before
	u8 hidden_2;		// first sector of VOLUME - we rely on the MBR instead)
	u8 hidden_3;		// hidden sectors high byte
	u8 sectors_l_0;		// large num sectors low byte
	u8 sectors_l_1;		//
	u8 sectors_l_2;		//
	u8 sectors_l_3;		// large num sectors high byte
} BPB, *PBPB;

/*
	Extended BIOS Parameter Block structure (FAT12/16)
*/
typedef struct _tagEBPB {
	u8 unit;		// int 13h drive#
	u8 head;		// archaic, used by Windows NT-class OSes for flags
	u8 signature;		// 0x28 or 0x29
	u8 serial_0;		// serial#
	u8 serial_1;		// serial#
	u8 serial_2;		// serial#
	u8 serial_3;		// serial#
	u8 label[11];		// volume label
	u8 system[8];		// filesystem ID
} EBPB, *PEBPB;

/*
	Extended BIOS Parameter Block structure (FAT32)
*/
typedef struct _tagEBPB32 {
	uint8_t fatsize_0;			// big FAT size in sectors low byte

	uint8_t fatsize_1;			//

	uint8_t fatsize_2;			//

	uint8_t fatsize_3;			// big FAT size in sectors high byte

	uint8_t extflags_l;			// extended flags low byte

	uint8_t extflags_h;			// extended flags high byte

	uint8_t fsver_l;			// filesystem version (0x00) low byte

	uint8_t fsver_h;			// filesystem version (0x00) high byte

	uint8_t root_0;				// cluster of root dir, low byte

	uint8_t root_1;				//

	uint8_t root_2;				//

	uint8_t root_3;				// cluster of root dir, high byte

	uint8_t fsinfo_l;			// sector pointer to FSINFO within reserved area, low byte (2)

	uint8_t fsinfo_h;			// sector pointer to FSINFO within reserved area, high byte (0)

	uint8_t bkboot_l;			// sector pointer to backup boot sector within reserved area, low byte (6)

	uint8_t bkboot_h;			// sector pointer to backup boot sector within reserved area, high byte (0)

	uint8_t reserved[12];		// reserved, should be 0

	uint8_t unit;				// int 13h drive#

	uint8_t head;				// archaic, used by Windows NT-class OSes for flags

	uint8_t signature;			// 0x28 or 0x29

	uint8_t serial_0;			// serial#

	uint8_t serial_1;			// serial#

	uint8_t serial_2;			// serial#

	uint8_t serial_3;			// serial#

	uint8_t label[11];			// volume label

	uint8_t system[8];			// filesystem ID

} EBPB32, *PEBPB32;

/*
	Logical Boot Record structure (volume boot sector)
*/
typedef struct _tagLBR {
	u8 jump[3];		// JMP instruction
	u8 oemid[8];		// OEM ID, space-padded
	BPB bpb;		// BIOS Parameter Block
	union {
		EBPB ebpb;	// FAT12/16 Extended BIOS Parameter Block
		//EBPB32 ebpb32;	// FAT32 Extended BIOS Parameter Block
	} ebpb;
	u8 code[448];		// boot sector code
	u8 sig_55;		// 0x55 signature byte
	u8 sig_aa;		// 0xaa signature byte
} LBR, *PLBR;

/*
	Volume information structure (Internal to DOSFS)
*/
typedef struct _tagVOLINFO {
	u8 unit;				// unit on which this volume resides
	u8 filesystem;			// formatted filesystem

// These two fields aren't very useful, so support for them has been commented out to
// save memory. (Note that the "system" tag is not actually used by DOS to determine
// filesystem type - that decision is made entirely on the basis of how many clusters
// the drive contains. DOSFS works the same way).
// See tag: OEMID in dosfs.c
//	u8 oemid[9];			// OEM ID ASCIIZ
//	u8 system[9];			// system ID ASCIIZ
	u8 label[12];			// volume label ASCIIZ
	u32 startsector;		// starting sector of filesystem
	u8 secperclus;			// sectors per cluster
	u16 reservedsecs;		// reserved sectors
	u32 numsecs;			// number of sectors in volume
	u32 secperfat;			// sectors per FAT
	u16 rootentries;		// number of root dir entries

	u32 numclusters;		// number of clusters on drive

	// The fields below are PHYSICAL SECTOR NUMBERS.
	u32 fat1;			// starting sector# of FAT copy 1
	u32 rootdir;			// starting sector# of root directory (FAT12/FAT16) or cluster (FAT32)
	u32 dataarea;			// starting sector# of data area (cluster #2)
} VOLINFO, *PVOLINFO;

/*
	Get starting sector# of specified partition on drive #unit
	NOTE: This code ASSUMES an MBR on the disk.
	scratchsector should point to a SECTOR_SIZE scratch area
	Returns 0xffffffff for any error.
	If pactive is non-NULL, this function also returns the partition active flag.
	If pptype is non-NULL, this function also returns the partition type.
	If psize is non-NULL, this function also returns the partition size.
*/
u32 DFS_GetPtnStart(u8 unit, u8 *scratchsector, u8 pnum, u8 *pactive, u8 *pptype, u32 *psize);

/*
	Retrieve volume info from BPB and store it in a VOLINFO structure
	You must provide the unit and starting sector of the filesystem, and
	a pointer to a sector buffer for scratch
	Attempts to read BPB and glean information about the FS from that.
	Returns 0 OK, nonzero for any error.
*/
u32 DFS_GetVolInfo(u8 unit, u8 *scratchsector, u32 startsector, PVOLINFO volinfo);

#endif // _DOSFS_H
