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
  (m6510t internal 4mhz clock?, 8 port pins?)
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
started some work on the floppy emulation code:
	+ made the functions / structs less vc1541-centric	
	+ unified config & reset functions, using type variable
		to choose the actions to be done
 
a limitation is clear: only a single occurrence of emulated drive
is expected. 
at a later stage, at least drive_config & drive_reset will need
an index to choose between the drives

current status:
	+ preliminary vc1541 & c1551 implementation (I still have to 
		investigate how good)
	+ no support for other drive types (even if now I list all
		the variants supported by Vice in the enum)
*/

#include "driver.h"
#include "deprecat.h"
#include "image.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"

#include "machine/tpi6525.h"

#include "includes/vc1541.h"


#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	{ \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time()), (char*) M ); \
			logerror A; \
		} \
	}

#define XTAL_16_MHz	16000000

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
	17, 17, 17, 17, 17		/* only for 40 tracks d64 */
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


static int image_offset[IMAGE_MAX_TRACKS] = { 0 };


static void offset_init (int format)
{
	int i;

	/* Track # starts from 1 */
	image_offset[0] = 0;

	switch (format)
	{
		case format_d64:
			for (i = 1; i <= D64_MAX_TRACKS; i++)
				image_offset[i] = image_offset[i - 1] + d64_sectors_per_track[i - 1] * 256;
			break;

		case format_d64_40t:
			for (i = 1; i <= D64_40T_MAX_TRACKS; i++)
				image_offset[i] = image_offset[i - 1] + d64_sectors_per_track[i - 1] * 256;
			break;

		case format_d71:
			for (i = 1; i <= D71_MAX_TRACKS; i++)
				image_offset[i] = image_offset[i - 1] + d71_sectors_per_track[i - 1] * 256;
			break;

		case format_d81:
			for (i = 1; i <= D81_MAX_TRACKS; i++)
				image_offset[i] = image_offset[i - 1] + 40 * 256;
			break;

		case format_d80:
			for (i = 1; i <= D80_MAX_TRACKS; i++)
				image_offset[i] = image_offset[i - 1] + d80_sectors_per_track[i - 1] * 256;
			break;

		case format_d82:
			for (i = 1; i <= D82_MAX_TRACKS; i++)
				image_offset[i] = image_offset[i - 1] + d82_sectors_per_track[i - 1] * 256;
			break;

		default:
			logerror("Unsupported format");
			break;
	}
}


/* calculates offset to beginning of the image file for sector beginning */
/* since sectors are always 256 bytes large, this works for each file format! */
static int image_tracksector2offset (int track, int sector)
{
	return image_offset[track - 1] + sector * 256;
}


#define D64_TRACK_ID1   (image_tracksector2offset(18,0)+162)
#define D64_TRACK_ID2   (image_tracksector2offset(18,0)+163)

/*
 * only for testing at the moment
 */

#define WRITEPROTECTED 0

/* 0 summarized, 1 computer, 2 vc1541 */
static struct
{
	int atn[3], data[3], clock[3];
}
serial;

/* G64 or D64 image
 implementation as writeback system
 */

typedef struct
{
	int cpunumber;
	int type;
	union {
		struct {
			int deviceid;
			int serial_atn, serial_clock, serial_data;
			int acka, data;
		} serial;
		struct {
			int deviceid;
		} ieee488;
		struct {
			UINT8 cpu_ddr, cpu_port;
			void *irq_timer;
		} c1551;
	} drive;

	int via0irq, via1irq;

	int led, motor, frequency;

	double track;
	int clock;

	void *timer;

	struct {
		UINT8 data[(1+2+2+1+256/4+4)*5];
		int sync;
		int ready;
		int ffcount;
	} head;

	struct {
		int pos; /* position  in sector */
		int sector;
		UINT8 *data;
	} image;
} CBM_Drive_Emu;

static CBM_Drive_Emu drive_static= { 0 }, *drive = &drive_static;


/**************************************

	Generic image handling

**************************************/


