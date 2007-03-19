/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine
  (RAM, ROM, interrupts, I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "timer.h"
#include "cpu/z80/z80.h"
#include "video/generic.h"
#include "includes/cgenie.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "image.h"

#define AYWriteReg(chip,port,value) \
	AY8910_control_port_0_w(0,port);  \
	AY8910_write_port_0_w(0,value)

#define TAPE_HEADER "Colour Genie - Virtual Tape File"

UINT8 *cgenie_fontram;


int cgenie_tv_mode = -1;
int cgenie_load_cas = 0;
static int port_ff = 0xff;

#define CGENIE_DRIVE_INFO



/********************************************************
   abbreviations used:
   GPL	Granules Per Lump
   GAT	Granule Allocation Table
   GATL GAT Length
   GATM GAT Mask
   DDGA Disk Directory Granule Allocation
*********************************************************/
typedef struct
{
	UINT8 DDSL; 	 /* Disk Directory Start Lump (lump number of GAT) */
	UINT8 GATL; 	 /* # of bytes used in the Granule Allocation Table sector */
	UINT8 STEPRATE;  /* step rate and somet SD/DD flag ... */
	UINT8 TRK;		 /* number of tracks */
	UINT8 SPT;		 /* sectors per track (both heads counted!) */
	UINT8 GATM; 	 /* number of used bits per byte in the GAT sector (GAT mask) */
	UINT8 P7;		 /* ???? always zero */
	UINT8 FLAGS;	 /* ???? some flags (SS/DS bit 6) */
	UINT8 GPL;		 /* Sectors per granule (always 5 for the Colour Genie) */
	UINT8 DDGA; 	 /* Disk Directory Granule allocation (number of driectory granules) */
}	PDRIVE;

static PDRIVE pd_list[12] = {
	{0x14, 0x28, 0x07, 0x28, 0x0A, 0x02, 0x00, 0x00, 0x05, 0x02}, /* CMD"<0=A" 40 tracks, SS, SD */
	{0x14, 0x28, 0x07, 0x28, 0x14, 0x04, 0x00, 0x40, 0x05, 0x04}, /* CMD"<0=B" 40 tracks, DS, SD */
	{0x18, 0x30, 0x53, 0x28, 0x12, 0x03, 0x00, 0x03, 0x05, 0x03}, /* CMD"<0=C" 40 tracks, SS, DD */
	{0x18, 0x30, 0x53, 0x28, 0x24, 0x06, 0x00, 0x43, 0x05, 0x06}, /* CMD"<0=D" 40 tracks, DS, DD */
	{0x14, 0x28, 0x07, 0x28, 0x0A, 0x02, 0x00, 0x04, 0x05, 0x02}, /* CMD"<0=E" 40 tracks, SS, SD */
	{0x14, 0x28, 0x07, 0x28, 0x14, 0x04, 0x00, 0x44, 0x05, 0x04}, /* CMD"<0=F" 40 tracks, DS, SD */
	{0x18, 0x30, 0x53, 0x28, 0x12, 0x03, 0x00, 0x07, 0x05, 0x03}, /* CMD"<0=G" 40 tracks, SS, DD */
	{0x18, 0x30, 0x53, 0x28, 0x24, 0x06, 0x00, 0x47, 0x05, 0x06}, /* CMD"<0=H" 40 tracks, DS, DD */
	{0x28, 0x50, 0x07, 0x50, 0x0A, 0x02, 0x00, 0x00, 0x05, 0x02}, /* CMD"<0=I" 80 tracks, SS, SD */
	{0x28, 0x50, 0x07, 0x50, 0x14, 0x04, 0x00, 0x40, 0x05, 0x04}, /* CMD"<0=J" 80 tracks, DS, SD */
	{0x30, 0x60, 0x53, 0x50, 0x12, 0x03, 0x00, 0x03, 0x05, 0x03}, /* CMD"<0=K" 80 tracks, SS, DD */
	{0x30, 0x60, 0x53, 0x50, 0x24, 0x06, 0x00, 0x43, 0x05, 0x06}, /* CMD"<0=L" 80 tracks, DS, DD */
};

#define IRQ_TIMER		0x80
#define IRQ_FDC 		0x40
static UINT8 irq_status;

static UINT8 motor_drive;
static UINT8 head;

/* current tape file handles */
static char tape_name[12+1];
static mame_file *tape_get_file;

/* tape buffer for the first eight bytes at write (to extract a filename) */
static UINT8 tape_buffer[9];

/* file offset within tape file */
static int tape_count;

/* number of sync and data bits that were written */
static int put_bit_count;

/* number of sync and data bits to read */
static int get_bit_count;

/* sync and data bits mask */
static int tape_bits;

/* time in cycles for the next bit at read */
static int tape_time;

/* flag if writing to tape detected the sync header A5 already */
static int in_sync;

/* cycle count at last output port change */
static int put_cycles;

/* cycle count at last input port read */
static int get_cycles;

/* a prototype to be called from cgenie_stop_machine */
static void tape_put_close(running_machine *machine);


static OPBASE_HANDLER (opbaseoverride)
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	mess_image *img;
	UINT8 *buff, *s, data;
	UINT16 size, entry = 0, block_len, block_ofs = 0;

	/* check if the BASIC prompt is visible on the screen */
	if( cgenie_load_cas && RAM[0x4400+3*40] == 0x3e )
	{
		cgenie_load_cas = 0;

		img = image_from_devtype_and_index(IO_CASSETTE, 0);

		if (image_exists(img))
		{
			buff = (UINT8*) malloc(65536);
			if( !buff )
			{
				logerror("failed to allocate 64K buff\n");
				return address;
			}

			size = image_fread(img, buff, 65536);
			s = buff;
			if( memcmp(s, TAPE_HEADER, sizeof(TAPE_HEADER)-1) == 0 )
			{
				s = (UINT8*)memchr(s, 26, size);
				if( s )
				{
					*s++ = '\n';
					*s++ = '\0';
					logerror("%s",s);
				}
				size -= s - buff;
			}
			if( s[0] == 0x66 && s[1] == 0x55 && s[8] == 0x3c )
			{
				logerror("image name: [%-6.6s]\n",s+1);
				s += 8;
				size -= 8;
				while( size > 3 )
				{
					data = *s++;
					switch( data )
					{
					case 0x01:		   /* CMD file header */
					case 0x07:		   /* another type of CMD file header */
					case 0x3c:		   /* CAS file header */
						block_len = *s++;
						/* on CMD files size zero means size 256 */
						if( block_len == 0 )
							block_len = 256;
						block_ofs = *s++;
						block_ofs += 256 * *s++;
						if( data != 0x3c )
						{
							block_len -= 2;
							if( block_len == 0 )
								block_len = 256;
						}
						size -= 4;
						logerror("cgenie_cmd_load block ($%02X) %d at $%04X\n", data, block_len, block_ofs);
						while( block_len && size )
						{
							program_write_byte(block_ofs, *s);
							s++;
							block_ofs++;
							block_len--;
							size--;
						}
						if( data == 0x3c )
							s++;
						break;
					case 0x02:
						block_len = *s++;
						size -= 1;
					case 0x78:
						block_ofs = *s++;
						block_ofs += 256 * *s++;
						if( !entry )
							entry = block_ofs;
						logerror( "cgenie_cmd_load entry ($%02X) at $%04X\n", data, entry);
						size -= 3;
						if( size <= 3 )
						{
							logerror("starting program at $%04X\n", block_ofs);
						}
						break;
					default:
						size--;
					}
				}
				activecpu_set_reg(REG_PC, entry);
			}
			free(buff);
		}
		memory_set_opbase_handler(0,NULL);
	}
	return address;
}

