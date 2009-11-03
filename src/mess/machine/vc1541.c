/***************************************************************************

       commodore vc1541 floppy disk drive

***************************************************************************/

/*
 vc1540
  higher serial bus timing than the vc1541, to fast for c64
 vc1541
  (also build in in sx64)
  used with commodore vc20, c64, c16, c128
  floppy disk drive 5 1/4 inch, single sided, double density,
   40 tracks (tone head to big to use 80 tracks?)
  gcr encoding

  computer M6502, 2x VIA6522, 2 KByte RAM, 16 KByte ROM
  1 Commodore serial bus (2 connectors)

 vc1541 ieee488 hardware modification
  additional ieee488 connection, modified rom

 vc1541 II
  ?

 dolphin drives
  vc1541 clone?
  additional 8 KByte RAM at 0x8000 (complete track buffer ?)
  24 KByte rom

 c1551
  used with commodore c16
  m6510t? processor (4 MHz???),
  (m6510t internal 4 MHz clock?, 8 port pins?)
  VIA6522 ?, CIA6526 ?,
  2 KByte RAM, 16 KByte ROM
  connector commodore C16 expansion cartridge
  parallel protocoll

 1750
  single sided 1751
 1751
  (also build in in c128d series)
  used with c128 (vic20,c16,c64 possible?)
  double sided
  can read some other disk formats (mfm encoded?)
  modified commodore serial bus (with quick serial transmission)
  (m6510?, cia6526?)
0000-00FF      Zero page work area, job queue, variables
0100-01FF      GCR overflow area and stack (1571 mode BAM side 1)
0200-02FF      Command buffer, parser, tables, variables
0300-07FF      5 data buffers, 0-4 one of which is used for BAM
1800-1BFF      65C22A, serial, controller ports
1C00-1FFF      65C22A, controller ports
8000-FFE5      32K byte ROM, DOS and controller routines
FFE6-FFFF      JMP table, user command vectors

 1581
  3 1/2 inch, double sided, double density, 80 tracks
  used with c128 (vic20,c16,c64 possible?)
  only mfm encoding?
0000-00FF       Zero page work area, job queue, variables
0100-01FF       Stack, variables, vectors
0200-02FF       Command buffer, tables, variables
0300-09FF       Data buffers (0-6)
0A00-0AFF       BAM for tracks 0-39
0B00-0BFF       BAM for tracks 40-79
0C00-1FFF       Track cache buffer
4000-5FFF       8520A CIA
6000-7FFF       WD177X FDC
8000-FEFF       32K byte ROM, DOS and controller routines
FF00-FFFF       Jump table, vectors

 2031/4031
 ieee488 interface
 1541 with ieee488 bus instead of serial bus
  $1800 via6522 used for ieee488 interface?
  maybe like in pet series?
   port a data in/out?
   port b ce port, ddr 31

 2040/3040
 ieee488 interface
 2 drives

 2041
 ieee488 interface

 4040
 ieee488 interface
 2 drives

 sfd1001
 ieee488 interface
 5 1/4 inch high density
 2 heads

 */
/* 2008-09 FP
Started some work on the floppy emulation code:
    + made the functions / structs less vc1541-centric & d64-centric
    + unified config & reset functions, using type variable
        to choose the actions to be done
    + moved GCR encoding at loading time (wip untested code)

A limitation is clear: only a single occurrence of emulated drive
is expected.
At a later stage, at least drive_config & drive_reset will need
an index to choose between the drives
In the end, we could also split the floppy drives emulation from the
disk image handling...

Current status:
    + preliminary vc1541 & c1551 implementation (quite slow, especially
        if you turn on logging)
    + no support for other drive types (even if now I list all
        the variants supported by Vice in the enum)
    + communications between the drive CPU and the computer CPU have
        to be checked, debugged and improved: somehow it works, but no
        sync bit is set (encoding bug? timers-related bug?) and the drive
        keeps reading beyond the end of the track (synchronization
        problems?)
    + all but .g64 images should be correctly encoded at loading time
        (I actually have doubts about some .d82 details)
    + error maps are not still used, even if their presence is
        acknowledged
    + floppy drives should become devices and use ROMREGION_LOADBYNAME
        for their ROMs (not the current defines)
    + all available drive dumps are included for each systems using it
        (vc1541 = sx64, vip64, c16v, plus4v; vc1571 = c128d; vc1571cr = c128dcr;
        vc1581 = c128d81; vc1540 = vc20v, vic20plv; c2031 = vc20i (only dumps added);
        c1551 = c16c, plus4c). this is only a temporary workaround to document
        available dumps. correct revisions will be used with each systems when
        emulation works (or 'default' bios will be used for most common
        configurations, when machines were compatible with more than a single
        drive model)
*/
/*
Informations on the fileformats (which the code is based on)

D64 - http://ist.uwaterloo.ca/~schepers/formats/D64.TXT
D71 - http://ist.uwaterloo.ca/~schepers/formats/D71.TXT
D81 - http://ist.uwaterloo.ca/~schepers/formats/D81.TXT
D80 & D82 - http://ist.uwaterloo.ca/~schepers/formats/D80-D82.TXT
G64 & GCR - http://ist.uwaterloo.ca/~schepers/formats/G64.TXT
GCR - http://www.baltissen.org/newhtm/1541c.htm

Other info sources:
    http://sta.c64.org/cbm1541mem.html

*/


#include "driver.h"
#include "image.h"
#include "cpu/m6502/m6502.h"
#include "machine/6525tpi.h"
#include "machine/6522via.h"
#include "includes/cbmserb.h"

#include "includes/vc1541.h"


#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(MACHINE)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

#define XTAL_16_MHz	16000000


/*
 * only for testing at the moment
 */

#define WRITEPROTECTED 0


/*******************************************

    Structures

*******************************************/

/* Legacy code. At a later stage, we should convert everything to devices.
However, fixing the C64 / Floppy drive communications is a priority over
cleaning up of this part */


/* 0 summarized, 1 computer, 2 vc1541 */
static struct
{
	int atn[3], data[3], clock[3];
}
serial;

typedef struct
{
	int sync;				// active after > 9 '1's
	int ones_count;
	int ready;				// needed for 1551
	UINT8 *data;			// entire d64 image encoded at loading time
} GCR_data;

typedef struct
{
	const char *cputag;
	int type;

	int led, motor, frequency;

	double track;
	int clock;
	int start_pos, pos;		// start_pos = start of half track ; pos = position from the beginning of the current half-track!

	void *timer;

	GCR_data gcr;

	/* vc1541 specific */
	struct {
		int deviceid;
		int serial_atn, serial_clock, serial_data;
		int acka, data;
	} vc1541_serial;

	int via0irq, via1irq;

} _vc1541_drive_emu_t;

typedef struct
{
	const char *cputag;
	int type;

	int led, motor, frequency;

	double track;
	int clock;
	int start_pos, pos;		// start_pos = start of half track ; pos = position from the beginning of the current half-track!

	void *timer;

	GCR_data gcr;

	/* c1551 specific */
	UINT8 cpu_ddr, cpu_port;
	void *irq_timer;

} _c1551_drive_emu_t;

static _vc1541_drive_emu_t vc1541_drive;
static _c1551_drive_emu_t c1551_drive;


/*******************************************

    Format specific code

*******************************************/


/* Are there other formats to support which could require more tracks? */
#define IMAGE_MAX_TRACKS 154

#define D64_MAX_TRACKS 35
#define D64_40T_MAX_TRACKS 40

static const int d64_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	17, 17, 17, 17, 17,		/* only for 40 tracks d64 */
	17, 17				/* in principle the drive could read 42 tracks */
};

#define D71_MAX_TRACKS 70

static const int d71_sectors_per_track[] =
{
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	19, 19, 19, 19, 19, 19, 19,
	18, 18, 18, 18, 18, 18,
	17, 17, 17, 17, 17
};

#define D81_MAX_TRACKS 80

/* Each track in a d81 file has 40 sectors, so no need of a sectors_per_track table */

#define D80_MAX_TRACKS 77

static const int d80_sectors_per_track[] =
{
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23
};


/* Still have to investigate if these are read as 154 or as 77 in the drive... */
/* For now we assume it's 154, but I'm not sure */
#define D82_MAX_TRACKS 154

static const int d82_sectors_per_track[] =
{
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23
};