/* four different frequencies for the 4 different zones on the disk */
static const double drive_times[4]= {
	13/16e6, 14/16e6, 15/16e6, 16/16e6
};

/*
 * gcr encoding 4 bits to 5 bits
 * 1 change, 0 none
 *
 * physical encoding of the data on a track
 * sector header
 *  sync (5x 0xff not gcr encoded)
 *  sync mark (0x08)
 *  checksum (chksum xor sector xor track xor id1 xor id2 gives 0)
 *  sector#
 *  track#
 *  id2 (disk id to prevent writing to disk after disk change)
 *  id1
 *  0x0f
 *  0x0f
 * cap normally 10 (min 5) byte?
 * sector data
 *  sync (5x 0xff not gcr encoded)
 *  sync mark (0x07)
 *  256 bytes
 *  checksum (256 bytes xored)
 * cap
 *
 * max 42 tracks, stepper resolution 84 tracks
 */
static const int bin_2_gcr[] =
{
	0x0a, 0x0b, 0x12, 0x13, 0x0e, 0x0f, 0x16, 0x17,
	0x09, 0x19, 0x1a, 0x1b, 0x0d, 0x1d, 0x1e, 0x15
};

#if 0
static const int gcr_2_bin[] = {
	-1, -1, -1, -1,
	-1, -1, -1, -1,
	-1, 8, 0, 1,
	-1, 0xc, 4, 5,
	-1, -1, 2, 3,
	-1, 0xf, 6, 7,
	-1, 9, 0xa, 0xb,
	-1, 0xd, 0xe, -1
};
#endif

static void gcr_double_2_gcr(UINT8 a, UINT8 b, UINT8 c, UINT8 d, UINT8 *dest)
{
	UINT8 gcr[8];
	gcr[0]=bin_2_gcr[a>>4];
	gcr[1]=bin_2_gcr[a&0xf];
	gcr[2]=bin_2_gcr[b>>4];
	gcr[3]=bin_2_gcr[b&0xf];
	gcr[4]=bin_2_gcr[c>>4];
	gcr[5]=bin_2_gcr[c&0xf];
	gcr[6]=bin_2_gcr[d>>4];
	gcr[7]=bin_2_gcr[d&0xf];
	dest[0]=(gcr[0]<<3)|(gcr[1]>>2);
	dest[1]=(gcr[1]<<6)|(gcr[2]<<1)|(gcr[3]>>4);
	dest[2]=(gcr[3]<<4)|(gcr[4]>>1);
	dest[3]=(gcr[4]<<7)|(gcr[5]<<2)|(gcr[6]>>3);
	dest[4]=(gcr[6]<<5)|gcr[7];
}

static struct {
	int count;
	int data[4];
} gcr_helper;

static void drive_sector_start(void)
{
	gcr_helper.count=0;
}

static void drive_sector_data(UINT8 data, int *pos)
{
	gcr_helper.data[gcr_helper.count++]=data;
	if (gcr_helper.count==4) {
		gcr_double_2_gcr(gcr_helper.data[0], gcr_helper.data[1],
						 gcr_helper.data[2], gcr_helper.data[3],
						 drive->head.data+*pos);
		*pos=*pos+5;
		gcr_helper.count=0;
	}
}

static void drive_sector_end(int *pos)
{
	assert(gcr_helper.count==0);
}