static void cgenie_fdc_callback(int);

static void cgenie_machine_reset(running_machine *machine)
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	/* reset the AY8910 to be quiet, since the cgenie BIOS doesn't */
	AYWriteReg(0, 0, 0);
	AYWriteReg(0, 1, 0);
	AYWriteReg(0, 2, 0);
	AYWriteReg(0, 3, 0);
	AYWriteReg(0, 4, 0);
	AYWriteReg(0, 5, 0);
	AYWriteReg(0, 6, 0);
	AYWriteReg(0, 7, 0x3f);
	AYWriteReg(0, 8, 0);
	AYWriteReg(0, 9, 0);
	AYWriteReg(0, 10, 0);

	/* wipe out color RAM */
	memset(&ROM[0x0f000], 0x00, 0x0400);

	/* wipe out font RAM */
	memset(&ROM[0x0f400], 0xff, 0x0400);

	wd179x_init(WD_TYPE_179X,cgenie_fdc_callback);

	if( readinputport(0) & 0x80 )
	{
		logerror("cgenie floppy discs enabled\n");
	}
	else
	{
		logerror("cgenie floppy discs disabled\n");
	}

	/* copy DOS ROM, if enabled or wipe out that memory area */
	if( readinputport(0) & 0x40 )
	{
		if ( readinputport(0) & 0x080 )
		{
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MRA8_ROM);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MWA8_ROM);
			logerror("cgenie DOS enabled\n");
			memcpy(&ROM[0x0c000],&ROM[0x10000], 0x2000);
		}
		else
		{
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MRA8_NOP);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MWA8_NOP);
			logerror("cgenie DOS disabled (no floppy image given)\n");
		}
	}
	else
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MWA8_NOP);
		logerror("cgenie DOS disabled\n");
		memset(&memory_region(REGION_CPU1)[0x0c000], 0x00, 0x2000);
	}

	/* copy EXT ROM, if enabled or wipe out that memory area */
	if( readinputport(0) & 0x20 )
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xefff, 0, 0, MRA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xefff, 0, 0, MWA8_ROM);
		logerror("cgenie EXT enabled\n");
		memcpy(&memory_region(REGION_CPU1)[0x0e000],
			   &memory_region(REGION_CPU1)[0x12000], 0x1000);
	}
	else
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xefff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xefff, 0, 0, MWA8_NOP);
		logerror("cgenie EXT disabled\n");
		memset(&memory_region(REGION_CPU1)[0x0e000], 0x00, 0x1000);
	}

	cgenie_load_cas = 1;
	memory_set_opbase_handler(0, opbaseoverride);
}

