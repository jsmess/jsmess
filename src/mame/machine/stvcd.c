/***************************************************************************

  machine/stvcd.c - Sega Saturn and ST-V CD-ROM handling

  Rewritten Dec. 2005 by R. Belmont, based on the abandoned "Sim"
  Saturn/ST-V emulator by RB & The_Author.

  Status: this is sufficient for the Saturn BIOS to recognize and
  boot discs but actual games demand a lot more than this
  implementation has to give at the moment.  In particular,
  buffer handling is a sick hack and filters need to be supported
  (fixes for both are in progress).

  Information sources:
  - Tyranid's document
  - A commented disassembly I made of the Saturn BIOS's CD code

  More info is welcome!  Please private message RB on mame.net,
  mameworld.net, or bannister.org's message boards if you can
  supply it.

  ------------------------------------------------------------

  Stuff we know about the CD protocol will collect here.

  Top byte of CR1 is always status.
  This status is the same for all commands.

  See CD_STAT_ defines below for definitions.

  Address is mostly in terms of FAD (Frame ADdress).
  FAD is absolute number of frames from the start of the disc.
  In other words, FAD = LBA + 150; FAD is the same units as
  LBA except it counts starting at absolute zero instead of
  the first sector (00:02:00 in MSF format).

  Commands executed on the way to booting a disk:

CD: Initialize CD system (PC=4232)
CD: Set CD Device Connection (PC=4232)
CD: Get Session Info (PC=4232)
CD: Play Disk (PC=4232)
CD: Get Buffer Size (PC=4232)
CD: Get and delete sector data (PC=4232)
CD: End data transfer (PC=4232)
CD: Change Directory (PC=4232)
CD: Get file system scope(PC=4232)
CD: Read File (PC=4232)
CD: Get Status (PC=4232)
CD: Get File Info (PC=4232)
CD: End data transfer (PC=4232)
CD: Get Buffer Size (PC=4232)
CD: Get and delete sector data (PC=4232)
CD: End data transfer (PC=4232)

***************************************************************************/

#include "driver.h"
#ifdef MESS
#include "devices/chd_cd.h"
#endif
#include "cdrom.h"
#include "stvcd.h"

static cdrom_file *cdrom = (cdrom_file *)NULL;

static void cd_readTOC(void);
static void cd_readblock(unsigned long fad, unsigned char *dat);
static void cd_playdata(void);

typedef struct
{
	unsigned char flags;		// iso9660 flags
	unsigned long length;		// length of file
	unsigned long firstfad;		// first sector of file
	unsigned char name[128];
} direntryT;

typedef struct
{
   UINT8 mode;
   UINT8 chan;
   UINT8 smmask;
   UINT8 cimask;
   UINT8 fid;
   UINT8 smval;
   UINT8 cival;
   UINT8 condtrue;
   UINT8 condfalse;
   UINT32 fad;
   UINT32 range;
} filterT;

#define MAX_FILTERS	(24)

// local variables
static UINT16 cr1, cr2, cr3, cr4;
static UINT16 hirqmask, hirqreg;
static int state;
static UINT16 cd_stat;
static unsigned int cd_curfad = 0;
static UINT32 in_buffer = 0;	// amount of data in the buffer
static int oddframe = 0;
static UINT32 wordsread = 0;
static UINT32 fadstoplay = 0;
static int buffull, sectorstore, freeblocks;
static filterT filters[MAX_FILTERS];
static filterT *cddevice;
static int cddevicenum;

// data transfer buffer
static UINT8 databuf[(512*1024)];	// saturn has a 512k CD data buffer
static UINT32 dbufptr;
static UINT32 dbuflen;

// iso9660 utilities
static void read_new_dir(unsigned long fileno);
static void make_dir_current(unsigned long fad);

static direntryT curroot;		// root entry of current filesystem
static direntryT *curdir;		// current directory
static int numfiles;			// # of entries in current directory
static int firstfile;			// first non-directory file

enum
{
	STATE_UNK = 0,
	STATE_HWINFO
};

// HIRQ definitions
#define CMOK 0x0001 // command ok / ready for new command
#define DRDY 0x0002 // drive ready
#define CSCT 0x0004 //
#define BFUL 0x0008 // buffer full
#define PEND 0x0010 // command pending
#define DCHG 0x0020 // disc change / tray open
#define ESEL 0x0040 // soft reset, end of blah
#define EHST 0x0080 //
#define ECPY 0x0100 //
#define EFLS 0x0200 // stop execution of cd block filesystem
#define SCDQ 0x0400 // subcode Q renewal complete
#define MPED 0x0800 // MPEG
#define MPCM 0x1000 // MPEG
#define MPST 0x2000 // MPEG