static void drive_sector_to_gcr(int track, int sector)
{
	int i=0, j, offset, chksum=0;

	if (drive->image.data==NULL) return;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive_sector_start();

	drive_sector_data(8, &i);
	chksum= sector^track
		^drive->image.data[D64_TRACK_ID1]^drive->image.data[D64_TRACK_ID2];
	drive_sector_data(chksum, &i);
	drive_sector_data(sector, &i);
	drive_sector_data(track, &i);
	drive_sector_data(drive->image.data[D64_TRACK_ID1], &i);
	drive_sector_data(drive->image.data[D64_TRACK_ID2], &i);
	drive_sector_data(0xf, &i);
	drive_sector_data(0xf, &i);
	drive_sector_end(&i);

	/* 5 - 10 gcr bytes cap */
	gcr_double_2_gcr(0, 0, 0, 0, drive->head.data+i);i+=5;
	gcr_double_2_gcr(0, 0, 0, 0, drive->head.data+i);i+=5;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive->head.data[i++]=0xff;
	drive_sector_data(0x7, &i);

	chksum=0;
	for (offset=image_tracksector2offset(track,sector), j=0; j<256; j++) {
		chksum^=drive->image.data[offset];
		drive_sector_data(drive->image.data[offset++], &i);
	}
	drive_sector_data(chksum, &i);
	drive_sector_data(0, &i); /* padding up */
	drive_sector_data(0, &i);
	drive_sector_end(&i);
	gcr_double_2_gcr(0, 0, 0, 0, drive->head.data+i);i+=5;
	gcr_double_2_gcr(0, 0, 0, 0, drive->head.data+i);i+=5;
}


/**************************************

	1541 / 1541II

**************************************/