MACHINE_START( cgenie )
{
	UINT8 *gfx = memory_region(REGION_GFX2);
	int i;

	/* initialize static variables */
	memset(tape_buffer, 0, sizeof(tape_buffer));
	memset(tape_name, 0, sizeof(tape_name));
	tape_get_file = NULL;
	irq_status = 0;
	motor_drive = 0;
	head = 0;
	tape_count = 0;
	put_bit_count = 0;
	get_bit_count = 0;
	tape_bits = 0;
	tape_time = 0;
	in_sync = 0;
	put_cycles = 0;
	get_cycles = 0;

	/*
	 * Every fifth cycle is a wait cycle, so I reduced
	 * the overlocking by one fitfth
	 */
	cpunum_set_clockscale(0, 0.80);

	/* Initialize some patterns to be displayed in graphics mode */
	for( i = 0; i < 256; i++ )
		memset(gfx + i * 8, i, 8);

	/* set up RAM */
	memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1, 0, 0, cgenie_videoram_w);
	videoram = mess_ram;
	memory_set_bankptr(1, mess_ram);

	add_reset_callback(machine, cgenie_machine_reset);
	add_exit_callback(machine, tape_put_close);
	return 0;
}


DEVICE_LOAD( cgenie_cassette )
{
	return INIT_PASS;
}

/* basic-dsk is a disk image format which has the tracks and sectors
 * stored in order, no information is stored which details the number
 * of tracks, number of sides, number of sectors etc, so we need to
 * set that up here
 */