static const int max_tracks_per_format[] =
{
	D64_MAX_TRACKS,
	D64_MAX_TRACKS,
	D64_40T_MAX_TRACKS,
	D64_40T_MAX_TRACKS,
	D64_MAX_TRACKS,
	D71_MAX_TRACKS,
	D71_MAX_TRACKS,
	D81_MAX_TRACKS,
	D80_MAX_TRACKS,
	D82_MAX_TRACKS,
	0		/* Still to add support for .g64 */
};

static int image_offset[IMAGE_MAX_TRACKS];
static int gcr_offset[IMAGE_MAX_TRACKS];

/*******************************************

    Offset initialization

*******************************************/

/* This is twice useful: with length = 256, it allows to determine the starting offset of each
track in the .d64 image; with length = 368, it allows to determine the starting offset of each
track in the GCR encoded image  */
static void offset_init( int format, int *offset_list, int length )
{
	int i;

	/* Track # starts from 1, hence Track k will start at image_offset[k-1] */
	offset_list[0] = 0;

	switch (format)
	{
		case format_d64:
		case format_d64_err:
		case format_d67:
			for (i = 1; i < D64_MAX_TRACKS; i++)
				offset_list[i] = offset_list[i - 1] + d64_sectors_per_track[i - 1] * length;
			break;

		case format_d64_40t:
		case format_d64_40t_err:
			for (i = 1; i < D64_40T_MAX_TRACKS; i++)
				offset_list[i] = offset_list[i - 1] + d64_sectors_per_track[i - 1] * length;
			break;

		case format_d71:
		case format_d71_err:
			for (i = 1; i < D71_MAX_TRACKS; i++)
				offset_list[i] = offset_list[i - 1] + d71_sectors_per_track[i - 1] * length;
			break;

		case format_d81:
			for (i = 1; i < D81_MAX_TRACKS; i++)
				offset_list[i] = offset_list[i - 1] + 40 * length;
			break;

		case format_d80:
			for (i = 1; i < D80_MAX_TRACKS; i++)
				offset_list[i] = offset_list[i - 1] + d80_sectors_per_track[i - 1] * length;
			break;

		case format_d82:
			for (i = 1; i < D82_MAX_TRACKS; i++)
				offset_list[i] = offset_list[i - 1] + d82_sectors_per_track[i - 1] * length;
			break;

		default:
			/* Still have to work on g64 images */
			logerror("Unsupported format\n");
			break;
	}
}


/*******************************************

    Generic image handling

*******************************************/


/* This calculates starting offset (from the beginning of the image file) of a sector */
/* Since sectors are always 256 bytes large, this works no matter the file format! */
static int image_tracksector2offset( int track, int sector )
{
	return image_offset[track - 1] + sector * 256;
}

/*
// WARNING: this would work with a d64 image... it has to be adapted to work with GCR image we store at loading time
static void image_offset2tracksector( int offset )
{
    int track = 0, sector = 0, helper, i = 0;

    helper = offset;

    do
    {
    helper -= (d64_sectors_per_track[++i] * 256);
    } while (helper > 0);

    track = i;

    sector = (offset - image_offset[track]) / 256;

    logerror("We are reading at offset %d, which is in track %d and sector %d \n", offset, track, sector);
}
*/


/* These functions return the Disk ID stored in the disk BAM */
/* It is not necessarily the one some copy protection checks, and
   indeed, in these cases a g64 image is needed (which stores the
   Disk ID as coded in the GCR data) */

static int disk_id1( int format )
{
	int id1 = 0;

	switch (format)
	{
		case format_d64:
		case format_d64_err:
		case format_d64_40t:
		case format_d64_40t_err:
		case format_d67:
		case format_d71:
		case format_d71_err:
			id1 = image_tracksector2offset(18, 0) + 0xa2;
			break;

		case format_d81:
			id1 = image_tracksector2offset(40, 0) + 0x16;
			break;

		case format_d80:
		case format_d82:
			id1 = image_tracksector2offset(39, 0) + 0x18;
			break;

		default:
			/* Still have to work on g64 images */
			logerror("Unsupported format\n");
			break;
	}

	return id1;
}


static int disk_id2( int format )
{
	int id2 = 0;

	switch (format)
	{
		case format_d64:
		case format_d64_err:
		case format_d64_40t:
		case format_d64_40t_err:
		case format_d67:
		case format_d71:
		case format_d71_err:
			id2 = image_tracksector2offset(18, 0) + 0xa3;
			break;

		case format_d81:
			id2 = image_tracksector2offset(40, 0) + 0x17;
			break;

		case format_d80:
		case format_d82:
			id2 = image_tracksector2offset(39, 0) + 0x19;
			break;

		default:
			/* Still have to work on g64 images */
			logerror("Unsupported format\n");
			break;
	}

	return id2;
}


/* Four different frequencies for the 4 different zones on the disk */
/* These work for 1541 drives, are the same for other drives? */
static const double drive_times[4]=
{
	XTAL_16_MHz / 13, XTAL_16_MHz / 14, XTAL_16_MHz / 15, XTAL_16_MHz / 16
};


/* Commodore GCR format */
/*
Original    Encoded
4 bits      5 bits

0000    ->  01010 = 0x0a
0001    ->  01011 = 0x0b
0010    ->  10010 = 0x12
0011    ->  10011 = 0x13
0100    ->  01110 = 0x0e
0101    ->  01111 = 0x0f
0110    ->  10110 = 0x16
0111    ->  10111 = 0x17
1000    ->  01001 = 0x09
1001    ->  11001 = 0x19
1010    ->  11010 = 0x1a
1011    ->  11011 = 0x1b
1100    ->  01101 = 0x0d
1101    ->  11101 = 0x1d
1110    ->  11110 = 0x1e
1111    ->  10101 = 0x15

We use the encoded values in bytes because we use them to encode
groups of 4 bytes into groups of 5 bytes, below.
*/

static const int bin_2_gcr[] =
{
	0x0a, 0x0b, 0x12, 0x13, 0x0e, 0x0f, 0x16, 0x17,
	0x09, 0x19, 0x1a, 0x1b, 0x0d, 0x1d, 0x1e, 0x15
};


/* This could be of use if we ever implement saving in .d64 format, to convert back GCR -> d64 */
/*
static const int gcr_2_bin[] =
{
    -1, -1,   -1,   -1,
    -1, -1,   -1,   -1,
    -1, 0x08, 0x00, 0x01,
    -1, 0x0c, 0x04, 0x05,
    -1, -1,   0x02, 0x03,
    -1, 0x0f, 0x06, 0x07,
    -1, 0x09, 0x0a, 0x0b,
    -1, 0x0d, 0x0e, -1
};
*/


/* gcr_double_2_gcr takes 4 bytes (a, b, c, d) and shuffles their nibbles to obtain 5 bytes in dest */
/* The result is basically res = (enc(a) << 15) | (enc(b) << 10) | (enc(c) << 5) | enc(d)
 * with res being 5 bytes long and enc(x) being the GCR encode of x.
 * In fact, we store the result as five separate bytes in the dest argument
 */
static void gcr_double_2_gcr( UINT8 a, UINT8 b, UINT8 c, UINT8 d, UINT8 *dest )
{
	UINT8 gcr[8];

	/* Encode each nibble to 5 bits */
	gcr[0] = bin_2_gcr[a >> 4];
	gcr[1] = bin_2_gcr[a & 0x0f];
	gcr[2] = bin_2_gcr[b >> 4];
	gcr[3] = bin_2_gcr[b & 0x0f];
	gcr[4] = bin_2_gcr[c >> 4];
	gcr[5] = bin_2_gcr[c & 0x0f];
	gcr[6] = bin_2_gcr[d >> 4];
	gcr[7] = bin_2_gcr[d & 0x0f];

	/* Re-order the encoded data to only keep the 5 lower bits of each byte */
	dest[0] = (gcr[0] << 3) | (gcr[1] >> 2);
	dest[1] = (gcr[1] << 6) | (gcr[2] << 1) | (gcr[3] >> 4);
	dest[2] = (gcr[3] << 4) | (gcr[4] >> 1);
	dest[3] = (gcr[4] << 7) | (gcr[5] << 2) | (gcr[6] >> 3);
	dest[4] = (gcr[6] << 5) | gcr[7];
}

