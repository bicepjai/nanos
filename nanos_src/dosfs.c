/*
	DOSFS Embedded FAT-Compatible Filesystem
	(C) 2005 Lewin A.R.W. Edwards (sysadm@zws.com)

	You are permitted to modify and/or use this code in your own projects without
	payment of royalty, regardless of the license(s) you choose for those projects.

	You cannot re-copyright or restrict use of the code as released by Lewin Edwards.
*/

#include "dosfs.h"

/*
	Get starting sector# of specified partition on drive #unit
	NOTE: This code ASSUMES an MBR on the disk.
	scratchsector should point to a SECTOR_SIZE scratch area
	Returns 0xffffffff for any error.
	If pactive is non-NULL, this function also returns the partition active flag.
	If pptype is non-NULL, this function also returns the partition type.
	If psize is non-NULL, this function also returns the partition size.
*/
u32 DFS_GetPtnStart(u8 unit, u8 *scratchsector, u8 pnum, u8 *pactive, u8 *pptype, u32 *psize)
{
	u32 result;
	u32 a,i;
	PMBR mbr = (PMBR) scratchsector;
	// DOS ptable supports maximum 4 partitions
	if (pnum > 3)
		return 0; //error

	// Read MBR from target media
	if (!DFS_ReadSector(scratchsector,0,1,unit)) {
		return 0; //error
	}
	result = (u32) mbr->ptable[pnum].start_0 | (((u32) mbr->ptable[pnum].start_1) << 8) |
	  (((u32) mbr->ptable[pnum].start_2) << 16) |  (((u32) mbr->ptable[pnum].start_3) << 24);
	if (pactive)
		*pactive = mbr->ptable[pnum].active;

	if (pptype)
		*pptype = mbr->ptable[pnum].type;

	if (psize)
		*psize = (u32) mbr->ptable[pnum].size_0 |
		  (((u32) mbr->ptable[pnum].size_1) << 8) |
		  (((u32) mbr->ptable[pnum].size_2) << 16) |
		  (((u32) mbr->ptable[pnum].size_3) << 24);
	return result;
}

/*
	Retrieve volume info from BPB and store it in a VOLINFO structure
	You must provide the unit and starting sector of the filesystem, and
	a pointer to a sector buffer for scratch
	Attempts to read BPB and glean information about the FS from that.
	Returns 0 OK, nonzero for any error.
*/
u32 DFS_GetVolInfo(u8 unit, u8 *scratchsector, u32 startsector, PVOLINFO volinfo)
{
	int i;
	PLBR lbr = (PLBR) scratchsector;
	volinfo->unit = unit;
	volinfo->startsector = startsector;
	if(!DFS_ReadSector(scratchsector,startsector,1,unit))
		return 0; //error
	
    
// tag: OEMID, refer dosfs.h
//	strncpy(volinfo->oemid, lbr->oemid, 8);
//	volinfo->oemid[8] = 0;

	volinfo->secperclus = lbr->bpb.secperclus;
	volinfo->reservedsecs = (u16) lbr->bpb.reserved_l | (((u16) lbr->bpb.reserved_h) << 8);
	volinfo->numsecs =  (u16) lbr->bpb.sectors_s_l | (((u16) lbr->bpb.sectors_s_h) << 8);

	if (!volinfo->numsecs)
	volinfo->numsecs = (u32) lbr->bpb.sectors_l_0 | (((u32) lbr->bpb.sectors_l_1) << 8) | (((u32) lbr->bpb.sectors_l_2) << 16) | (((u32) lbr->bpb.sectors_l_3) << 24);

	// If secperfat is 0, we must be in a FAT32 volume; get secperfat
	// from the FAT32 EBPB. The volume label and system ID string are also
	// in different locations for FAT12/16 vs FAT32.
	volinfo->secperfat =  (u16) lbr->bpb.secperfat_l |  (((u16) lbr->bpb.secperfat_h) << 8);
	if (!volinfo->secperfat) {
		//volinfo->secperfat = (u32) lbr->ebpb.ebpb32.fatsize_0 | (((u32) lbr->ebpb.ebpb32.fatsize_1) << 8) | (((u32) lbr->ebpb.ebpb32.fatsize_2) << 16) | (((u32) lbr->ebpb.ebpb32.fatsize_3) << 24);

		//memcpy(volinfo->label, lbr->ebpb.ebpb32.label, 11);
		volinfo->label[11] = 0;
	
// tag: OEMID, refer dosfs.h
//		memcpy(volinfo->system, lbr->ebpb.ebpb32.system, 8);
//		volinfo->system[8] = 0; 
	}
	else {
		memcpy(volinfo->label, lbr->ebpb.ebpb.label, 11);
		volinfo->label[11] = 0;
	
// tag: OEMID, refer dosfs.h
//		memcpy(volinfo->system, lbr->ebpb.ebpb.system, 8);
//		volinfo->system[8] = 0; 
	}

	// note: if rootentries is 0, we must be in a FAT32 volume.
	volinfo->rootentries =  (u16) lbr->bpb.rootentries_l | (((u16) lbr->bpb.rootentries_h) << 8);

	// after extracting raw info we perform some useful precalculations
	volinfo->fat1 = startsector + volinfo->reservedsecs;

	// The calculation below is designed to round up the root directory size for FAT12/16
	// and to simply ignore the root directory for FAT32, since it's a normal, expandable
	// file in that situation.
	if (volinfo->rootentries) {
		volinfo->rootdir = volinfo->fat1 + (volinfo->secperfat * 2);
		volinfo->dataarea = volinfo->rootdir + (((volinfo->rootentries * 32) + (SECTOR_SIZE - 1)) / SECTOR_SIZE);
	}
	else {
		volinfo->dataarea = volinfo->fat1 + (volinfo->secperfat * 2);
		/*volinfo->rootdir = (u32) lbr->ebpb.ebpb32.root_0 |
		  (((u32) lbr->ebpb.ebpb32.root_1) << 8) |
		  (((u32) lbr->ebpb.ebpb32.root_2) << 16) |
		  (((u32) lbr->ebpb.ebpb32.root_3) << 24);*/
	}

	// Calculate number of clusters in data area and infer FAT type from this information.
	volinfo->numclusters = (volinfo->numsecs - volinfo->dataarea) / volinfo->secperclus;
	if (volinfo->numclusters < 4085)
		volinfo->filesystem = FAT12;
	else if (volinfo->numclusters < 65525)
		volinfo->filesystem = FAT16;
	else
		volinfo->filesystem = FAT32;
	//printf ("*************exit DFS_GetVolInfo ************* \n");
	return 1;
}