static ADDRESS_MAP_START( _1541_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_READWRITE(via_2_r, via_2_w)  /* 0 and 1 used in vc20 */
	AM_RANGE(0x1810, 0x189f) AM_READ(SMH_NOP) /* for debugger */
	AM_RANGE(0x1c00, 0x1c0f) AM_READWRITE(via_3_r, via_3_w)
	AM_RANGE(0x1c10, 0x1c9f) AM_READ(SMH_NOP) /* for debugger */
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( dolphin_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_READWRITE(via_2_r, via_2_w)  /* 0 and 1 used in vc20 */
	AM_RANGE(0x1c00, 0x1c0f) AM_READWRITE(via_3_r, via_3_w)
	AM_RANGE(0x8000, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END


#if 0
INPUT_PORTS_START (vc1541)
PORT_START
PORT_DIPNAME (0x60, 0x00, "Device #", CODE_NONE)
PORT_DIPSETTING (0x00, "8")
PORT_DIPSETTING (0x20, "9")
PORT_DIPSETTING (0x40, "10")
PORT_DIPSETTING (0x60, "11")
INPUT_PORTS_END
#endif

static TIMER_CALLBACK(drive_timer)
{
	if (drive->clock==0) {
		drive->clock=1;
		drive->head.ready=0;
		drive->head.sync=0;
		if (drive->type==type_1541) {
			cpunum_set_input_line(machine, drive->cpunumber, M6502_SET_OVERFLOW, 1);
			via_3_ca1_w(machine, 0,1);
		}
		return;
	}
	if (++(drive->image.pos)>=sizeof(drive->head.data)) {
		if (++(drive->image.sector)>=
			d64_sectors_per_track[(int)drive->track-1]) {
			drive->image.sector=0;
		}
		drive_sector_to_gcr((int)drive->track,drive->image.sector);
		drive->image.pos=0;
	}
	drive->head.ready=1;
	if (drive->head.data[drive->image.pos]==0xff) {
		drive->head.ffcount++;
		if (drive->head.ffcount==5) {
			drive->head.sync=1;
		}
	} else {
		drive->head.ffcount=0;
		drive->head.sync=0;
	}
	if (drive->type==type_1541) {
		cpunum_set_input_line(machine, drive->cpunumber, M6502_SET_OVERFLOW, 0);
		via_3_ca1_w(machine, 0,0);
	}
	drive->clock=0;
}

/*
 * via 6522 at 0x1800
 * port b
 * 0 inverted serial data in
 * 1 inverted serial data out
 * 2 inverted serial clock in
 * 3 inverted serial clock out
 * 4 inverted serial atn out
 * 5 input device id 1
 * 6 input device id 2
 * id 2+id 1/0+0 devicenumber 8/0+1 9/1+0 10/1+1 11
 * 7 inverted serial atn in
 * also ca1 (books says cb2)
 * irq to m6502 irq connected (or with second via irq)
 */
static void vc1541_via0_irq (running_machine *machine, int level)
{
	drive->via0irq = level;
	DBG_LOG(2, "vc1541 via0 irq",("level %d %d\n",drive->via0irq,drive->via1irq));
	cpunum_set_input_line (machine, drive->cpunumber,
					  M6502_IRQ_LINE, drive->via1irq || drive->via0irq);
}

static READ8_HANDLER( vc1541_via0_read_portb )
{
	static int old=-1;
	int value = 0x7a;

	if (!drive->drive.serial.serial_data || !serial.data[0])
		value |= 1;
	if (!drive->drive.serial.serial_clock || !serial.clock[0])
		value |= 4;
	if (!serial.atn[0]) value |= 0x80;

	switch (drive->drive.serial.deviceid)
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
	if (value!=old) {

		DBG_LOG(2, "vc1541 serial read",("%s %s %s\n",
										 serial.atn[0]?"ATN":"atn",
										 serial.clock[0]?"CLOCK":"clock",
										 serial.data[0]?"DATA":"data"));

		DBG_LOG(2, "vc1541 serial read",("%s %s %s\n",
										 value&0x80?"ATN":"atn",
										 value&4?"CLOCK":"clock",
										 value&1?"DATA":"data"));
		old=value;
	}

	return value;
}

static WRITE8_HANDLER( vc1541_via0_write_portb )
{
	DBG_LOG(2, "vc1541 serial write",("%s %s %s\n",
									 data&0x10?"ATN":"atn",
									 data&8?"CLOCK":"clock",
									 data&2?"DATA":"data"));

	drive->drive.serial.data=data&2?0:1;
	drive->drive.serial.acka=(data&0x10)?1:0;

	if ((!(data & 2)) != drive->drive.serial.serial_data)
	{
		vc1541_serial_data_write (1, drive->drive.serial.serial_data = !(data & 2));
	}

	if ((!(data & 8)) != drive->drive.serial.serial_clock)
	{
		vc1541_serial_clock_write (1, drive->drive.serial.serial_clock = !(data & 8));
	}
	vc1541_serial_atn_write (machine, 1, drive->drive.serial.serial_atn = 1);
}

/*
 * via 6522 at 0x1c00
 * port a
    byte in gcr format from or to floppy

 * port b
 * 0 output steppermotor
 * 1 output steppermotor
     10: 00->01->10->11->00 move head to higher tracks
 * 2 output motor (rotation) (300 revolutions per minute)
 * 3 output led
 * 4 input disk not write protected
 * 5 timer adjustment
 * 6 timer adjustment
 * 4 different speed zones (track dependend)
    frequency select?
    3 slowest
    0 highest
 * 7 input sync signal when reading from disk (more then 9 1 bits)

 * ca1 byte ready input (also m6502 set overflow input)

 * ca2 set overflow enable for 6502
 * ca3 read/write
 *
 * irq to m6502 irq connected
 */
static void vc1541_via1_irq (running_machine *machine, int level)
{
	drive->via1irq = level;
	DBG_LOG(2, "vc1541 via1 irq",("level %d %d\n",drive->via0irq,drive->via1irq));
	cpunum_set_input_line (machine, drive->cpunumber,
					  M6502_IRQ_LINE, drive->via1irq || drive->via0irq);
}

static READ8_HANDLER( vc1541_via1_read_porta )
{
	int data=drive->head.data[drive->image.pos];
	DBG_LOG(2, "vc1541 drive",("port a read %.2x\n", data));
	return data;
}

static WRITE8_HANDLER( vc1541_via1_write_porta )
{
	DBG_LOG(1, "vc1541 drive",("port a write %.2x\n", data));
}

static  READ8_HANDLER( vc1541_via1_read_portb )
{
	UINT8 value = 0xff;

#if 0
	if (WRITEPROTECTED)
		value &= ~0x10;
#endif
	if (drive->head.sync) {
		value&=~0x80;
	}

	return value;
}

static WRITE8_HANDLER( vc1541_via1_write_portb )
{
	static int old=0;
	if (data!=old) {
		DBG_LOG(1, "vc1541 drive",("%.2x\n", data));
		if ((old&3)!=(data&3)) {
			switch (old&3) {
			case 0:
				if ((data&3)==1) drive->track+=0.5;
				else if ((data&3)==3) drive->track-=0.5;
				break;
			case 1:
				if ((data&3)==2) drive->track+=0.5;
				else if ((data&3)==0) drive->track-=0.5;
				break;
			case 2:
				if ((data&3)==3) drive->track+=0.5;
				else if ((data&3)==1) drive->track-=0.5;
				break;
			case 3:
				if ((data&3)==0) drive->track+=0.5;
				else if ((data&3)==2) drive->track-=0.5;
				break;
			}
			if (drive->track<1) drive->track=1.0;
			if (drive->track>35) drive->track=35;
		}
		if ( (drive->motor!=(data&4))||(drive->frequency!=(data&0x60)) )
		{
			double tme;
			drive->motor = data & 4;
			drive->frequency = data & 0x60;
			tme=drive_times[drive->frequency>>5]*8*2;
			if (drive->motor)
			{
				if (attotime_to_double(timer_timeelapsed(drive->timer)) > 1.0e29)
					timer_reset(drive->timer, attotime_never);
				else
					timer_adjust_periodic(drive->timer, attotime_zero, 0, double_to_attotime(tme));
			}
			else
			{
				timer_reset(drive->timer, attotime_never);
			}
		}
		old=data;
	}
	drive->led = data & 8;
}

static const struct via6522_interface via2 =
{
	0,								   /*vc1541_via0_read_porta, */
	vc1541_via0_read_portb,
	0,								   /*via2_read_ca1, */
	0,								   /*via2_read_cb1, */
	0,								   /*via2_read_ca2, */
	0,								   /*via2_read_cb2, */
	0,								   /*via2_write_porta, */
	vc1541_via0_write_portb,
	0,                                 /*via2_write_ca1, */
	0,                                 /*via2_write_cb1, */
	0,								   /*via2_write_ca2, */
	0,								   /*via2_write_cb2, */
	vc1541_via0_irq
}, via3 =
{
	vc1541_via1_read_porta,
	vc1541_via1_read_portb,
	0,								   /*via3_read_ca1, */
	0,								   /*via3_read_cb1, */
	0,								   /*via3_read_ca2, */
	0,								   /*via3_read_cb2, */
	vc1541_via1_write_porta,
	vc1541_via1_write_portb,
	0,                                 /*via3_write_ca1, */
	0,                                 /*via3_write_cb1, */
	0,								   /*via3_write_ca2, */
	0,								   /*via3_write_cb2, */
	vc1541_via1_irq
};


void vc1541_serial_reset_write (int which, int level)
{
}

int vc1541_serial_atn_read (int which)
{
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
	return serial.atn[0];
}

void vc1541_serial_atn_write (running_machine *machine, int which, int level)
{
#if 0
	int value;
#endif
	if (serial.atn[1 + which] != level)
	{
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
		serial.atn[1 + which] = level;
		if (serial.atn[0] != level)
		{
			serial.atn[0] = serial.atn[1] && serial.atn[2];
			if (serial.atn[0] == level)
			{
				DBG_LOG(1, "vc1541",("%d:%.4x atn %s\n",
									 cpu_getactivecpu (),
									 activecpu_get_pc(),
									 serial.atn[0]?"ATN":"atn"));
				via_set_input_ca1 (machine, 2, !level);
#if 0
				value=drive->drive.serial.data;
				if (drive->drive.serial.acka!=!level) value=0;
				if (value!=serial.data[2]) {
					serial.data[2]=value;
					if (serial.data[0]!=value) {
						serial.data[0]=serial.data[1] && serial.data[2];
					}
				}
#endif
			}
		}
	}
}

int vc1541_serial_data_read (int which)
{
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
	return serial.data[0];
}

void vc1541_serial_data_write (int which, int level)
{
	if (serial.data[1 + which] != level)
	{
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
		serial.data[1 + which] = level;
		if (serial.data[0] != level)
		{
			serial.data[0] = serial.data[1] && serial.data[2];
			if (serial.data[0] == level)
			{
				DBG_LOG(1, "vc1541",("%d:%.4x data %s\n",
									 cpu_getactivecpu (),
									 activecpu_get_pc(),
									 serial.data[0]?"DATA":"data"));
			}
		}
	}
}

int vc1541_serial_clock_read (int which)
{
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
	return serial.clock[0];
}

void vc1541_serial_clock_write (int which, int level)
{
	if (serial.clock[1 + which] != level)
	{
#ifdef CPU_SYNC
		if (cpu_getactivecpu()==0) cpu_sync();
#endif
		serial.clock[1 + which] = level;
		if (serial.clock[0] != level)
		{
			serial.clock[0] = serial.clock[1] && serial.clock[2];
			if (serial.clock[0] == level)
			{
				DBG_LOG(1, "vc1541",("%d:%.4x clock %s\n",
									 cpu_getactivecpu (),
									 activecpu_get_pc(),
									 serial.clock[0]?"CLOCK":"clock"));
			}
		}
	}
}

int vc1541_serial_request_read (int which)
{
	return 1;
}

void vc1541_serial_request_write (int which, int level)
{
}


/**************************************

	1570 / 1571 / 1571CR

**************************************/

static ADDRESS_MAP_START( _1571_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_READWRITE(via_2_r, via_2_w)  /* 0 and 1 used in vc20 */
	AM_RANGE(0x1810, 0x189f) AM_READ(SMH_NOP) /* for debugger */
	AM_RANGE(0x1c00, 0x1c0f) AM_READWRITE(via_3_r, via_3_w)
	AM_RANGE(0x1c10, 0x1c9f) AM_READ(SMH_NOP) /* for debugger */
//	AM_RANGE(0x2000, 0x2003) // WD17xx
//	AM_RANGE(0x4000, 0x400f) // CIA
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/**************************************

	1581

**************************************/

static ADDRESS_MAP_START( _1581_map, ADDRESS_SPACE_PROGRAM, 8 )
//	AM_RANGE(0x4000, 0x400f) // CIA
//	AM_RANGE(0x6000, 0x6003) // WD17xx
ADDRESS_MAP_END


/**************************************

	2031 / 2040 / 3040 / 4040 / 
		1001 / 8050 / 8250

**************************************/

static ADDRESS_MAP_START( _2031_map, ADDRESS_SPACE_PROGRAM, 8 )
//	AM_RANGE(0x0200, 0x021f) // RIOT1
//	AM_RANGE(0x0280, 0x029f) // RIOT2
ADDRESS_MAP_END


/**************************************

	1551

**************************************/

/*
  c1551 irq line
  only timing related??? (60 hz?), delivered from c16?
 */
static TIMER_CALLBACK(c1551_irq_timer)
{
	cpunum_set_input_line(machine, drive->cpunumber, M6502_IRQ_LINE, PULSE_LINE);
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
static WRITE8_HANDLER ( c1551_port_w )
{
	static int old=0;
	if (offset) {
		DBG_LOG(1, "c1551 port",("write %.2x\n",data));
		drive->drive.c1551.cpu_port=data;

		if (data!=old) {
			DBG_LOG(1, "vc1541 drive",("%.2x\n", data));
			if ((old&3)!=(data&3)) {
				switch (old&3) {
				case 0:
					if ((data&3)==1) drive->track+=0.5;
					else if ((data&3)==3) drive->track-=0.5;
					break;
				case 1:
					if ((data&3)==2) drive->track+=0.5;
					else if ((data&3)==0) drive->track-=0.5;
					break;
				case 2:
					if ((data&3)==3) drive->track+=0.5;
					else if ((data&3)==1) drive->track-=0.5;
					break;
				case 3:
					if ((data&3)==0) drive->track+=0.5;
					else if ((data&3)==2) drive->track-=0.5;
					break;
				}
				if (drive->track<1) drive->track=1.0;
				if (drive->track>35) drive->track=35;
			}
			if ( (drive->motor!=(data&4))||(drive->frequency!=(data&0x60)) ) {
				double tme;
				drive->motor = data & 4;
				drive->frequency = data & 0x60;
				tme=drive_times[drive->frequency>>5]*8*2;
				if (drive->motor)
				{
					if (attotime_to_double(timer_timeelapsed(drive->timer)) > 1.0e29)
						timer_reset(drive->timer, attotime_never);
					else
						timer_adjust_periodic(drive->timer, attotime_zero, 0, double_to_attotime(tme));
				}
				else
				{
					timer_reset(drive->timer, attotime_never);
				}
			}
			old=data;
		}
		drive->led = data & 8;
	} else {
		drive->drive.c1551.cpu_ddr=data;
		DBG_LOG(1, "c1551 ddr",("write %.2x\n",data));
	}
}

static  READ8_HANDLER ( c1551_port_r )
{
	int data;

	if (offset) {
		data=0x7f;
#if 0
		if (WRITEPROTECTED)
			data &= ~0x10;
#endif
		if (drive->head.ready) {
			data|=0x80;
			drive->head.ready=0;
		}
		data&=~drive->drive.c1551.cpu_ddr;
		data|=drive->drive.c1551.cpu_ddr&drive->drive.c1551.cpu_port;
		DBG_LOG(3, "c1551 port",("read %.2x\n", data));
	} else {
		data=drive->drive.c1551.cpu_ddr;
		DBG_LOG(3, "c1551 ddr",("read %.2x\n", data));
	}
	return data;
}

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
static int c1551_port_c_r(void)
{
	int data=0xff;
	data&=~0x20;
	if (drive->head.sync) data&=~0x40;
	return data;
}

static int c1551_port_b_r (void)
{
	int data=drive->head.data[drive->image.pos];
	DBG_LOG(2, "c1551 drive",("port a read %.2x\n", data));
	return data;
}


static ADDRESS_MAP_START( _1551_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x0001) AM_READWRITE(c1551_port_r, c1551_port_w)
	AM_RANGE(0x0002, 0x07ff) AM_RAM
    AM_RANGE(0x4000, 0x4007) AM_READWRITE(tpi6525_0_port_r, tpi6525_0_port_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


static void c1551x_write_data (running_machine *machine, TPI6525 *This, int data)
{
	DBG_LOG(1, "c1551 cpu", ("%d write data %.2x\n",
						 cpu_getactivecpu (), data));
#ifdef CPU_SYNC
	cpu_sync();
#endif
	tpi6525_0_port_a_w(machine, 0,data);
}

static int c1551x_read_data (running_machine *machine, TPI6525 *This)
{
	int data=0xff;
#ifdef CPU_SYNC
	cpu_sync ();
#endif
	data=tpi6525_0_port_a_r(machine, 0);
	DBG_LOG(2, "c1551 cpu",("%d read data %.2x\n",
						 cpu_getactivecpu (), data));
	return data;
}

static void c1551x_write_handshake (running_machine *machine, TPI6525 *This, int data)
{
	DBG_LOG(1, "c1551 cpu",("%d write handshake %.2x\n",
						 cpu_getactivecpu (), data));
#ifdef CPU_SYNC
	cpu_sync();
#endif
	tpi6525_0_port_c_w(machine, 0,data&0x40?0xff:0x7f);
}

static int c1551x_read_handshake (running_machine *machine, TPI6525 *This)
{
	int data=0xff;
#ifdef CPU_SYNC
	cpu_sync();
#endif
	data=tpi6525_0_port_c_r(machine, 0)&8?0x80:0;
	DBG_LOG(2, "c1551 cpu",("%d read handshake %.2x\n",
						 cpu_getactivecpu (), data));
	return data;
}

static int c1551x_read_status (running_machine *machine, TPI6525 *This)
{
	int data=0xff;
#ifdef CPU_SYNC
	cpu_sync();
#endif
	data=tpi6525_0_port_c_r(machine, 0)&3;
	DBG_LOG(1, "c1551 cpu",("%d read status %.2x\n",
						 cpu_getactivecpu (), data));
	return data;
}

void c1551x_0_write_data (int data)
{
	c1551x_write_data(Machine, tpi6525, data);
}

int c1551x_0_read_data (void)
{
	return c1551x_read_data(Machine, tpi6525);
}

void c1551x_0_write_handshake (int data)
{
	c1551x_write_handshake(Machine, tpi6525, data);
}

int c1551x_0_read_handshake (void)
{
	return c1551x_read_handshake(Machine, tpi6525);
}

int c1551x_0_read_status (void)
{
	return c1551x_read_status(Machine, tpi6525);
}

/**************************************

	Drive init & reset

**************************************/


int drive_config (int type, int id, int mode, int cpunr, int devicenr)
{
	drive->type = type;
	drive->cpunumber = cpunr;
	drive->drive.serial.deviceid = devicenr;
	drive->timer = timer_alloc(drive_timer, NULL);

	if ((drive->type == type_1541) || (drive->type == type_2031)) 
	{
		via_config (2, &via2);
		via_config (3, &via3);
	}

	if (type == type_1551)
	{
		tpi6525[0].c.read=c1551_port_c_r;
		tpi6525[0].b.read=c1551_port_b_r;

		/* time should be small enough to allow quitting of the irq
		line before the next interrupt is triggered */
		drive->drive.c1551.irq_timer = timer_alloc(c1551_irq_timer, NULL);
		timer_adjust_periodic(drive->drive.c1551.irq_timer, attotime_zero, 0, ATTOTIME_IN_HZ(60));
	}

	return 0;
}

void drive_reset (void)
{
	int i;

	if (drive->type == type_1541) 
	{
		for (i = 0; i < sizeof (serial.atn) / sizeof (serial.atn[0]); i++)
		{
			serial.atn[i] = serial.data[i] = serial.clock[i] = 1;
		}
	}

	drive->track = 1.0;

	if ((drive->type == type_1541) || (drive->type == type_2031)) 
	{
		via_reset ();
	}

	if ((drive->type == type_1551)) 
	{
		tpi6525_0_reset();
	}
}

/**************************************

	Machine drivers

**************************************/

MACHINE_DRIVER_START( cpu_vc1540 )
	MDRV_CPU_ADD("cpu_vc1540", M6502, XTAL_16_MHz / 16)
	MDRV_CPU_PROGRAM_MAP(_1541_map, 0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_vc1541 )
	MDRV_IMPORT_FROM(cpu_vc1540)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c2031 )
	MDRV_IMPORT_FROM(cpu_vc1540)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_dolphin )
	MDRV_CPU_ADD("cpu_dolphin", M6502, XTAL_16_MHz / 16)
	MDRV_CPU_PROGRAM_MAP(dolphin_map, 0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c1551 )
	MDRV_CPU_ADD("cpu_c1551", M6510T, 2000000)
	MDRV_CPU_PROGRAM_MAP(_1551_map, 0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cpu_c1571 )
	MDRV_CPU_ADD("cpu_vc1540", M6502, XTAL_16_MHz / 16)
	MDRV_CPU_PROGRAM_MAP(_1571_map, 0)
MACHINE_DRIVER_END

/**************************************

	Device configurations

**************************************/

DEVICE_IMAGE_LOAD(vc1541)
{
	offset_init (format_d64);

	/* Fix me: we should load directly the GCR data */
	drive->image.data = image_ptr(image);
	if (!drive->image.data)
		return INIT_FAIL;

	logerror("floppy image %s loaded\n", image_filename(image));

	return INIT_PASS;
}

DEVICE_IMAGE_UNLOAD(vc1541)
{
	/* writeback of image data */
	drive->image.data = NULL;
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
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(vc1541); break;
		case MESS_DEVINFO_PTR_UNLOAD:				info->unload = DEVICE_IMAGE_UNLOAD_NAME(vc1541); break;
		case MESS_DEVINFO_PTR_DEV_SPECIFIC:			info->f = (genf *) drive_config; break;

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
		case MESS_DEVINFO_PTR_DEV_SPECIFIC:			info->f = (genf *) drive_config; break;

		default: vc1541_device_getinfo(devclass, state, info); break;
	}
}



void c1571_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	vc1541_device_getinfo(devclass, state, info);
}