/* FIXME: temp workaround */
static void filling_byte( UINT8 a, UINT8 *dest )
{
	dest[0] = a;
}


/**************************************

    Drive timers (aka floppy reading)

**************************************/

static TIMER_CALLBACK( vc1541_drive_timer )
{
	const device_config *via_1 = devtag_get_device(machine, "_1541_via_1");

	if (vc1541_drive.clock == 0)
	{
		vc1541_drive.gcr.ready = 0;
		// why put sync back to 0 here?!? it would break the sync count, isn't it?
		// or is it the way the serial port reads the sync count?!?
		vc1541_drive.gcr.sync = 0;

		cputag_set_input_line(machine, vc1541_drive.cputag, M6502_SET_OVERFLOW, 1);
		via_ca1_w(via_1, 0, 1);

		vc1541_drive.clock = 1;
		return;
	}

	// this code has 2 effects: move on the pos on the data and reset it to 0 at the end of the track!
	// size of the track is: (# sectors in the track) * (size of sector GCR endoded)
	// Here, size of sector is 368 because of our choices in the loading: 9 bytes of header gap & 14 bytes of sector gap
	// We will need to set the size at loading time when adding support for G64 images
	if (++(vc1541_drive.pos) >= (d64_sectors_per_track[(int)vc1541_drive.track - 1] * 368))
		vc1541_drive.pos = 0;
	// should we check if there is a change of track? to reset the pos in that case as well?

	vc1541_drive.gcr.ready = 1;

	// notice that if there is a sequence of more than 40 1s it correctly does not reset the sync
	// counter until a 0 is found
	if (vc1541_drive.gcr.data[vc1541_drive.pos] == 0xff)
	{
		vc1541_drive.gcr.ones_count++;

//      if (vc1541_drive.gcr.ones_count == 2) // 9 ones should already mark it as a sync, even if CBM chose to use 40 of them
		if (vc1541_drive.gcr.ones_count == 5)
			vc1541_drive.gcr.sync = 1;
	}
	else
	{
		vc1541_drive.gcr.ones_count = 0;
		vc1541_drive.gcr.sync = 0;
	}

	cputag_set_input_line(machine, vc1541_drive.cputag, M6502_SET_OVERFLOW, 0);
	via_ca1_w(via_1, 0, 0);

	vc1541_drive.clock = 0;
}

static TIMER_CALLBACK( c1551_drive_timer )
{
	if (c1551_drive.clock == 0)
	{
		c1551_drive.gcr.ready = 0;
		// why put sync back to 0 here?!? it would break the sync count, isn't it?
		// or is it the way the serial port reads the sync count?!?
		c1551_drive.gcr.sync = 0;

		c1551_drive.clock = 1;
		return;
	}

	// this code has 2 effects: move on the pos on the data and reset it to 0 at the end of the track!
	// size of the track is: (# sectors in the track) * (size of sector GCR endoded)
	// Here, size of sector is 368 because of our choices in the loading: 9 bytes of header gap & 14 bytes of sector gap
	// We will need to set the size at loading time when adding support for G64 images
	if (++(c1551_drive.pos) >= (d64_sectors_per_track[(int)c1551_drive.track - 1] * 368))
		c1551_drive.pos = 0;
	// should we check if there is a change of track? to reset the pos in that case as well?

	c1551_drive.gcr.ready = 1;

	// notice that if there is a sequence of more than 40 1s it correctly does not reset the sync
	// counter until a 0 is found
	if (c1551_drive.gcr.data[c1551_drive.pos] == 0xff)
	{
		c1551_drive.gcr.ones_count++;

//      if (c1551_drive.gcr.ones_count == 2) // 9 ones should already mark it as a sync, even if CBM chose to use 40 of them
		if (c1551_drive.gcr.ones_count == 5)
			c1551_drive.gcr.sync = 1;
	}
	else
	{
		c1551_drive.gcr.ones_count = 0;
		c1551_drive.gcr.sync = 0;
	}

	c1551_drive.clock = 0;
}


/**************************************

    VC1541 Serial Bus Device Interface

**************************************/

/* 2009-05 FP - Note: I could use directly these functions as DEVICE_HANDLERs for
the serial bus device, but these are also used in the VIA interface above, so I
prefer to keep the handlers separated at this stage. Further work on the serial
bus emulation should allow to remove the duplicated code. */

static void vc1541_serial_reset_write( running_machine *machine, int which, int level )
{
}

static UINT8 vc1541_serial_atn_read( running_machine *machine, int which )
{
/* there is no more cpu_getactivecpu! */
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
	return serial.atn[0];
}

static void vc1541_serial_atn_write( running_machine *machine, int which, int level )
{
	const device_config *via_0 = devtag_get_device(machine, "_1541_via_0");
#if 0
	int value;
#endif
	if (serial.atn[1 + which] != level)
	{
/* there is no more cpu_getactivecpu! */
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
		serial.atn[1 + which] = level;
		if (serial.atn[0] != level)
		{
			serial.atn[0] = serial.atn[1] && serial.atn[2];
			if (serial.atn[0] == level)
			{
			#if 0
				DBG_LOG(machine, 1, "vc1541",("%d:%.4x atn %s\n",
									 cpu_getactivecpu (),
									 activecpu_get_pc(),
									 serial.atn[0]?"ATN":"atn"));
			#endif
				via_ca1_w (via_0, 0, !level);
#if 0
				value = vc1541_drive.vc1541_serial.data;
				if (vc1541_drive.vc1541_serial.acka != !level)
					value = 0;
				if (value != serial.data[2])
				{
					serial.data[2] = value;
					if (serial.data[0] != value)
					{
						serial.data[0] = serial.data[1] && serial.data[2];
					}
				}
#endif
			}
		}
	}
}

static UINT8 vc1541_serial_data_read( running_machine *machine, int which )
{
/* there is no more cpu_getactivecpu! */
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
	return serial.data[0];
}

static void vc1541_serial_data_write( running_machine *machine, int which, int level )
{
	if (serial.data[1 + which] != level)
	{
/* there is no more cpu_getactivecpu! */
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
		serial.data[1 + which] = level;
		if (serial.data[0] != level)
		{
			serial.data[0] = serial.data[1] && serial.data[2];
			#if 0
			if (serial.data[0] == level)
			{
				DBG_LOG(machine, 1, "vc1541",("%d:%.4x data %s\n",
									 cpu_getactivecpu (),
									 activecpu_get_pc(),
									 serial.data[0]?"DATA":"data"));
			}
			#endif
		}
	}
}

static UINT8 vc1541_serial_clock_read( running_machine *machine, int which )
{
/* there is no more cpu_getactivecpu! */
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
	return serial.clock[0];
}

static void vc1541_serial_clock_write( running_machine *machine, int which, int level )
{
	if (serial.clock[1 + which] != level)
	{
/* there is no more cpu_getactivecpu! */
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
		serial.clock[1 + which] = level;
		if (serial.clock[0] != level)
		{
			serial.clock[0] = serial.clock[1] && serial.clock[2];
			if (serial.clock[0] == level)
			{
/*              DBG_LOG(machine, 1, "vc1541",("%d:%.4x clock %s\n",
                                     cpu_getactivecpu (),
                                     activecpu_get_pc(),
                                     serial.clock[0]?"CLOCK":"clock"));*/
			}
		}
	}
}

static UINT8 vc1541_serial_request_read( running_machine *machine, int which )
{
	return 1;
}

static void vc1541_serial_request_write( running_machine *machine, int which, int level )
{
}


/* 2009-05 FP - TODO: better implementation of the DEVICE_HANDLERs below. Right now they are
redundant (see also my Note above)  */
static READ8_DEVICE_HANDLER( emu_drive_atn_read )
{
	return vc1541_serial_atn_read(device->machine, 0);
}

static READ8_DEVICE_HANDLER( emu_drive_clock_read )
{
	return vc1541_serial_clock_read(device->machine, 0);
}

static READ8_DEVICE_HANDLER( emu_drive_data_read )
{
	return vc1541_serial_data_read(device->machine, 0);
}

static READ8_DEVICE_HANDLER( emu_drive_request_read )
{
	return vc1541_serial_request_read(device->machine, 0);
}

static WRITE8_DEVICE_HANDLER( emu_drive_atn_write )
{
	vc1541_serial_atn_write(device->machine, 0, data);
}