DEVICE_LOAD( cgenie_floppy )
{
	int i, j, dir_offset;
	UINT8 buff[16];
	UINT8 tracks = 0;
	UINT8 heads = 0;
	UINT8 spt = 0;
	short dir_sector = 0;
	short dir_length = 0;

	/* A Floppy Isnt manditory, so return if none */
	if (device_load_basicdsk_floppy(image) != INIT_PASS)
		return INIT_FAIL;

	/* determine image geometry */
	image_fseek(image, 0, SEEK_SET);

	/* determine geometry from disk contents */
	for( i = 0; i < 12; i++ )
	{
		image_fseek(image, pd_list[i].SPT * 256, SEEK_SET);
		image_fread(image, buff, 16);
		/* find an entry with matching DDSL */
		if (buff[0] != 0x00 || buff[1] != 0xfe || buff[2] != pd_list[i].DDSL)
			continue;
		logerror("cgenie: checking format #%d\n", i);

		dir_sector = pd_list[i].DDSL * pd_list[i].GATM * pd_list[i].GPL + pd_list[i].SPT;
		dir_length = pd_list[i].DDGA * pd_list[i].GPL;

		/* scan directory for DIR/SYS or NCW1983/JHL files */
		/* look into sector 2 and 3 first entry relative to DDSL */
		for( j = 16; j < 32; j += 8 )
		{
			dir_offset = dir_sector * 256 + j * 32;
			if( image_fseek(image, dir_offset, SEEK_SET) < 0 )
				break;
			if( image_fread(image, buff, 16) != 16 )
				break;
			if( !strncmp((char*)buff + 5, "DIR     SYS", 11) ||
				!strncmp((char*)buff + 5, "NCW1983 JHL", 11) )
			{
				tracks = pd_list[i].TRK;
				heads = (pd_list[i].SPT > 18) ? 2 : 1;
				spt = pd_list[i].SPT / heads;
				dir_sector = pd_list[i].DDSL * pd_list[i].GATM * pd_list[i].GPL + pd_list[i].SPT;
				dir_length = pd_list[i].DDGA * pd_list[i].GPL;
				memcpy(memory_region(REGION_CPU1) + 0x5A71 + image_index_in_device(image) * sizeof(PDRIVE), &pd_list[i], sizeof(PDRIVE));
				break;
			}
		}

		logerror("cgenie: geometry %d tracks, %d heads, %d sec/track\n", tracks, heads, spt);
		/* set geometry so disk image can be read */
		basicdsk_set_geometry(image, tracks, heads, spt, 256, 0, 0, FALSE);

		logerror("cgenie: directory sectors %d - %d (%d sectors)\n", dir_sector, dir_sector + dir_length - 1, dir_length);
		/* mark directory sectors with deleted data address mark */
		/* assumption dir_sector is a sector offset */
		for (j = 0; j < dir_length; j++)
		{
			UINT8 track;
			UINT8 side;
			UINT8 sector_id;
			UINT16 track_offset;
			UINT16 sector_offset;

			/* calc sector offset */
			sector_offset = dir_sector + j;

			/* get track offset */
			track_offset = sector_offset / spt;

			/* calc track */
			track = track_offset / heads;

			/* calc side */
			side = track_offset % heads;

			/* calc sector id - first sector id is 0! */
			sector_id = sector_offset % spt;

			/* set deleted data address mark for sector specified */
			basicdsk_set_ddam(image, track, side, sector_id, 1);
		}

	}
	return INIT_PASS;
}

/*************************************
 *
 *				Tape emulation.
 *
 *************************************/

/*******************************************************************
 * tape_put_byte
 * write next data byte to virtual tape. After collecting the first
 * nine bytes try to extract the kind of data and filename.
 *******************************************************************/
static void tape_put_byte(UINT8 value)
{
	mame_file *file;

	if( tape_count < 9 )
	{
		tape_name[0] = '\0';
		tape_buffer[tape_count++] = value;
		if( tape_count == 9 )
		{
			/* BASIC tape ? */
			if( tape_buffer[1] != 0x55 || tape_buffer[8] != 0x3c )
				sprintf(tape_name, "basic%c.cas", tape_buffer[1]);
			else
			/* SYSTEM tape ? */
			if( tape_buffer[1] == 0x55 && tape_buffer[8] == 0x3c )
				sprintf(tape_name, "%-6.6s.cas", tape_buffer + 2);
			else
				strcpy(tape_name, "unknown.cas");
			mame_fopen(SEARCHPATH_IMAGE, tape_name, OPEN_FLAG_WRITE, &file);
		}
	}
	else
	{
		tape_count++;
	}
}

/*******************************************************************
 * tape_put_close
 * eventuall flush output buffer and close an open
 * virtual tape output file.
 *******************************************************************/
static void tape_put_close(running_machine *machine)
{
	tape_count = 0;
}

/*******************************************************************
 * tape_put_bit
 * port FF tape status bit changed. Figure out what to do with it ;-)
 *******************************************************************/