// CD status (hi byte of CR1) definitions:
// (these defines are shifted up 8)
#define CD_STAT_BUSY     0x0000		// status change in progress
#define CD_STAT_PAUSE    0x0100		// CD block paused (temporary stop)
#define CD_STAT_STANDBY  0x0200		// CD drive stopped
#define CD_STAT_PLAY     0x0300		// CD play in progress
#define CD_STAT_SEEK     0x0400		// drive seeking
#define CD_STAT_SCAN     0x0500		// drive scanning
#define CD_STAT_OPEN     0x0600		// tray is open
#define CD_STAT_NODISC   0x0700		// no disc present
#define CD_STAT_RETRY    0x0800		// read retry in progress
#define CD_STAT_ERROR    0x0900		// read data error occured
#define CD_STAT_FATAL    0x0a00		// fatal error (hard reset required)
#define CD_STAT_PERI     0x2000		// periodic response if set, else command response
#define CD_STAT_TRANS    0x4000		// data transfer request if set
#define CD_STAT_WAIT     0x8000		// waiting for command if set, else executed immediately
#define CD_STAT_REJECT   0xff00		// ultra-fatal error.

// global functions
void stvcd_reset(void)
{
	state = STATE_UNK;
	hirqmask = 0xffff;
	hirqreg = 0;
	cr1 = 'C';
	cr2 = ('D'<<8) | 'B';
	cr3 = ('L'<<8) | 'O';
	cr4 = ('C'<<8) | 'K';
	cd_stat = 0;

	if (curdir != (direntryT *)NULL)
		free((void *)curdir);
	curdir = (direntryT *)NULL;		// no directory yet

	// reset flag vars
	buffull = sectorstore = 0;

	freeblocks = 200;

	// open device
	if (cdrom)
	{
		cdrom_close(cdrom);
		cdrom = (cdrom_file *)NULL;
	}

	#ifdef MESS
	cdrom = mess_cd_get_cdrom_file_by_number(0);
	#else
	cdrom = cdrom_open(get_disk_handle(0));
	#endif

	memset(databuf, 0, (512*1024));

	dbufptr = 0;
	dbuflen = 0;
	wordsread = 0;

	if (cdrom)
	{
		logerror("Opened CD-ROM successfully, reading root directory\n");
		read_new_dir(0xffffff);	// read root directory
	}
}

static UINT16 cd_readWord(UINT32 addr)
{
	unsigned short rv;

	switch (addr & 0xffff)
	{
		case 0x0008:	// read HIRQ register
			hirqreg &= ~DCHG;	// always clear bit 6 (tray open)
//          logerror("CD: R HIRR (%04x) (PC=%x)\n", hirqreg,   activecpu_get_pc());
			rv = hirqreg;

			if (buffull) rv |= BFUL;
			if (sectorstore) rv |= CSCT;

			return rv;
			break;

		case 0x000C:
//          logerror("CD:\tRW HIRM (%04x)  (PC=%x)", hirqmask,   activecpu_get_pc());
			return hirqmask;
			break;

		case 0x0018:
			rv = cd_stat;
			if (!(cd_stat & CD_STAT_PERI) && (cd_stat & CD_STAT_SEEK))
			{
			 	cd_stat &= ~CD_STAT_SEEK;
				cd_stat |= CD_STAT_PAUSE;
			}
			cd_stat |= CD_STAT_PERI;	// always revert to periodic responses after a command
			if (cr1 == 'C')		// first read on cold boot?
			{
				rv = cr1;
				cr1 = 0;
				cd_stat = CD_STAT_PAUSE;
			}
//          logerror("CD: R CR1 (%04x)  (PC=%x HIRR=%04x)\n", rv, activecpu_get_pc(), hirqreg);
			return rv;
			break;

		case 0x001c:
//          logerror("CD: R CR2 (%04x)  (PC=%x HIRR=%04x)\n", cr2,   activecpu_get_pc(), hirqreg);
			return cr2;
			break;

		case 0x0020:
//          logerror("CD: R CR3 (%04x)  (PC=%x HIRR=%04x)\n", cr3,   activecpu_get_pc(), hirqreg);
#if 0
			if (cd_stat & CD_STAT_PERI)	   	// for periodic responses, return
			{								// the disc position
				cr3 = (cd_curfad>>16)&0xff;
				cr4 = (cd_curfad & 0xffff);
			}
#endif
			return cr3;
			break;

		case 0x0024:
//          logerror("CD: R CR4 (%04x)  (PC=%x HIRR=%04x)\n", cr4,   activecpu_get_pc(), hirqreg);
			rv = cr4;
			if (state == STATE_HWINFO)
			{
				hirqreg |= DRDY;
				cd_stat = CD_STAT_PERI|CD_STAT_PAUSE;	// switch to status readout after a read - bios wants state A
				cr1 = 0;
				cr2 = 0x4101;
				cr3 = 0x100;
				cr4 = 0;
				state = STATE_UNK;
			}
			return rv;
			break;

		case 0x8000:
			wordsread++;
			rv = databuf[dbufptr]<<8 | databuf[dbufptr+1];
//          logerror( "CD: RW %04x DATA (PC=%x)\n", rv,   activecpu_get_pc());
			if (dbufptr < (512*1024))
			{
				dbufptr += 2;
			}
			else
			{
				logerror(  "CD: attempt to read past data buffer end!");
			}
			return rv;
			break;

		default:
			logerror( "CD: RW %08x (PC=%x)\n", addr,   activecpu_get_pc());
			return 0xffff;
			break;
	}

	return 0xffff;
}