static WRITE8_DEVICE_HANDLER( emu_drive_clock_write )
{
	vc1541_serial_clock_write(device->machine, 0, data);
}

static WRITE8_DEVICE_HANDLER( emu_drive_data_write )
{
	vc1541_serial_data_write(device->machine, 0, data);
}

static WRITE8_DEVICE_HANDLER( emu_drive_request_write )
{
	vc1541_serial_request_write(device->machine, 0, data);
}

static WRITE8_DEVICE_HANDLER( emu_drive_reset_write )
{
	vc1541_serial_reset_write(device->machine, 0, data);
}


static const cbm_serial_bus_interface cbm_emu_drive_interface =
{
	DEVCB_HANDLER(emu_drive_atn_read),
	DEVCB_HANDLER(emu_drive_clock_read),
	DEVCB_HANDLER(emu_drive_data_read),
	DEVCB_HANDLER(emu_drive_request_read),
	DEVCB_HANDLER(emu_drive_atn_write),
	DEVCB_HANDLER(emu_drive_clock_write),
	DEVCB_HANDLER(emu_drive_data_write),
	DEVCB_HANDLER(emu_drive_request_write),

	DEVCB_HANDLER(emu_drive_reset_write)
};



/**************************************************************************
 *  VIA 6522 at 0x1800
 *
 *  0x1800  port B
 *      bit 0 : data in (inverted: 0 = Low, 1 = High)
 *      bit 1 : data out (inverted: 0 = Low, 1 = High)
 *      bit 2 : clock in (inverted: 0 = Low, 1 = High)
 *      bit 3 : clock out (inverted: 0 = Low, 1 = High)
 *      bit 4 : atna out (inverted: 0 = Low, 1 = High)
 *      bit 5 : device id 1
 *      bit 6 : device id 2
 *              device ID number is set by a jumper:
 *              00 = 8 (default), 01 = 9, 10 = 10, 11 = 11
 *      bit 7 : atn in (inverted: 0 = Low, 1 = High)
 *  0x1801  port A - Read to acknowledge interrupt generated by ATN IN going high.
 *  0x1802  port B data direction register.
 *          if a bit here is 0, then the corresponding bit of portB can only be read
 *          if a bit here is 1, then the corresponding bit of portB can be read and written
 *          default value 0x1a
 *  0x1803  port A data direction register.
 *          default value 0xff
 *  0x1804  timer 1 counter (low)
 *  0x1805  timer 1 counter (high) / Timer one counter
 *          Read low byte or write high byte to start timer or restart timer upon underflow.
 *  0x1806  timer 1 latch (low)
 *  0x1807  timer 1 latch (high)
 *          read/write starting value of timer from/to here.
 *  0x1808  timer 2 counter (low)
 *  0x1809  timer 2 counter (high)
 *  0x180a  shift register
 *  0x180b  auxiliary control register. bit 6: 0 = Stop timer; 1 = Start timer.
 *  0x180c  peripheral control register
 *  0x180d  interrupt flag register. bit 1: 1 = ATN IN went high, bit 6: 1 = Timer underflow occurred.
 *  0x180e  interrupt enable register.
 *          bit 1: write -  1 = Enable interrupts generated by ATN IN going high
 *          bit 7: read - interrupt has been generated.
 *          bit 7: write - Fill bit: bits 0-6, that are set to 1, get their values from this bit; bits 0-6, that are set to 0, are left unchanged
 *
 * also ca1 (books says cb2)
 * irq to m6502 irq connected (or with second via irq)
 *
 **************************************************************************/

static void vc1541_via0_irq( const device_config *device, int level )
{
	vc1541_drive.via0irq = level;
	DBG_LOG(device->machine, 2, "vc1541 via0 irq", ("level %d %d\n", vc1541_drive.via0irq, vc1541_drive.via1irq));

	cputag_set_input_line(device->machine, vc1541_drive.cputag, M6502_IRQ_LINE, vc1541_drive.via1irq || vc1541_drive.via0irq);
}

static READ8_DEVICE_HANDLER( vc1541_via0_read_portb )
{
	int value = 0x7a;

	if (!vc1541_drive.vc1541_serial.serial_data || !serial.data[0])
		value |= 0x01;

	if (!vc1541_drive.vc1541_serial.serial_clock || !serial.clock[0])
		value |= 0x04;

	if (!serial.atn[0])
		value |= 0x80;

	switch (vc1541_drive.vc1541_serial.deviceid)
	{
		case 8:
			value &= ~0x60;
			break;
		case 9:
			value &= ~0x40;
			break;
		case 10:
			value &= ~0x20;
			break;
		case 11:
			break;
	}

	return value;
}

static WRITE8_DEVICE_HANDLER( vc1541_via0_write_portb )
{
	DBG_LOG(device->machine, 2, "vc1541 serial write", ("%s %s %s\n",
									 data & 0x10 ? "ATN"   : "atn",
									 data & 0x08 ? "CLOCK" : "clock",
									 data & 0x02 ? "DATA"  : "data"));

	vc1541_drive.vc1541_serial.data = data & 0x02 ? 0 : 1;
	vc1541_drive.vc1541_serial.acka = data & 0x10 ? 1 : 0;

	if ((!(data & 0x02)) != vc1541_drive.vc1541_serial.serial_data)
	{
		vc1541_serial_data_write(device->machine, 1, vc1541_drive.vc1541_serial.serial_data = !(data & 0x02));
	}

	if ((!(data & 0x08)) != vc1541_drive.vc1541_serial.serial_clock)
	{
		vc1541_serial_clock_write(device->machine, 1, vc1541_drive.vc1541_serial.serial_clock = !(data & 0x08));
	}

	vc1541_serial_atn_write(device->machine, 1, vc1541_drive.vc1541_serial.serial_atn = 1);
}

static const via6522_interface vc1541_via0 =
{
	DEVCB_NULL,								   /*via0_read_porta, */
	DEVCB_HANDLER(vc1541_via0_read_portb),
	DEVCB_NULL,								   /*via0_read_ca1, */
	DEVCB_NULL,								   /*via0_read_cb1, */
	DEVCB_NULL,								   /*via0_read_ca2, */
	DEVCB_NULL,								   /*via0_read_cb2, */
	DEVCB_NULL,								   /*via0_write_porta, */
	DEVCB_HANDLER(vc1541_via0_write_portb),
	DEVCB_NULL,                                /*via0_write_ca1, */
	DEVCB_NULL,                                /*via0_write_cb1, */
	DEVCB_NULL,								   /*via0_write_ca2, */
	DEVCB_NULL,								   /*via0_write_cb2, */
	DEVCB_LINE(vc1541_via0_irq)
};


/**************************************************************************
 *  VIA 6522 at 0x1c00
 *
 *  0x1c00  port B - Disk controller i/o control line
 *      bit 0 : step head motor output 1
 *      bit 1 : step head motor output 2
 *      bit 2 : motor on output (0 = Off, 1 = On) - 300 revolutions per minute
 *      bit 3 : LED control output (0 = Off, 1 = On)
 *      bit 4 : write protect sense (0 = protected, 1 = unprotected)
 *      bit 5 : density select 0
 *      bit 6 : density select 1
 *          data density (track dependend): 00 = lowest (faster time), 11 = highest (slower timer)
 *      bit 7 : sync signal (more then nine 1 bits) detect
 *  0x1c01  Port A - byte in gcr format from or to floppy
 *  0x1c02  Port B data direction register + LED output - default 0x6f
 *  0x1c03  Port A data direction register - 0x00 read from disk, 0xff write to disk
 *  0x1c04  Timer 1 counter (low)
 *  0x1c05  Timer 1 counter (high)
 *          Read low byte or write high byte to start timer or restart timer upon underflow.
 *  0x1c06  Timer 1 latch (low)
 *  0x1c07  Timer 1 latch (high)
 *          read/write starting value of timer from/to here.
 *  0x1c08  Timer 2 latch (low)
 *  0x1c09  Timer 2 latch (high)
 *  0x1c0a  Shift register
 *  0x1c0b  Auxiliary control register. bit 6: 0 = Stop timer; 1 = Start timer.
 *  0x1c0c  Peripheral control register
 *  0x1c0d  Interrupt flag register. bit 6: 1 = Timer underflow occurred.
 *  0x1c0e  Interrupt enable register
 *          bit 6: write -  1 = Enable interrupts generated by timer underflow
 *          bit 7: read - interrupt has been generated.
 *          bit 7: write - Fill bit: bits 0-6, that are set to 1, get their values from this bit; bits 0-6, that are set to 0, are left unchanged
 *
 * ca1 byte ready input (also m6502 set overflow input)
 * ca2 set overflow enable for 6502
 * ca3 read/write
 *
 * irq to m6502 irq connected
 *
 **************************************************************************/