static void tape_put_bit(void)
{
	int now_cycles = activecpu_gettotalcycles();
	int diff = now_cycles - put_cycles;
	int limit = 12 * (memory_region(REGION_CPU1)[0x4310] + memory_region(REGION_CPU1)[0x4311]);
	UINT8 value;

	/* overrun since last write ? */
	if( diff > 4000 )
	{
		/* reset tape output */
		tape_put_close(Machine);
		put_bit_count = tape_bits = in_sync = 0;
	}
	else
	{
		/* change within time for a 1 bit ? */
		if( diff < limit )
		{
			tape_bits = (tape_bits << 1) | 1;
			switch( in_sync )
			{
				case 0: 	   /* look for sync AA */
					if( (tape_bits & 0xffff) == 0xcccc )
					{
						popmessage("Tape sync1 written");
						in_sync = 1;
					}
					break;
				case 1: 	   /* look for sync 66 */
					if( (tape_bits & 0xffff) == 0x3c3c )
					{
						in_sync = 2;
						popmessage("Tape sync2 written");
						put_bit_count = 16;
					}
					break;
				case 2: 	   /* count 1 bit */
					put_bit_count += 1;
			}
		}
		/* no change within time indicates a 0 bit */
		else
		{
			tape_bits <<= 2;
			switch (in_sync)
			{
				case 0: 	   /* look for sync AA */
					if( (tape_bits & 0xffff) == 0xcccc )
					{
						popmessage("Tape sync1 written");
						in_sync = 1;
					}
					break;
				case 1: 	   /* look for sync 66 */
					if( (tape_bits & 0xffff) == 0x3c3c )
					{
						popmessage("Tape sync2 written");
						in_sync = 2;
						put_bit_count = 16;
					}
					break;
				case 2: 	   /* count 2 bits */
					put_bit_count += 2;
			}
		}

		logerror("%4d %4d %d bits %04X\n", diff, limit, in_sync, tape_bits & 0xffff);

		/* collected 8 sync plus 8 data bits ? */
		if( put_bit_count >= 16 )
		{
			/* extract data bits to value */
			value = 0;
			if( tape_bits & 0x8000 )
				value |= 0x80;
			if( tape_bits & 0x2000 )
				value |= 0x40;
			if( tape_bits & 0x0800 )
				value |= 0x20;
			if( tape_bits & 0x0200 )
				value |= 0x10;
			if( tape_bits & 0x0080 )
				value |= 0x08;
			if( tape_bits & 0x0020 )
				value |= 0x04;
			if( tape_bits & 0x0008 )
				value |= 0x02;
			if( tape_bits & 0x0002 )
				value |= 0x01;
			put_bit_count -= 16;
			tape_bits = 0;
			tape_put_byte(value);
		}
	}
/* remember the cycle count of this write */
	put_cycles = now_cycles;
}

/*******************************************************************
 * tape_get_byte
 * read next byte from input tape image file.
 * the first 32 bytes are faked to be sync header AA.
 *******************************************************************/
static void tape_get_byte(void)
{
	UINT8 value;

	if( tape_get_file )
	{
		if( tape_count < 32 )
		{
			popmessage("Tape load sync1");
			value = 0xaa;
		}
		else
		{
			popmessage("Tape load '%s' $%04X bytes", tape_name, tape_count);
			mame_fread(tape_get_file, &value, 1);
		}
		tape_bits |= 0xaaaa;
		if( value & 0x80 )
			tape_bits ^= 0x4000;
		if( value & 0x40 )
			tape_bits ^= 0x1000;
		if( value & 0x20 )
			tape_bits ^= 0x0400;
		if( value & 0x10 )
			tape_bits ^= 0x0100;
		if( value & 0x08 )
			tape_bits ^= 0x0040;
		if( value & 0x04 )
			tape_bits ^= 0x0010;
		if( value & 0x02 )
			tape_bits ^= 0x0004;
		if( value & 0x01 )
			tape_bits ^= 0x0001;
		get_bit_count = 16;
		tape_count++;
	}
}

/*******************************************************************
 * tape_get_open
 * Open a virtual tape image file for input. Look for a special
 * header and skip leading description, if present.
 * The filename is taken from BASIC input buffer at 41E8 ff.
 *******************************************************************/
static void tape_get_open(void)
{
	if( !tape_get_file )
	{
		char buffer[sizeof(TAPE_HEADER)];
		UINT8 *ram = memory_region(REGION_CPU1);
		char *p;

		sprintf(tape_name, "%-6.6s", ram + 0x41e8);
		p = strchr(tape_name, ' ');
		if( p ) *p = '\0';
		strcat(tape_name, ".cas");
		if (tape_name[0] != ' ')
		{
			logerror("tape_get_open '%s'\n", tape_name);
			mame_fopen(SEARCHPATH_IMAGE, tape_name, OPEN_FLAG_READ, &tape_get_file);
		}
		if( tape_get_file )
		{
			popmessage("Tape load '%s'", tape_name);
			mame_fread(tape_get_file, buffer, sizeof(TAPE_HEADER));
			if( strncmp(buffer, TAPE_HEADER, sizeof(TAPE_HEADER) - 1) == 0 )
			{
				UINT8 data;
				/* skip data until zero byte */
				do
				{
					mame_fread(tape_get_file, &data, 1);
				} while( data );
			}
			else
			{
				/* seek back to start of tape */
				mame_fseek(tape_get_file, 0, SEEK_SET);
			}
		}
		tape_count = 0;
	}
}