static UINT32 cd_readLong(UINT32 addr)
{
	unsigned long rv;

	switch (addr & 0xffff)
	{
		case 0x8000:
			wordsread++;

			rv = databuf[dbufptr]<<24 | databuf[dbufptr+1]<<16 | databuf[dbufptr+2]<<8 | databuf[dbufptr+3];

//          logerror( "CD: RL %08x DATA (PC=%x)", rv,   activecpu_get_pc());
			if (dbufptr < (512*1024))
			{
				dbufptr += 4;
			}
			else
			{
				logerror(  "CD: attempt to read past data buffer end!\n");
			}
			return rv;
			break;

		default:
			logerror( "CD: RL %08x (PC=%x)\n", addr,   activecpu_get_pc());
			return 0xffff;
			break;
	}
}

static void cd_writeWord(UINT32 addr, UINT16 data)
{
	unsigned long temp;

	switch(addr & 0xffff)
	{
	case 0x0008:
//      logerror("CD: W HIRR %04x (PC=%x)\n", data,   activecpu_get_pc());
		hirqreg |= data;	// never let write to HIRQ clear things (?)
		return;
	case 0x000C:
//      logerror("CD: W HIRM %04x (PC=%x)\n", data,   activecpu_get_pc());
		hirqmask = data;
		return;
	case 0x0018:
//      logerror("CD: W CR1 %04x (PC=%x)\n", data,   activecpu_get_pc());
		cr1 = data;
		cd_stat &= ~CD_STAT_PERI;
		break;
	case 0x001c:
//      logerror("CD: W CR2 %04x (PC=%x)\n", data,   activecpu_get_pc());
		cr2 = data;
		break;
	case 0x0020:
//      logerror("CD: W CR3 %04x (PC=%x)\n", data,   activecpu_get_pc());
		cr3 = data;
		break;
	case 0x0024:
//      logerror("CD: W CR4 %04x (PC=%x)\n", data,   activecpu_get_pc());
		cr4 = data;
		switch (cr1 & 0xff00)
		{
		case 0x0000:
			logerror( "CD: Get Status (PC=%x)\n",   activecpu_get_pc());

			// values taken from a US saturn with a disc in and the lid closed
			hirqreg = 0xfe3;
			cd_stat = CD_STAT_PAUSE;
			cd_stat &= ~CD_STAT_PERI;
			cr2 = 0x4100;
			cr3 = 0x100;
			cr4 = 0;
			break;

		case 0x0100:
			logerror( "CD: Get HW Info (PC=%x)\n",   activecpu_get_pc());
			hirqreg = 0xfe1;
			cd_stat = CD_STAT_PERI|CD_STAT_PAUSE;
			cd_stat = 0;
			cr2 = 0x0002;
			cr3 = 0x0000;
			cr4 = 0x0400;
			state = STATE_HWINFO;
			break;

		case 0x200:	// Get TOC
			logerror( "CD: Get TOC (PC=%x)\n",   activecpu_get_pc());
			cd_readTOC();
			cd_stat = CD_STAT_TRANS|CD_STAT_PAUSE;
			cr2 = dbuflen/2;	// TOC length
			cr3 = 0;
			cr4 = 0;
			hirqreg |= (CMOK|DRDY);
			break;

		case 0x0300:	// get session info (lower byte = session # to get?)
		            	// bios is interested in returns in cr3 and cr4
						// cr3 should be data track #
						// cr4 must be > 1 and < 100 or bios gets angry.
			logerror( "CD: Get Session Info (PC=%x)\n",   activecpu_get_pc());
			switch (cr1 & 0xff)
			{
				case 0:	// get total session info / disc start
					cr2 = 0;
					cr3 = 0x0100;	// total # of sessions in high byte (maybe should be 2?)
					cr4 = 0;
					break;

				case 1:	// get total session info / disc start
					cr2 = 0;
					cr3 = 0x0100;	// this should be 1 for track 1 or something
					cr4 = 0;
					break;

				default:
					fatalerror("CD: Unknown request to Get Session Info %x", cr1 & 0xff);
					break;
			}
			cd_stat = CD_STAT_PAUSE;
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK);
			break;

		case 0x400:	// initialize CD system
				// CR1 & 1 = reset software
				// CR1 & 2 = decode RW subcode
				// CR1 & 4 = don't confirm mode 2 subheader
				// CR1 & 8 = retry reading mode 2 sectors
				// CR1 & 10 = force single-speed
			logerror( "CD: Initialize CD system (PC=%x)\n",   activecpu_get_pc());
			hirqreg |= (CMOK|DRDY|ESEL);
			cd_stat = CD_STAT_PERI|CD_STAT_PAUSE;
			cd_curfad = 150;
			dbufptr = 0;
			in_buffer = 0;
			buffull = 0;
			cr2 = 0x4101;
			cr3 = 0x100 | ((cd_curfad>>16)&0xff);
			cr4 = cd_curfad;
			break;

		case 0x0600:	// end data transfer
						// returns # of bytes transfered (24 bits) in
						// low byte of cr1 (MSB) and cr2 (middle byte, LSB)
			logerror( "CD: End data transfer (PC=%x)\n",   activecpu_get_pc());
			hirqreg |= (CMOK|ESEL|EFLS|SCDQ|DRDY);
			temp = wordsread;	// (should this return the # of words actually read?)
			temp &= 0xffffff;
			cd_stat &= 0xff00;
			cd_stat |= ((temp>>16)&0xff);
			cd_stat &= ~CD_STAT_PERI;
			cr2 = (temp&0xffff);
			dbufptr = 0;  		// reset buffer pointers
			wordsread = 0;		// this command will also reset the read count
			break;

		case 0x1000: // Play Disk.  FAD is in lowest 7 bits of cr1 and all of cr2.
			logerror( "CD: Play Disk (PC=%x)\n",   activecpu_get_pc());
			cd_stat = CD_STAT_PLAY|0x80;	// set "cd-rom" bit
			cd_curfad = ((cr1&0xff)<<16) | cr2;
			fadstoplay = ((cr3&0xff)<<16) | cr4;

			if (cd_curfad & 0x800000)
			{
				if (cd_curfad != 0xffffff)
				{
					// fad mode
					cd_curfad &= 0xfffff;
					fadstoplay &= 0xfffff;
				}
			}
			else
			{
				// track mode
				fatalerror("CD: Play Disk track mode, not yet implemented");
			}

			logerror("CD: Play Disk: start %x length %x\n", cd_curfad, fadstoplay);

			cr2 = 0x4101;	// ctrl/adr in hi byte, track # in low byte
			cr3 = 0x100;	// index of subcode in hi byte, frame address
			cr4 = 0;		// frame address
			hirqreg |= (CMOK|DRDY);
			oddframe = 0;
			dbufptr = 0;  		// reset buffer pointers
			in_buffer = 0;

			// and do the disc I/O
			cd_playdata();
			break;

		case 0x1100: // disk seek
			logerror( "CD: Disk seek (PC=%x)\n",   activecpu_get_pc());
			temp = (cr1&0x7f)<<16;
			temp |= cr2;

			hirqreg |= (CMOK|DRDY);
			hirqreg = 0x1;
			cd_stat = CD_STAT_SEEK;
			cr2 = 0x4101;
			cr3 = (temp>>16)&0xff;
			cr4 = (temp&0xffff);
			break;

		case 0x3000:	// Set CD Device connection
			{
				UINT8 parm;

				logerror( "CD: Set CD Device Connection (PC=%x) op %x\n",   activecpu_get_pc(), cr3>>8);

				// get operation
				parm = cr3>>8;

				cddevicenum = parm;

				if (parm == 0xff)
				{
					cddevice = (filterT *)NULL;
				}
				else
				{
					if (parm < 0x24)
					{
						cddevice = &filters[(cr3>>8)];
					}
				}

	//          cd_stat = CD_STAT_PAUSE;
				hirqreg |= (CMOK|ESEL);
			}
			break;

		case 0x4000:	// Set Filter Range
						// cr1 low + cr2 = FAD0, cr3 low + cr4 = FAD1
						// cr3 hi = filter num.
			logerror( "CD: Set Filter Range (PC=%x)\n",   activecpu_get_pc());
			hirqreg |= (CMOK|ESEL|DRDY);
			cr2 = 0x4101;	// ctrl/adr in hi byte, track # in low byte
			cr3 = 0x0100|((cd_curfad>>16)&0xff);
			cr4 = (cd_curfad & 0xffff);
			break;

		case 0x4200:	// Set Filter Subheader conditions
			{
				UINT8 fnum = (cr3>>8)&0xff;

				logerror( "CD: Set Filter Subheader conditions (PC=%x) %x => chan %x masks %x fid %x vals %x\n", activecpu_get_pc(), fnum, cr1&0xff, cr2, cr3&0xff, cr4);

				filters[fnum].chan = cr1 & 0xff;
				filters[fnum].smmask = (cr2>>8)&0xff;
				filters[fnum].cimask = cr2&0xff;
				filters[fnum].fid = cr3&0xff;
				filters[fnum].smval = (cr4>>8)&0xff;
				filters[fnum].cival = cr4&0xff;

				hirqreg |= (CMOK|ESEL);
				cr2 = 0x4101;	// ctrl/adr in hi byte, track # in low byte
				cr3 = 0x0100|((cd_curfad>>16)&0xff);
				cr4 = (cd_curfad & 0xffff);
			}
			break;

		case 0x4400:	// Set Filter Mode
			{
				UINT8 fnum = (cr3>>8)&0xff;
				UINT8 mode = (cr1 & 0xff);

				// initialize filter?
				if (mode & 0x80)
				{
					memset(&filters[fnum], 0, sizeof(filterT));
				}
				else
				{
					filters[fnum].mode = mode;
				}

				logerror( "CD: Set Filter Mode (PC=%x) filt %x mode %x\n", activecpu_get_pc(), fnum, mode);
				hirqreg |= (CMOK|ESEL|DRDY);
				cr2 = 0x4101;	// ctrl/adr in hi byte, track # in low byte
				cr3 = 0x0100|((cd_curfad>>16)&0xff);
				cr4 = (cd_curfad & 0xffff);
			}
			break;

		case 0x4600:	// Set Filter Connection
			{
				UINT8 fnum = (cr3>>8)&0xff;

				logerror( "CD: Set Filter Connection (PC=%x) %x => mode %x parm %04x\n", activecpu_get_pc(), fnum, cr1 & 0xf, cr2);

				// set true condition?
				if (cr1 & 1)
				{
					filters[fnum].condtrue = (cr2>>8)&0xff;
				}
				else if (cr1 & 2)	// set false condition
				{
					filters[fnum].condfalse = cr2&0xff;
				}

				hirqreg |= (CMOK|ESEL);
				cr2 = 0x4101;	// ctrl/adr in hi byte, track # in low byte
				cr3 = 0x0100|((cd_curfad>>16)&0xff);
				cr4 = (cd_curfad & 0xffff);
			}
			break;

		case 0x4800:	// Reset Selector
			logerror( "CD: Reset Selector (PC=%x)\n",   activecpu_get_pc());
			hirqreg |= (CMOK|ESEL|DRDY);
			cr2 = 0x4101;	// ctrl/adr in hi byte, track # in low byte
			cr3 = 0x0100|((cd_curfad>>16)&0xff);
			cr4 = (cd_curfad & 0xffff);
			break;

		case 0x5000:	// get CD block size
			logerror( "CD: Get CD Block Size (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;	// zap periodic response
			cr2 = ((512*1024)-in_buffer)/2048;
			cr3 = 0x1800;
			cr4 = 0x00c8;
			hirqreg |= (CMOK|ESEL);
			break;

		case 0x5100:	// get buffer size
			logerror( "CD: Get Buffer Size (PC=%x) => %x\n",   activecpu_get_pc(), in_buffer);
			cd_stat &= ~CD_STAT_PERI;	// zap periodic response
			cr2 = 200;			// free blocks
			cr3 = 0x1800;			// max selectors
			cr4 = in_buffer/2048;		// buffer size in sectors (2048 byte units)
			hirqreg |= (CMOK);
			break;

		case 0x5200:	// calculate acutal size
			logerror( "CD: Calculate actual size (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|DRDY);
			cr2 = 0x4101;	// CTRL/track
			cr3 = (cd_curfad>>16)&0xff;
			cr4 = (cd_curfad & 0xffff);
			break;

		case 0x5300:	// get actual block size
			logerror( "CD: Get actual block size (PC=%x)\n", activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|DRDY);
			temp = in_buffer/2;	// must be in words
			temp &= 0xffffff;
			cd_stat &= 0xff00;
			cd_stat |= ((temp>>16)&0xff);
			cr2 = (temp&0xffff);
			cr3 = cr4 = 0;
			break;

		case 0x6000:	// set sector length
			logerror( "CD: Set sector length (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|ESEL|EFLS|SCDQ|DRDY);
			break;

		case 0x6100:	// get sector data
			logerror( "CD: Get sector data (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			cd_stat |= CD_STAT_TRANS;
			hirqreg |= (CMOK|ESEL|EFLS|SCDQ|DRDY);
			break;

		case 0x6200:	// delete sector data
			logerror( "CD: Delete sector data (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			cd_stat |= CD_STAT_TRANS;
			hirqreg |= (CMOK|ESEL|EFLS|SCDQ|DRDY);
			in_buffer = 0;	// zap sector data here

			// go ahead and read more.  GFS uses this for Nw mode on
			// multiblock reads.
			fadstoplay = 6;	// go 6 sectors at a time - oughta beat the timeout
			cd_playdata();
//          logerror(  "Got next sector (curFAD=%ld), in_buffer = %ld", cd_curfad, in_buffer);
			break;

		case 0x6300:	// get then delete sector data
			logerror( "CD: Get and delete sector data (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			cd_stat |= CD_STAT_TRANS;
			hirqreg |= (CMOK|ESEL|EFLS|SCDQ|DRDY);
			break;

		case 0x6700:	// get copy error
			logerror( "CD: Get copy error (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|ESEL|EFLS|SCDQ|DRDY);
			break;

		case 0x7000:	// change directory
			logerror( "CD: Change Directory (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|DRDY);

			temp = (cr3&0xff)<<16;
			temp |= cr4;
			read_new_dir(temp);
			break;

		case 0x7100:	// Read directory entry
			logerror( "CD: Read Directory Entry (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|DRDY);

			temp = (cr3&0xff)<<16;
			temp |= cr4;
			cr2 = 0x4101;	// CTRL/track
			cr3 = (curdir[temp].firstfad>>16)&0xff;
			cr4 = (curdir[temp].firstfad&0xffff);
			break;

		case 0x7200:	// Get file system scope
			logerror( "CD: Get file system scope(PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|DRDY);
			cr2 = numfiles;	// # of files in directory
			cr3 = 0x0100;	// report directory held
			cr4 = firstfile;	// first file id
			break;

		case 0x7300:	// Get File Info
			logerror( "CD: Get File Info (PC=%x)\n",   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			cd_stat |= CD_STAT_TRANS;
			cd_stat &= 0xff00;		// clear top byte of return value
			hirqreg |= (CMOK|DRDY);

			temp = (cr3&0xff)<<16;
			temp |= cr4;

			if (temp == 0xffffff)	// special
			{
			}
			else
			{
				cr2 = 6;	// 6 words for single file
							// first 4 bytes = FAD address
							// second 4 bytes = length
							// last 4 bytes:
							// - unit size
							// - gap size
							// - file #
							// attributes flags

				// start ptr at last 4k from end
				dbufptr = (508*1024);
				// first 4 bytes = FAD
				databuf[(508*1024)] =   (curdir[temp].firstfad>>24)&0xff;
				databuf[(508*1024)+1] = (curdir[temp].firstfad>>16)&0xff;
				databuf[(508*1024)+2] = (curdir[temp].firstfad>>8)&0xff;
				databuf[(508*1024)+3] = (curdir[temp].firstfad&0xff);
		 		// second 4 bytes = length of file
				databuf[(508*1024)+4] = (curdir[temp].length>>24)&0xff;
				databuf[(508*1024)+5] = (curdir[temp].length>>16)&0xff;
				databuf[(508*1024)+6] = (curdir[temp].length>>8)&0xff;
				databuf[(508*1024)+7] = (curdir[temp].length&0xff);

				databuf[(508*1024)+8] = 0x00;
				databuf[(508*1024)+9] = 0x00;
				databuf[(508*1024)+10] = temp;
				databuf[(508*1024)+11] = curdir[temp].flags;
			}
			wordsread = 0;
			break;

		case 0x7400:	// Read File
			logerror( "CD: Read File (PC=%x)\n",   activecpu_get_pc());
			temp = (cr3&0xff)<<16;
			temp |= cr4;

			cd_stat = CD_STAT_PLAY|0x80;	// set "cd-rom" bit
			cr2 = 0x4101;	// CTRL/track
			cr3 = (curdir[temp].firstfad>>16)&0xff;
			cr4 = (curdir[temp].firstfad&0xffff);

			cd_curfad = curdir[temp].firstfad;
			if (curdir[temp].length / 2048)
			{
				fadstoplay = curdir[temp].length/2048;
				fadstoplay++;
			}
			else
			{
				fadstoplay = curdir[temp].length/2048;
			}

			hirqreg |= (CMOK|DRDY);
			oddframe = 0;
			dbufptr = 0;  		// reset buffer pointers
			in_buffer = 0;

			// and do the disc I/O
			cd_playdata();
			break;

		case 0x7500:
			logerror( "CD: Abort File (PC=%x)\n",   activecpu_get_pc());
			// bios expects "2bc" mask to work against this
			hirqreg |= (CMOK|EFLS|EHST|ESEL|DCHG|PEND|BFUL|CSCT|DRDY);
			cd_stat = CD_STAT_PERI|CD_STAT_PAUSE;	// force to pause
			break;

		case 0xe000:	// appears to be copy protection check.  needs only to return OK.
			logerror( "CD: Verify copy protection (PC=%x)\n",   activecpu_get_pc());
			cd_stat = CD_STAT_PAUSE;
			cr2 = 0x4;
			hirqreg |= (CMOK|DRDY);
			break;

		case 0xe100:	// get disc region
			logerror( "CD: Get disc region (PC=%x)\n",   activecpu_get_pc());
			cd_stat = CD_STAT_PAUSE;
			cr2 = 0x4;		// (must return this value to pass bios checks)
			hirqreg |= (CMOK|DRDY);
			break;

		default:
			logerror( "CD: Unknown command %04x (PC=%x)\n", cr1,   activecpu_get_pc());
			cd_stat &= ~CD_STAT_PERI;
			hirqreg |= (CMOK|DRDY);
			break;
		}
//      scu_cdinterrupt();
		break;
	default:
		logerror("CD: WW %08x %04x (PC=%x)\n", addr, data, activecpu_get_pc());
		break;
	}

}

READ32_HANDLER( stvcd_r )
{
	UINT32 rv = 0;

	offset <<= 2;

	switch (offset)
	{
		case 0x90008:
		case 0x9000c:
		case 0x90018:
		case 0x9001c:
		case 0x90020:
		case 0x90024:
			rv = cd_readWord(offset);
			return rv<<16;
			break;

		case 0x98000:
		case 0x18000:
			if (mem_mask == 0)
			{
				rv = cd_readLong(offset);
			}
			else if (mem_mask == 0x0000ffff)
			{
				rv = cd_readWord(offset)<<16;
			}
			else
			{
				fatalerror("CD: Unknown data buffer read @ mask = %08x", mem_mask);
			}

			break;

		default:
			logerror("Unknown CD read @ %x\n", offset);
			break;
	}

	return rv;
}

WRITE32_HANDLER( stvcd_w )
{
	offset <<= 2;

	switch (offset)
	{
		case 0x90008:
		case 0x9000c:
		case 0x90018:
		case 0x9001c:
		case 0x90020:
		case 0x90024:
			cd_writeWord(offset, data>>16);
			break;

		default:
			logerror("Unknown CD write %x @ %x\n", data, offset);
			break;
	}
}

// iso9660 parsing
static void read_new_dir(unsigned long fileno)
{
	int foundpd, i;
	unsigned long cfad, dirfad;
	unsigned char sect[2048];

	if (fileno == 0xffffff)
	{
		cfad = 166;		// first sector of directory as per iso9660 specs

		foundpd = 0;	// search for primary vol. desc
		while ((!foundpd) && (cfad < 200))
		{
			memset(sect, 0, 2048);
			cd_readblock(cfad++, sect);

			if ((sect[1] == 'C') && (sect[2] == 'D') && (sect[3] == '0') && (sect[4] == '0') && (sect[5] == '1'))
			{
			 	switch (sect[0])
				{
					case 0:	// boot record
						break;

					case 1: // primary vol. desc
						foundpd = 1;
						break;

					case 2: // secondary vol desc
						break;

					case 3: // vol. section descriptor
						break;

					case 0xff:
						cfad = 200;
						break;
				}
			}
		}

		// got primary vol. desc.
		if (foundpd)
		{
			dirfad = sect[140] | (sect[141]<<8) | (sect[142]<<16) | (sect[143]<<24);
			dirfad += 150;

			// parse root entry
			curroot.firstfad = sect[158] | (sect[159]<<8) | (sect[160]<<16) | (sect[161]<<24);
			curroot.firstfad += 150;
			curroot.length = sect[166] | (sect[167]<<8) | (sect[168]<<16) | (sect[169]<<24);
			curroot.flags = sect[181];
			for (i = 0; i < sect[188]; i++)
			{
				curroot.name[i] = sect[189+i];
			}
			curroot.name[i] = '\0';	// terminate

			// easy to fix, but make sure we *need* to first
			if (curroot.length > 14336)
			{
				fatalerror("ERROR: root directory too big (%ld)", curroot.length);
			}

			// done with all that, read the root directory now
			make_dir_current(curroot.firstfad);
		}
	}
	else
	{
		if (curdir[fileno].length > 14336)
		{
			fatalerror("ERROR: new directory too big (%ld)!", curdir[fileno].length);
		}
		make_dir_current(curdir[fileno].firstfad);
	}
}

// makes the directory pointed to by FAD current
static void make_dir_current(unsigned long fad)
{
	int i;
	unsigned long nextent, numentries;
	unsigned char sect[14336];
	direntryT *curentry;

	memset(sect, 0, 14336);
	for (i = 0; i < (curroot.length/2048); i++)
	{
		cd_readblock(fad+i, &sect[2048*i]);
	}

	nextent = 0;
	numentries = 0;
	while (nextent < 14336)
	{
		if (sect[nextent])
		{
			nextent += sect[nextent];
			numentries++;
		}
		else
		{
			nextent = 14336;
		}
	}

	if (curdir != (direntryT *)NULL)
	{
		free((void *)curdir);
	}

	curdir = (direntryT *)malloc(sizeof(direntryT)*numentries);
	curentry = curdir;
	numfiles = numentries;

	nextent = 0;
	while (numentries)
	{
		curentry->firstfad = sect[nextent+2] | (sect[nextent+3]<<8) | (sect[nextent+4]<<16) | (sect[nextent+5]<<24);
		curentry->firstfad += 150;
		curentry->length = sect[nextent+10] | (sect[nextent+11]<<8) | (sect[nextent+12]<<16) | (sect[nextent+13]<<24);
		curentry->flags = sect[nextent+25];
		for (i = 0; i < sect[nextent+32]; i++)
		{
			curentry->name[i] = sect[nextent+33+i];
		}
		curentry->name[i] = '\0';	// terminate

		nextent += sect[nextent];
		curentry++;
		numentries--;
	}

	for (i = 0; i < numfiles; i++)
	{
		if (!(curdir[i].flags & 0x02))
		{
			firstfile = i;
			i = numfiles;
		}
	}
}

static void cd_readTOC(void)
{
	int i, ntrks, toclen, tocptr;

	if (cdrom)
	{
		ntrks = cdrom_get_last_track(cdrom);
	}
	else
	{
		ntrks = 0;
	}

	toclen = (4 * ntrks);	// toclen header entry

	// data format for Saturn TOC:
	// no header.
	// 4 bytes per track
	// top nibble of first byte is CTRL info
	// low nibble is ADR
	// next 3 bytes are FAD address (LBA + 150)
	// there are always 99 track entries (0-98)
	// unused tracks are ffffffff.
	// entries 99-101 are metadata

	tocptr = 0;	// starting point of toc entries

	for (i = 0; i < ntrks; i++)
	{
		int fad;

		if (cdrom)
		{
			databuf[tocptr] = cdrom_get_adr_control(cdrom, i)<<4 | 0x01;
		}

		if (cdrom)
		{
			fad = cdrom_get_track_start(cdrom, i) + 150;
		}
		else
		{
			fad = 150;
		}

		logerror("[%02d]: mode %02x, FAD %d\n", i+1, databuf[tocptr], fad);

		databuf[tocptr+1] = (fad>>16)&0xff;
		databuf[tocptr+2] = (fad>>8)&0xff;
		databuf[tocptr+3] = fad&0xff;

		tocptr += 4;
	}

	// fill in the rest
	for ( ; i < 99; i++)
	{
		databuf[tocptr] = 0xff;
		databuf[tocptr+1] = 0xff;
		databuf[tocptr+2] = 0xff;
		databuf[tocptr+3] = 0xff;

		tocptr += 4;
	}

	// tracks 99-101 are special metadata
	// $$$FIXME: what to do with the address info for these?
	tocptr = 99 * 4;
	databuf[tocptr] = databuf[0];	// get ctrl/adr from first track
	databuf[tocptr+1] = 1;	// first track's track #

	databuf[tocptr+4] = databuf[ntrks*4];	// ditto for last track
	databuf[tocptr+5] = ntrks;	// last track's track #

	// always assume 1 session, give it track 0's starting address
	databuf[tocptr+8] = databuf[0];
	databuf[tocptr+9] = databuf[1];
	databuf[tocptr+10] = databuf[2];
	databuf[tocptr+11] = databuf[3];

	dbuflen = 102*4;	// there are always 102*4 entries
}

// loads in data set up by a CD-block PLAY command
static void cd_playdata(void)
{
	if (1) //(cd_stat & 0x0f00) == CD_STAT_PLAY)
	{
		while (fadstoplay)
		{
//          logerror(  "cd_playdata: Reading FAD %d", cd_curfad);

			if (cdrom)
			{
				cdrom_read_data(cdrom, cd_curfad-150, &databuf[in_buffer], CD_TRACK_MODE1);
			}
			in_buffer += 2048;
			cd_curfad ++;

			// if read past end of buffer, go into pause
			if (in_buffer > (512*1024))
			{
				cd_stat &= ~CD_STAT_PLAY;
				cd_stat |= CD_STAT_PAUSE;
			}

			fadstoplay--;
		}
	}
}

// loads a single sector off the CD, anywhere from FAD 150 on up
static void cd_readblock(unsigned long fad, unsigned char *dat)
{
	if (cdrom)
	{
		cdrom_read_data(cdrom, fad-150, dat, CD_TRACK_MODE1);
	}
}