static void vc1541_via1_irq( const device_config *device, int level )
{
	vc1541_drive.via1irq = level;
	DBG_LOG(device->machine, 2, "vc1541 via1 irq", ("level %d %d\n", vc1541_drive.via0irq, vc1541_drive.via1irq));

	cputag_set_input_line(device->machine, vc1541_drive.cputag, M6502_IRQ_LINE, vc1541_drive.via1irq || vc1541_drive.via0irq);
}

static READ8_DEVICE_HANDLER( vc1541_via1_read_porta )
{
	UINT8 data = vc1541_drive.gcr.data[vc1541_drive.start_pos + vc1541_drive.pos];

//  logerror("Data read: Track %.1f Pos %d\n", vc1541_drive.track, vc1541_drive.pos);

	DBG_LOG(device->machine, 2, "vc1541 drive", ("port a read %.2x\n", data));
	return data;
}

static WRITE8_DEVICE_HANDLER( vc1541_via1_write_porta )
{
	DBG_LOG(device->machine, 1, "vc1541 drive", ("port a write %.2x\n", data));
}

static  READ8_DEVICE_HANDLER( vc1541_via1_read_portb )
{
	UINT8 value = 0xff;

#if 0
	if (WRITEPROTECTED)
		value &= ~0x10;
#endif

	if (vc1541_drive.gcr.sync)
	{
		value &= ~0x80;
	}

	return value;
}

static int old = 0;

static WRITE8_DEVICE_HANDLER( vc1541_via1_write_portb )
{
	if (data != old)
	{
		DBG_LOG(device->machine, 1, "vc1541 drive",("%.2x\n", data));

	/*  decrease value (%00-%11-%10-%01-%00...) to move head downwards; */
	/*  increase value (%00-%01-%10-%11-%00...) to move head upwards.   */
		if ((old & 0x03) != (data & 0x03))
		{
			switch (old & 0x03)
			{
			case 0:
				if ((data & 0x03) == 1)
					vc1541_drive.track += 0.5;
				else if ((data & 0x03) == 3)
					vc1541_drive.track -= 0.5;
				break;

			case 1:
				if ((data & 0x03) == 2)
					vc1541_drive.track += 0.5;
				else if ((data & 0x03) == 0)
					vc1541_drive.track -= 0.5;
				break;

			case 2:
				if ((data & 0x03) == 3)
					vc1541_drive.track += 0.5;
				else if ((data & 0x03) == 1)
					vc1541_drive.track -= 0.5;
				break;

			case 3:
				if ((data & 0x03) == 0)
					vc1541_drive.track += 0.5;
				else if ((data & 0x03) == 2)
					vc1541_drive.track -= 0.5;
				break;
			}

			/* If we have reached the first or last track, we cannot proceed further! */
			if (vc1541_drive.track < 1)
				vc1541_drive.track = 1.0;
			if (vc1541_drive.track > 35)
				vc1541_drive.track = 35.0;

			logerror("Track #: %.1f\n", vc1541_drive.track);

			/* We now adjust the vc1541_drive.pos according to the new track */
			/* This seems a simple way to deal with half tracks */
			vc1541_drive.pos = 0;
			vc1541_drive.start_pos = gcr_offset[(int)vc1541_drive.track - 1]				// starting point of Track #
				+ (int)(2 * (vc1541_drive.track - (int)vc1541_drive.track)) * 184 / 2;	// half track if needed (again, 184 comes from out loading choices)
		}

		if ((vc1541_drive.motor != (data & 0x04)) || (vc1541_drive.frequency != (data & 0x60)))
		{
			double tme;
			vc1541_drive.motor = data & 0x04;
			vc1541_drive.frequency = data & 0x60;
			tme = drive_times[vc1541_drive.frequency >> 5];

			if (vc1541_drive.motor)
			{
				if (attotime_to_double(timer_timeelapsed(vc1541_drive.timer)) > 1.0e29)
				{
					timer_reset(vc1541_drive.timer, attotime_never);
				}
				else
				{
					timer_adjust_periodic(vc1541_drive.timer, attotime_zero, 0, ATTOTIME_IN_HZ(tme));
					logerror("Freq changed: %f\n", tme);
				}
			}
			else
			{
				timer_reset(vc1541_drive.timer, attotime_never);
				logerror("Motor turned off\n");
			}
		}
		old = data;
	}

	vc1541_drive.led = data & 0x08;
}

static const via6522_interface vc1541_via1 =
{
	DEVCB_HANDLER(vc1541_via1_read_porta),
	DEVCB_HANDLER(vc1541_via1_read_portb),
	DEVCB_NULL,									/*via3_read_ca1, */
	DEVCB_NULL,									/*via3_read_cb1, */
	DEVCB_NULL,									/*via3_read_ca2, */
	DEVCB_NULL,									/*via3_read_cb2, */
	DEVCB_HANDLER(vc1541_via1_write_porta),
	DEVCB_HANDLER(vc1541_via1_write_portb),
	DEVCB_NULL,									/*via3_write_ca1, */
	DEVCB_NULL,									/*via3_write_cb1, */
	DEVCB_NULL,									/*via3_write_ca2, */
	DEVCB_NULL,									/*via3_write_cb2, */
	DEVCB_LINE(vc1541_via1_irq)
};


/**************************************

    1541 / 1541B / 1541C / 1541II /
        2031 (same as 1541 but IEEE)

**************************************/