static void tape_get_bit(void)
{
	int now_cycles = activecpu_gettotalcycles();
	int limit = 10 * memory_region(REGION_CPU1)[0x4312];
	int diff = now_cycles - get_cycles;

	/* overrun since last read ? */
	if( diff >= 4000 )
	{
		if( tape_get_file )
		{
			mame_fclose(tape_get_file);
			tape_get_file = NULL;
			popmessage("Tape file closed");
		}
		get_bit_count = tape_bits = tape_time = 0;
	}
	else
	/* check what he will get for input */
	{
		/* count down cycles */
		tape_time -= diff;

		/* time for the next sync or data bit ? */
		if( tape_time <= 0 )
		{
			/* approx time for a bit */
			tape_time += limit;
			/* need to read get new data ? */
			if( --get_bit_count <= 0 )
			{
				tape_get_open();
				tape_get_byte();
			}
			/* shift next sync or data bit to bit 16 */
			tape_bits <<= 1;
			if( tape_bits & 0x10000 )
				port_ff ^= 1;
		}
	}
	/* remember the cycle count of this read */
	get_cycles = now_cycles;
}

/*************************************
 *
 *				Port handlers.
 *
 *************************************/

/* used bits on port FF */
#define FF_CAS	0x01		   /* tape output signal */
#define FF_BGD0 0x04		   /* background color enable */
#define FF_CHR1 0x08		   /* charset 0xc0 - 0xff 1:fixed 0:defined */
#define FF_CHR0 0x10		   /* charset 0x80 - 0xbf 1:fixed 0:defined */
#define FF_CHR	(FF_CHR0 | FF_CHR1)
#define FF_FGR	0x20		   /* 1: "hi" resolution graphics, 0: text mode */
#define FF_BGD1 0x40		   /* background color select 1 */
#define FF_BGD2 0x80		   /* background color select 2 */
#define FF_BGD	(FF_BGD0 | FF_BGD1 | FF_BGD2)

WRITE8_HANDLER( cgenie_port_ff_w )
{
	int port_ff_changed = port_ff ^ data;

	if( port_ff_changed & FF_CAS )	/* casette port changed ? */
	{
		/* virtual tape ? */
		if( readinputport(0) & 0x08 )
			tape_put_bit();
		else
			DAC_data_w(0,(data & FF_CAS) ? 127:0 );
	}

	/* background bits changed ? */
	if( port_ff_changed & FF_BGD )
	{
		unsigned char r, g, b;

		if( data & FF_BGD0 )
		{
			r = 112;
			g = 0;
			b = 112;
		}
		else
		{
			if( cgenie_tv_mode == 0 )
			{
				switch( data & (FF_BGD1 + FF_BGD2) )
				{
				case FF_BGD1:
					r = 112;
					g = 40;
					b = 32;
					break;
				case FF_BGD2:
					r = 40;
					g = 112;
					b = 32;
					break;
				case FF_BGD1 + FF_BGD2:
					r = 72;
					g = 72;
					b = 72;
					break;
				default:
					r = 0;
					g = 0;
					b = 0;
					break;
				}
			}
			else
			{
				r = 15;
				g = 15;
				b = 15;
			}
		}
		palette_set_color(Machine, 0, r, g, b);
	}

	/* character mode changed ? */
	if( port_ff_changed & FF_CHR )
	{
		cgenie_font_offset[2] = (data & FF_CHR0) ? 0x00 : 0x80;
		cgenie_font_offset[3] = (data & FF_CHR1) ? 0x00 : 0x80;
		if( (port_ff_changed & FF_CHR) == FF_CHR )
			cgenie_invalidate_range(0x80, 0xff);
		else
		if( (port_ff_changed & FF_CHR) == FF_CHR0 )
			cgenie_invalidate_range(0x80, 0xbf);
		else
			cgenie_invalidate_range(0xc0, 0xff);
	}

	/* graphics mode changed ? */
	if( port_ff_changed & FF_FGR )
	{
		cgenie_invalidate_range(0x00, 0xff);
		cgenie_mode_select(data & FF_FGR);
	}

	port_ff = data;
}

 READ8_HANDLER( cgenie_port_ff_r )
{
	/* virtual tape ? */

	if( readinputport(0) & 0x08 )
		tape_get_bit();

	return port_ff;
}

int cgenie_port_xx_r( int offset )
{
	return 0xff;
}

/*************************************
 *									 *
 *		Memory handlers 			 *
 *									 *
 *************************************/

static UINT8 psg_a_out = 0x00;
static UINT8 psg_b_out = 0x00;
static UINT8 psg_a_inp = 0x00;
static UINT8 psg_b_inp = 0x00;

 READ8_HANDLER( cgenie_psg_port_a_r )
{
	return psg_a_inp;
}

UINT8 cgenie_psg_port_b_r(offs_t port)
{
	if( psg_a_out < 0xd0 )
	{
		/* comparator value */
		psg_b_inp = 0x00;
		if( input_port_9_r(0) > psg_a_out )
			psg_b_inp |= 0x80;
		if( input_port_10_r(0) > psg_a_out )
			psg_b_inp |= 0x40;
		if( input_port_11_r(0) > psg_a_out )
			psg_b_inp |= 0x20;
		if( input_port_12_r(0) > psg_a_out )
			psg_b_inp |= 0x10;
	}
	else
	{
		/* read keypad matrix */
		psg_b_inp = 0xFF;
		if( !(psg_a_out & 0x01) )
			psg_b_inp &= ~(input_port_13_r(0) & 15);
		if( !(psg_a_out & 0x02) )
			psg_b_inp &= ~(input_port_13_r(0) / 16);
		if( !(psg_a_out & 0x04) )
			psg_b_inp &= ~(input_port_14_r(0) & 15);
		if( !(psg_a_out & 0x08) )
			psg_b_inp &= ~(input_port_14_r(0) / 16);
		if( !(psg_a_out & 0x10) )
			psg_b_inp &= ~(input_port_15_r(0) & 15);
		if( !(psg_a_out & 0x20) )
			psg_b_inp &= ~(input_port_15_r(0) / 16);
	}
	return psg_b_inp;
}

WRITE8_HANDLER( cgenie_psg_port_a_w )
{
	psg_a_out = data;
}

WRITE8_HANDLER( cgenie_psg_port_b_w )
{
	psg_b_out = data;
}

 READ8_HANDLER( cgenie_status_r )
{
	/* If the floppy isn't emulated, return 0 */
	if( (readinputport(0) & 0x80) == 0 )
		return 0;
	return wd179x_status_r(offset);
}

 READ8_HANDLER( cgenie_track_r )
{
	/* If the floppy isn't emulated, return 0xff */
	if( (readinputport(0) & 0x80) == 0 )
		return 0xff;
	return wd179x_track_r(offset);
}

 READ8_HANDLER( cgenie_sector_r )
{
	/* If the floppy isn't emulated, return 0xff */
	if( (readinputport(0) & 0x80) == 0 )
		return 0xff;
	return wd179x_sector_r(offset);
}

 READ8_HANDLER(cgenie_data_r )
{
	/* If the floppy isn't emulated, return 0xff */
	if( (readinputport(0) & 0x80) == 0 )
		return 0xff;
	return wd179x_data_r(offset);
}

WRITE8_HANDLER( cgenie_command_w )
{
	/* If the floppy isn't emulated, return immediately */
	if( (readinputport(0) & 0x80) == 0 )
		return;
	wd179x_command_w(offset, data);
}

WRITE8_HANDLER( cgenie_track_w )
{
	/* If the floppy isn't emulated, ignore the write */
	if( (readinputport(0) & 0x80) == 0 )
		return;
	wd179x_track_w(offset, data);
}

WRITE8_HANDLER( cgenie_sector_w )
{
	/* If the floppy isn't emulated, ignore the write */
	if( (readinputport(0) & 0x80) == 0 )
		return;
	wd179x_sector_w(offset, data);
}

WRITE8_HANDLER( cgenie_data_w )
{
	/* If the floppy isn't emulated, ignore the write */
	if( (readinputport(0) & 0x80) == 0 )
		return;
	wd179x_data_w(offset, data);
}

 READ8_HANDLER( cgenie_irq_status_r )
{
int result = irq_status;

	irq_status &= ~(IRQ_TIMER | IRQ_FDC);
	return result;
}

INTERRUPT_GEN( cgenie_timer_interrupt )
{
	if( (irq_status & IRQ_TIMER) == 0 )
	{
		irq_status |= IRQ_TIMER;
		cpunum_set_input_line(0, 0, HOLD_LINE);
	}
}