static ADDRESS_MAP_START( _1541_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE("_1541_via_0", via_r, via_w)	/* 0 and 1 used in vc20 */	// IEC Bus
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE("_1541_via_1", via_r, via_w)		// Drive
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( dolphin_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE("_1541_via_0", via_r, via_w)	/* 0 and 1 used in vc20 */
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE("_1541_via_1", via_r, via_w)
	AM_RANGE(0x8000, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END

/**************************************

    1570 / 1571 / 1571CR

**************************************/

static ADDRESS_MAP_START( _1571_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE("_1541_via_0", via_r, via_w)  /* 0 and 1 used in vc20 */
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE("_1541_via_1", via_r, via_w)
//  AM_RANGE(0x2000, 0x2003) // WD17xx
//  AM_RANGE(0x4000, 0x400f) // CIA
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

// for the C128DCR Drive
/*
David Gahris provided info about differences between 128DCR and stand-alone 1571s

* There is no 6526 or 8521 in the drive section of the 128DCR.  I believe the
  5710 handles fast serial because the 5710 is a Commodore custom chip and
  128 fast serial is a Commodore custom protocol.  [Ed's note: Commodore lists
  NTE229, ECG229, SK3246A/229, and 121-Z9021 as substitutes of the 5710.]

* There is no 177x or heat-shrink-wrapped daughterboard in the drive section
  of the 128DCR.  I believe the Motorola plastic quad flat-pack chip handles
  MFM because I am pretty sure that I have seen this chip in IBM PC drive
  mechanisms.

* The 1571cr ROM works great in 1541 mode but not at all in 1571 mode in a
  standalone 1571.

* Olaf Seibert informed me that the 1571cr ROM contains a string to the
  effect of "5710 (c) 1986 by c. hauck"
*/
static ADDRESS_MAP_START( _1571cr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE("_1541_via_0", via_r, via_w)  /* 0 and 1 used in vc20 */
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE("_1541_via_1", via_r, via_w)
//  AM_RANGE(0x2000, 0x2005) // 5710 FDC
//  AM_RANGE(0x400c, 0x400f) // 5710 CIA Serial Port
//  AM_RANGE(0x4010, 0x4017) // 5710 FDC2
//  AM_RANGE(0x6000, 0x7fff) // RAM shadows
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


/**************************************

    1581

**************************************/

static ADDRESS_MAP_START( _1581_map, ADDRESS_SPACE_PROGRAM, 8 )
//  AM_RANGE(0x4000, 0x400f) // CIA
//  AM_RANGE(0x6000, 0x6003) // WD17xx
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


/**************************************

    1001 / 8050 / 8250

http://www.zimmers.net/anonftp/pub/cbm/firmware/drives/old/8050/d/8250.notes

**************************************/

static ADDRESS_MAP_START( sfd1001_map, ADDRESS_SPACE_PROGRAM, 8 )
//  AM_RANGE(0x0200, 0x021f) // RIOT1
//  AM_RANGE(0x0280, 0x029f) // RIOT2
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/**************************************

    2040 / 3040 / 4040

http://www.zimmers.net/anonftp/pub/cbm/schematics/drives/old/4040/memorymap.txt

**************************************/

static ADDRESS_MAP_START( _4040_map, ADDRESS_SPACE_PROGRAM, 8 )
ADDRESS_MAP_END


/**************************************

    1551

**************************************/

/*
  c1551 irq line
  only timing related??? (60 Hz?), delivered from c16?
 */
static TIMER_CALLBACK(c1551_irq_timer)
{
	cputag_set_input_line(machine, c1551_drive.cputag, M6502_IRQ_LINE, ASSERT_LINE);
}

/*
  ddr 0x6f (p0 .. p5? in pinout)
  0,1: output stepper motor
  2 output motor (rotation) (300 revolutions per minute)
  3 output led ?
  4 input disk not write protected ?
  5,6: output frequency select
  7: input byte ready
 */

static WRITE8_HANDLER( c1551_port_w )
{
	if (offset)
	{
		DBG_LOG(space->machine, 1, "c1551 port",("write %.2x\n", data));
		c1551_drive.cpu_port = data;

		if (data != old)
		{
			DBG_LOG(space->machine, 1, "c1551 drive",("%.2x\n", data));

			if ((old & 0x03) != (data & 0x03))
			{
				switch (old & 0x03)
				{
				case 0:
					if ((data & 0x03) == 1)
						c1551_drive.track += 0.5;
					else if ((data & 0x03) == 3)
						c1551_drive.track -= 0.5;
					break;

				case 1:
					if ((data & 0x03) == 2)
						c1551_drive.track += 0.5;
					else if ((data & 0x03) == 0)
						c1551_drive.track -= 0.5;
					break;

				case 2:
					if ((data & 0x03) == 3)
						c1551_drive.track += 0.5;
					else if ((data & 0x03) == 1)
						c1551_drive.track -= 0.5;
					break;

				case 3:
					if ((data & 0x03) == 0)
						c1551_drive.track += 0.5;
					else if ((data & 0x03) == 2)
						c1551_drive.track -= 0.5;
					break;
				}

				/* If we have reached the first or last track, we cannot proceed further! */
				if (c1551_drive.track < 1)
					c1551_drive.track = 1.0;
				if (c1551_drive.track > 35)
					c1551_drive.track = 35.0;

			logerror("Track #: %.1f\n", c1551_drive.track);

			/* We now adjust the c1551_drive.pos according to the new track */
			/* This seems a simple way to deal with half tracks */
			c1551_drive.pos = 0;
			c1551_drive.start_pos = gcr_offset[(int)c1551_drive.track - 1]				// starting point of Track #
				+ (int)(2 * (c1551_drive.track - (int)c1551_drive.track)) * 184 / 2;	// half track if needed (again, 184 comes from out loading choices)
			}

			if ((c1551_drive.motor != (data & 0x04)) || (c1551_drive.frequency != (data & 0x60)))
			{
				double tme;
				c1551_drive.motor = data & 0x04;
				c1551_drive.frequency = data & 0x60;

				tme = drive_times[c1551_drive.frequency >> 5];
				if (c1551_drive.motor)
				{
					if (attotime_to_double(timer_timeelapsed(c1551_drive.timer)) > 1.0e29)
						timer_reset(c1551_drive.timer, attotime_never);
					else
						timer_adjust_periodic(c1551_drive.timer, attotime_zero, 0, ATTOTIME_IN_HZ(tme));
				}
				else
				{
					timer_reset(c1551_drive.timer, attotime_never);
				}
			}
			old = data;
		}
		c1551_drive.led = data & 0x08;
	}
	else
	{
		c1551_drive.cpu_ddr = data;
		DBG_LOG(space->machine, 1, "c1551 ddr", ("write %.2x\n", data));
	}
}

static READ8_HANDLER( c1551_port_r )
{
	int data;

	if (offset)
	{
		data = 0x7f;
#if 0
		if (WRITEPROTECTED)
			data &= ~0x10;
#endif
		if (c1551_drive.gcr.ready)
		{
			data |= 0x80;
			c1551_drive.gcr.ready = 0;
		}

		data &= ~c1551_drive.cpu_ddr;
		data |= c1551_drive.cpu_ddr & c1551_drive.cpu_port;
		DBG_LOG(space->machine, 3, "c1551 port",("read %.2x\n", data));
	}
	else
	{
		data = c1551_drive.cpu_ddr;
		DBG_LOG(space->machine, 3, "c1551 ddr",("read %.2x\n", data));
	}
	return data;
}


static ADDRESS_MAP_START( _1551_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0001) AM_READWRITE(c1551_port_r, c1551_port_w)
	AM_RANGE(0x0002, 0x07ff) AM_RAM
	AM_RANGE(0x4000, 0x4007) AM_DEVREADWRITE("c1551_tpi", tpi6525_r, tpi6525_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/******************** Handlers for the C1551 TPI ********************/

/*
   tia6523
   port a
    c16 communication in/out
   port b
    drive data in/out
   port c ddr (0x1f)
    0 output status out
    1 output status out
    2 output
    3 output handshake out
    4 output
    5 input drive number 9
    6 input sync 0 active
    7 input handshake in
 */
static READ8_DEVICE_HANDLER( c1551_port_c_r )
{
	int data = 0xff;

	data &= ~0x20;

	if (c1551_drive.gcr.sync)
		data &= ~0x40;

	return data;
}

static READ8_DEVICE_HANDLER( c1551_port_b_r )
{
	UINT8 data = c1551_drive.gcr.data[c1551_drive.start_pos + c1551_drive.pos];

	DBG_LOG(device->machine, 2, "c1551 drive",("port a read %.2x\n", data));
	return data;
}

static const tpi6525_interface c1551_tpi_intf =
{
	NULL,
	c1551_port_b_r,
	c1551_port_c_r,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


/******************** Handlers for the C16 TPI ********************/

WRITE8_DEVICE_HANDLER( c1551x_write_data )
{
//  DBG_LOG(device->machine, 1, "c1551 cpu", ("%d write data %.2x\n", cpu_getactivecpu (), data));
#ifdef CPU_SYNC
	cpu_sync();
#endif
	const device_config *tpi_0 = devtag_get_device(device->machine, "c1551_tpi");

	tpi6525_porta_w(tpi_0, 0, data);
}

READ8_DEVICE_HANDLER( c1551x_read_data )
{
	UINT8 data = 0xff;
	const device_config *tpi_0 = devtag_get_device(device->machine, "c1551_tpi");

#ifdef CPU_SYNC
	cpu_sync ();
#endif

	data = tpi6525_porta_r(tpi_0, 0);
//  DBG_LOG(device->machine, 2, "c1551 cpu",("%d read data %.2x\n", cpu_getactivecpu (), data));
	return data;
}

WRITE8_DEVICE_HANDLER( c1551x_write_handshake )
{
	const device_config *tpi_0 = devtag_get_device(device->machine, "c1551_tpi");
//  DBG_LOG(device->machine, 1, "c1551 cpu",("%d write handshake %.2x\n", cpu_getactivecpu (), data));
#ifdef CPU_SYNC
	cpu_sync();
#endif

	tpi6525_portc_w(tpi_0, 0, data & 0x40 ? 0xff : 0x7f);
}

READ8_DEVICE_HANDLER( c1551x_read_handshake )
{
	const device_config *tpi_0 = devtag_get_device(device->machine, "c1551_tpi");
	UINT8 data = 0xff;
#ifdef CPU_SYNC
	cpu_sync();
#endif

	data = tpi6525_portc_r(tpi_0, 0) & 0x08 ? 0x80 : 0x00;
//  DBG_LOG(device->machine, 2, "c1551 cpu",("%d read handshake %.2x\n", cpu_getactivecpu (), data));
	return data;
}

READ8_DEVICE_HANDLER( c1551x_read_status )
{
	const device_config *tpi_0 = devtag_get_device(device->machine, "c1551_tpi");
	UINT8 data = 0xff;
#ifdef CPU_SYNC
	cpu_sync();
#endif

	data = tpi6525_portc_r(tpi_0, 0) & 0x03;
//  DBG_LOG(device->machine, 1, "c1551 cpu",("%d read status %.2x\n", cpu_getactivecpu (), data));

	return data;
}



/**************************************

    Machine drivers

**************************************/


MACHINE_DRIVER_START( cpu_vc1540 )
	MDRV_CPU_ADD("cpu_vc1540", M6502, XTAL_16_MHz / 16)
	MDRV_CPU_PROGRAM_MAP(_1541_map)

	MDRV_VIA6522_ADD("_1541_via_0", 0, vc1541_via0)
	MDRV_VIA6522_ADD("_1541_via_1", 0, vc1541_via1)

	MDRV_CBM_SERBUS_ADD("serial_bus", cbm_emu_drive_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_vc1541 )
	MDRV_IMPORT_FROM(cpu_vc1540)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c1571 )
	MDRV_CPU_ADD("cpu_vc1571", M6502, XTAL_16_MHz / 16)
	MDRV_CPU_PROGRAM_MAP(_1571_map)

	MDRV_VIA6522_ADD("_1541_via_0", 0, vc1541_via0)
	MDRV_VIA6522_ADD("_1541_via_1", 0, vc1541_via1)

	MDRV_CBM_SERBUS_ADD("serial_bus", cbm_emu_drive_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c1571cr )
	MDRV_CPU_ADD("cpu_vc1571", M6502, XTAL_16_MHz / 16)
	MDRV_CPU_PROGRAM_MAP(_1571cr_map)

	MDRV_VIA6522_ADD("_1541_via_0", 0, vc1541_via0)
	MDRV_VIA6522_ADD("_1541_via_1", 0, vc1541_via1)

	MDRV_CBM_SERBUS_ADD("serial_bus", cbm_emu_drive_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c1581 )
	MDRV_IMPORT_FROM(cpu_c1571)		// obviously incorrect: we need proper 1581 memory map and emulation!
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c2031 )
	MDRV_IMPORT_FROM(cpu_vc1540)	// not sure the memory map is exactly the vc1541 one!
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_dolphin )
	MDRV_CPU_ADD("cpu_dolphin", M6502, XTAL_16_MHz / 16)
	MDRV_CPU_PROGRAM_MAP(dolphin_map)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c1551 )
	MDRV_CPU_ADD("cpu_c1551", M6510T, 2000000)
	MDRV_CPU_PROGRAM_MAP(_1551_map)

	/* tpi */
	MDRV_TPI6525_ADD("c1551_tpi", c1551_tpi_intf)

	MDRV_CBM_SERBUS_ADD("serial_bus", cbm_emu_drive_interface)	// in the current code, serial bus device is tied to the floppy drive
																// and c16 reads/writes to serial bus in m7501_port_r/w!
MACHINE_DRIVER_END



/**************************************

    Drive init & reset

**************************************/

void cbm_drive_config( running_machine *machine, int type, int id, int mode, const char *cputag, int devicenr )
{
	vc1541_drive.type = type;
	vc1541_drive.cputag = cputag;
	vc1541_drive.vc1541_serial.deviceid = devicenr;
	vc1541_drive.timer = timer_alloc(machine, vc1541_drive_timer, NULL);
	vc1541_drive.gcr.data = (UINT8*)auto_alloc_array(machine, UINT8, 0x500000);
}

void c1551_config( running_machine *machine, const char *cputag )
{
	c1551_drive.type = type_1551;
	c1551_drive.cputag = cputag;
	c1551_drive.timer = timer_alloc(machine, c1551_drive_timer, NULL);
	c1551_drive.gcr.data = (UINT8*)auto_alloc_array(machine, UINT8, 0x500000);

	/* time should be small enough to allow quitting of the irq
    line before the next interrupt is triggered */
	c1551_drive.irq_timer = timer_alloc(machine, c1551_irq_timer, NULL);
	timer_adjust_periodic(c1551_drive.irq_timer, attotime_zero, 0, ATTOTIME_IN_HZ(60));
}

void cbm_drive_reset( running_machine* machine )
{
	int i;

	if (vc1541_drive.type == type_1541)
	{
		for (i = 0; i < ARRAY_LENGTH(serial.atn); i++)
		{
			serial.atn[i] = serial.data[i] = serial.clock[i] = 1;
		}
	}

	/* Create offset map for GCR encoded images: it speeds up finding position where to read data from */
	switch (vc1541_drive.type)
	{
		case type_1541:
		case type_1541ii:
			offset_init(format_d64_40t, gcr_offset, 368);	// offset map is the same for both 35 and 40 track discs
			break;

		case type_1570:
		case type_1571:
		case type_1571cr:
			offset_init(format_d71, gcr_offset, 368);
			break;

		case type_1581:
			offset_init(format_d81, gcr_offset, 368);
			break;

		default:
			logerror("GCR offset map not created\n");
	}

	vc1541_drive.gcr.sync = 0;
	vc1541_drive.gcr.ones_count = 0;
	vc1541_drive.gcr.ready = 0;

	vc1541_drive.track = 1.0;
	vc1541_drive.start_pos = 0;
	vc1541_drive.pos = 0;
	vc1541_drive.led = 0;
	vc1541_drive.motor = 0;
	vc1541_drive.frequency = 0;
	vc1541_drive.clock = 0;

	vc1541_drive.vc1541_serial.acka = 0;
	vc1541_drive.vc1541_serial.data = 0;

}

void c1551_drive_reset( void )
{
	offset_init(format_d64_40t, gcr_offset, 368);	// offset map is the same for both 35 and 40 track discs

	c1551_drive.gcr.sync = 0;
	c1551_drive.gcr.ones_count = 0;
	c1551_drive.gcr.ready = 0;

	c1551_drive.track = 1.0;
	c1551_drive.start_pos = 0;
	c1551_drive.pos = 0;
	c1551_drive.led = 0;
	c1551_drive.motor = 0;
	c1551_drive.frequency = 0;
	c1551_drive.clock = 0;
}



/**************************************

    Device configurations

**************************************/

/* Floppy loading clean-up, part 1: with cbm_floppy_image_loading we can finally split c1551 from vc1541
   Both struct have a GCR_data gcr component where we store the floppy data, but remaining parts are finally separated!
   Later, everything should be changed to use devices for the drives...      */
static void cbm_floppy_image_loading( int format, int id1, int id2, UINT8 *copy, GCR_data gcr )
{
	UINT8 sector_checksum;
	int i = 0, j, k, l, pos;

	for (k = 1; k <= max_tracks_per_format[format]; k++)
	{
/*      logerror("decoding track %d offset %d %d %d %d %d\n", k,
                        image_tracksector2offset(k, 0) / (16*16*16*16),
                        (image_tracksector2offset(k, 0) % (16*16*16*16)) / (16*16*16),
                        (image_tracksector2offset(k, 0) % (16*16*16)) / (16*16),
                        (image_tracksector2offset(k, 0) % (16*16)) / 16,
                        image_tracksector2offset(k, 0) % 16);
*/

		for (l = 0; l < d64_sectors_per_track[k - 1]; l++)
		{
			// here we convert the sector data to gcr directly!
			// IMPORTANT: we shall implement errors in reading sectors!
			// these can modify e.g. header info $01 & $05

			// first we set the position at which sector data starts in the image
			pos = image_tracksector2offset(k, l);

			/*
                1. Header sync       FF FF FF FF FF (40 'on' bits, not GCR encoded)
                2. Header info       52 54 B5 29 4B 7A 5E 95 55 55 (10 GCR bytes)
                3. Header gap        55 55 55 55 55 55 55 55 55 (9 bytes, never read)
                4. Data sync         FF FF FF FF FF (40 'on' bits, not GCR encoded)
                5. Data block        55...4A (325 GCR bytes)
                6. Inter-sector gap  55 55 55 55...55 55 (4 to 19 bytes, never read)
            */

			/* Header sync */
			for (j = 0; j < 5; j++)
				filling_byte(0xff, gcr.data + i + j);
			i += 5;

			/* Header info */
			/* These are 8 bytes unencoded, which become 10 bytes encoded */
			// $00 - header block ID ($08)                      // this byte can be modified by error code 20 -> 0xff
			// $01 - header block checksum (EOR of $02-$05)     // this byte can be modified by error code 27 -> ^ 0xff
			// $02 - Sector# of data block
			// $03 - Track# of data block
			gcr_double_2_gcr(0x08, l ^ k ^ id2 ^ id1, l, k, gcr.data + i);
			i += 5;

			// $04 - Format ID byte #2
			// $05 - Format ID byte #1
			// $06 - $0F ("off" byte)
			// $07 - $0F ("off" byte)
			gcr_double_2_gcr(id2, id1, 0x0f, 0x0f, gcr.data + i);
			i += 5;

			/* Header gap */
			for (j = 0; j < 9; j++)
				filling_byte(0x55, gcr.data + i + j);
			i += 9;

			/* Data sync */
			for (j = 0; j < 5; j++)
				filling_byte(0xff, gcr.data + i + j);
			i += 5;

			/* Data block */
			// we first need to calculate the checksum of the 256 bytes of the sector
			sector_checksum = copy[pos];
			for (j = 1; j < 256; j++)
				sector_checksum ^= copy[pos + j];

			/*
                $00      - data block ID ($07)
                $01-100  - 256 bytes sector data
                $101     - data block checksum (EOR of $01-100)
                $102-103 - $00 ("off" bytes, to make the sector size a multiple of 5)
            */
			gcr_double_2_gcr(0x07, copy[pos], copy[pos + 1],
								copy[pos + 2], gcr.data + i);
			i += 5;

			for (j = 1; j < 64; j++)
			{
				gcr_double_2_gcr(copy[pos + 4 * j - 1], copy[pos + 4 * j],
									copy[pos + 4 * j + 1], copy[pos + 4 * j + 2], gcr.data + i);
				i += 5;
			}

			gcr_double_2_gcr(copy[pos + 255], sector_checksum, 0x00, 0x00, gcr.data + i);
			i += 5;

			/* Inter-sector gap */
			// "In tests that the author conducted on a real 1541 disk, gap sizes of 8  to  19 bytes were seen."
			// Here we put 14 as an average...
			for (j = 0; j < 14; j++)
				filling_byte(0x55, gcr.data + i + j);
			i += 14;
		}
	}

}

static DEVICE_IMAGE_LOAD( _1541 )
{
	UINT8 format_id_1, format_id_2;
	int format = 0, filesize;
	UINT8 *temp_copy = NULL;
	const char *filetype;

	filesize = image_length(image);
	filetype = image_filetype(image);

	/* Find out which type of image we are loading */
	if (!mame_stricmp (filetype, "d64"))
	{
		switch (filesize)
		{
			case 0x2ab00:
				format = format_d64;
				break;
			case (0x2ab00 + 0x2ab):
				format = format_d64_err;
				break;
			case 0x30000:
				format = format_d64_40t;
				break;
			case (0x30000 + 0x300):
				format = format_d64_40t_err;
				break;
			default:
				logerror("The image you're trying to load has not the regular size of a .d64 image: \n");
				logerror("Image %s size %d\n", image_filename(image), filesize);
				return INIT_FAIL;
		}
	}
	// these are not supported yet!
	else if (!mame_stricmp (filetype, "d71"))
	{
		switch (filesize)
		{
			case 0x55600:
				format = format_d71;
				break;
			case (0x55600 + 0x556):
				format = format_d71_err;
				break;
			default:
				logerror("The image you're trying to load has not the regular size of a .d71 image: \n");
				logerror("Image %s size %d\n", image_filename(image), filesize);
				return INIT_FAIL;
		}
	}
	else if (!mame_stricmp (filetype, "d67"))
		format = format_d67;
	else if (!mame_stricmp (filetype, "d81"))
		format = format_d81;
	else if (!mame_stricmp (filetype, "d80"))
		format = format_d80;
	else if (!mame_stricmp (filetype, "d82"))
		format = format_d82;
	else
	{
		/* Still have to work on g64 images */
		logerror("Unsupported format\n");
		return INIT_FAIL;
	}

	/* Set up track / sector map according to the format of the image */
	offset_init(format, image_offset, 256);

	/* Claim memory */
	temp_copy = auto_alloc_array(image->machine, UINT8, filesize);

	if ((image_fread( image, temp_copy, filesize ) != filesize) || !filesize)
		return INIT_FAIL;

	/* Start GCR encoding */
	format_id_1 = temp_copy[disk_id1(format)];	// can be modified by error code 29 -> ^ 0xff
	format_id_2 = temp_copy[disk_id2(format)];

	/* Disk is encoded and stored in xxx_drive.gcr.data */
	cbm_floppy_image_loading( format, format_id_1, format_id_2, temp_copy, vc1541_drive.gcr );

	logerror("Floppy image %s successfully loaded\n", image_filename(image));
	return INIT_PASS;
}

static DEVICE_IMAGE_LOAD( _1551 )
{
	UINT8 format_id_1, format_id_2;
	int format = 0, filesize;
	UINT8 *temp_copy = NULL;
	const char *filetype;

	filesize = image_length(image);
	filetype = image_filetype(image);

	/* Find out which type of image we are loading */
	if (!mame_stricmp (filetype, "d64"))
	{
		switch (filesize)
		{
			case 0x2ab00:
				format = format_d64;
				break;
			case (0x2ab00 + 0x2ab):
				format = format_d64_err;
				break;
			case 0x30000:
				format = format_d64_40t;
				break;
			case (0x30000 + 0x300):
				format = format_d64_40t_err;
				break;
			default:
				logerror("The image you're trying to load has not the regular size of a .d64 image: \n");
				logerror("Image %s size %d\n", image_filename(image), filesize);
				return INIT_FAIL;
		}
	}
	else
	{
		logerror("Unsupported format\n");
		return INIT_FAIL;
	}

	/* Set up track / sector map according to the format of the image */
	offset_init(format, image_offset, 256);

	/* Claim memory */
	temp_copy = auto_alloc_array(image->machine, UINT8, filesize);

	if ((image_fread( image, temp_copy, filesize ) != filesize) || !filesize)
		return INIT_FAIL;

	/* Start GCR encoding */
	format_id_1 = temp_copy[disk_id1(format)];	// can be modified by error code 29 -> ^ 0xff
	format_id_2 = temp_copy[disk_id2(format)];

	/* Disk is encoded and stored in xxx_drive.gcr.data */
	cbm_floppy_image_loading( format, format_id_1, format_id_2, temp_copy, c1551_drive.gcr );

	logerror("Floppy image %s successfully loaded\n", image_filename(image));
	return INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD( _1541 )
{
}

void vc1541_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:					info->i = IO_FLOPPY; break;
		case MESS_DEVINFO_INT_COUNT:				info->i = 1; break;
		case MESS_DEVINFO_INT_READABLE:				info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:			info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:			info->i = 0; break;
		case MESS_DEVINFO_INT_RESET_ON_LOAD:		info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(_1541); break;
		case MESS_DEVINFO_PTR_UNLOAD:				info->unload = DEVICE_IMAGE_UNLOAD_NAME(_1541); break;
		case MESS_DEVINFO_PTR_DEV_SPECIFIC:			info->f = (genf *) cbm_drive_config; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "d64"); break;
	}
}


void c2031_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	vc1541_device_getinfo(devclass, state, info);
}


void c1551_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(_1551); break;
		case MESS_DEVINFO_PTR_DEV_SPECIFIC:				info->f = (genf *) c1551_config; break;

		default:									vc1541_device_getinfo(devclass, state, info);
	}
}


void c1571_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "d71"); break;

		default:									vc1541_device_getinfo(devclass, state, info);
	}
}

void c1581_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "d81"); break;

		default:									vc1541_device_getinfo(devclass, state, info);
	}
}

void c8050_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "d80"); break;

		default:									vc1541_device_getinfo(devclass, state, info);
	}
}

void c8250_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "d82"); break;

		default:									vc1541_device_getinfo(devclass, state, info);
	}
}

void c2040_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "d67"); break;

		default:									vc1541_device_getinfo(devclass, state, info);
	}
}