static INTERRUPT_GEN( cgenie_fdc_interrupt )
{
	if( (irq_status & IRQ_FDC) == 0 )
	{
		irq_status |= IRQ_FDC;
		cpunum_set_input_line(0, 0, HOLD_LINE);
	}
}

void cgenie_fdc_callback(int event)
{
	/* if disc hardware is not enabled, do not cause an int */
	if (!( readinputport(0) & 0x80 ))
		return;

	switch( event )
	{
	case WD179X_IRQ_CLR:
		irq_status &= ~IRQ_FDC;
		break;
	case WD179X_IRQ_SET:
		cgenie_fdc_interrupt();
		break;
	}
}

WRITE8_HANDLER( cgenie_motor_w )
{
	UINT8 drive = 255;

	logerror("cgenie motor_w $%02X\n", data);

	if( data & 1 )
		drive = 0;
	if( data & 2 )
		drive = 1;
	if( data & 4 )
		drive = 2;
	if( data & 8 )
		drive = 3;

	if( drive > 3 )
		return;

	/* mask head select bit */
		head = (data >> 4) & 1;

	/* currently selected drive */
	motor_drive = drive;

	wd179x_set_drive(drive);
	wd179x_set_side(head);
}

/*************************************
 *		Keyboard					 *
 *************************************/
 READ8_HANDLER( cgenie_keyboard_r )
{
	int result = 0;

	if( offset & 0x01 )
		result |= input_port_1_r(0);
	if( offset & 0x02 )
		result |= input_port_2_r(0);
	if( offset & 0x04 )
		result |= input_port_3_r(0);
	if( offset & 0x08 )
		result |= input_port_4_r(0);
	if( offset & 0x10 )
		result |= input_port_5_r(0);
	if( offset & 0x20 )
		result |= input_port_6_r(0);
	if( offset & 0x40 )
		result |= input_port_7_r(0);
	if( offset & 0x80 )
		result |= input_port_8_r(0);

	return result;
}

/*************************************
 *		Video RAM					 *
 *************************************/

int cgenie_videoram_r( int offset )
{
	return videoram[offset];
}

WRITE8_HANDLER( cgenie_videoram_w )
{
	/* write to video RAM */
	if( data == videoram[offset] )
		return; 			   /* no change */
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}

 READ8_HANDLER( cgenie_colorram_r )
{
	return colorram[offset] | 0xf0;
}

WRITE8_HANDLER( cgenie_colorram_w )
{
	int a;

	/* only bits 0 to 3 */
	data &= 15;
	/* nothing changed ? */
	if( data == colorram[offset] )
		return;

/* set new value */
	colorram[offset] = data;
/* make offset relative to video frame buffer offset */
	offset = (offset + (cgenie_get_register(12) << 8) + cgenie_get_register(13)) & 0x3ff;
/* mark every 1k of the frame buffer dirty */
	for( a = offset; a < 0x4000; a += 0x400 )
		dirtybuffer[a] = 1;
}

 READ8_HANDLER( cgenie_fontram_r )
{
	return cgenie_fontram[offset];
}

WRITE8_HANDLER( cgenie_fontram_w )
{
	UINT8 *dp;
	int code;

	if( data == cgenie_fontram[offset] )
		return; 			   /* no change */

	/* store data */
	cgenie_fontram[offset] = data;

	/* convert eight pixels */
	dp = &Machine->gfx[0]->gfxdata[(256 * 8 + offset) * Machine->gfx[0]->width];
	dp[0] = (data & 0x80) ? 1 : 0;
	dp[1] = (data & 0x40) ? 1 : 0;
	dp[2] = (data & 0x20) ? 1 : 0;
	dp[3] = (data & 0x10) ? 1 : 0;
	dp[4] = (data & 0x08) ? 1 : 0;
	dp[5] = (data & 0x04) ? 1 : 0;
	dp[6] = (data & 0x02) ? 1 : 0;
	dp[7] = (data & 0x01) ? 1 : 0;

	/* invalidate related character */
	code = 0x80 + offset / 8;
	cgenie_invalidate_range(code, code);
}

/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

INTERRUPT_GEN( cgenie_frame_interrupt )
{
	if( cgenie_tv_mode != (readinputport(0) & 0x10) )
	{
		cgenie_tv_mode = input_port_0_r(0) & 0x10;
		memset(dirtybuffer, 1, videoram_size);
		/* force setting of background color */
		port_ff ^= FF_BGD0;
		cgenie_port_ff_w(0, port_ff ^ FF_BGD0);
	}
}

