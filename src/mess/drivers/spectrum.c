/***************************************************************************
		Spectrum/Inves/TK90X etc. memory map:

	CPU:
		0000-3fff ROM
		4000-ffff RAM

		Spectrum 128/+2/+2a/+3 memory map:

		CPU:
				0000-3fff Banked ROM/RAM (banked rom only on 128/+2)
				4000-7fff Banked RAM
				8000-bfff Banked RAM
				c000-ffff Banked RAM

		TS2068 memory map: (Can't have both EXROM and DOCK active)
		The 8K EXROM can be loaded into multiple pages.

	CPU:
				0000-1fff	  ROM / EXROM / DOCK (Cartridge)
				2000-3fff	  ROM / EXROM / DOCK
				4000-5fff \
				6000-7fff  \
				8000-9fff  |- RAM / EXROM / DOCK
				a000-bfff  |
				c000-dfff  /
				e000-ffff /


Interrupts:

Changes:

29/1/2000	KT -	Implemented initial +3 emulation.
30/1/2000	KT -	Improved input port decoding for reading and therefore
			correct keyboard handling for Spectrum and +3.
31/1/2000	KT -	Implemented buzzer sound for Spectrum and +3.
			Implementation copied from Paul Daniel's Jupiter driver.
			Fixed screen display problems with dirty chars.
			Added support to load .Z80 snapshots. 48k support so far.
13/2/2000	KT -	Added Interface II, Kempston, Fuller and Mikrogen
			joystick support.
17/2/2000	DJR -	Added full key descriptions and Spectrum+ keys.
			Fixed Spectrum +3 keyboard problems.
17/2/2000	KT -	Added tape loading from WAV/Changed from DAC to generic
			speaker code.
18/2/2000	KT -	Added tape saving to WAV.
27/2/2000	KT -	Took DJR's changes and added my changes.
27/2/2000	KT -	Added disk image support to Spectrum +3 driver.
27/2/2000	KT -	Added joystick I/O code to the Spectrum +3 I/O handler.
14/3/2000	DJR -	Tape handling dipswitch.
26/3/2000	DJR -	Snapshot files are now classifed as snapshots not
			cartridges.
04/4/2000	DJR -	Spectrum 128 / +2 Support.
13/4/2000	DJR -	+4 Support (unofficial 48K hack).
13/4/2000	DJR -	+2a Support (rom also used in +3 models).
13/4/2000	DJR -	TK90X, TK95 and Inves support (48K clones).
21/4/2000	DJR -	TS2068 and TC2048 support (TC2048 Supports extra video
			modes but doesn't have bank switching or sound chip).
09/5/2000	DJR -	Spectrum +2 (France, Spain), +3 (Spain).
17/5/2000	DJR -	Dipswitch to enable/disable disk drives on +3 and clones.
27/6/2000	DJR -	Changed 128K/+3 port decoding (sound now works in Zub 128K).
06/8/2000	DJR -	Fixed +3 Floppy support
10/2/2001	KT  -	Re-arranged code and split into each model emulated.
			Code is split into 48k, 128k, +3, tc2048 and ts2048
			segments. 128k uses some of the functions in 48k, +3
			uses some functions in 128, and tc2048/ts2048 use some
			of the functions in 48k. The code has been arranged so
			these functions come in some kind of "override" order,
			read functions changed to use  READ8_HANDLER and write
			functions changed to use WRITE8_HANDLER.
			Added Scorpion256 preliminary.
18/6/2001	DJR -	Added support for Interface 2 cartridges.
xx/xx/2001	KS -	TS-2068 sound fixed.
			Added support for DOCK cartridges for TS-2068.
			Added Spectrum 48k Psycho modified rom driver.
			Added UK-2086 driver.
23/12/2001	KS -	48k machines are now able to run code in screen memory.
				Programs which keep their code in screen memory
				like monitors, tape copiers, decrunchers, etc.
				works now.
		     	Fixed problem with interrupt vector set to 0xffff (much
			more 128k games works now).
				A useful used trick on the Spectrum is to set
				interrupt vector to 0xffff (using the table 
				which contain 0xff's) and put a byte 0x18 hex,
				the opcode for JR, at this address. The first
				byte of the ROM is a 0xf3 (DI), so the JR will
				jump to 0xfff4, where a long JP to the actual
				interrupt routine is put. Due to unideal
				bankswitching in MAME this JP were to 0001 what
				causes Spectrum to reset. Fixing this problem
				made much more software runing (i.e. Paperboy).
			Corrected frames per second value for 48k and 128k
			Sincalir machines.
				There are 50.08 frames per second for Spectrum
				48k what gives 69888 cycles for each frame and
				50.021 for Spectrum 128/+2/+2A/+3 what gives
				70908 cycles for each frame. 
			Remaped some Spectrum+ keys.
				Presing F3 to reset was seting 0xf7 on keyboard
				input port. Problem occured for snapshots of
				some programms where it was readed as pressing
				key 4 (which is exit in Tapecopy by R. Dannhoefer
				for example).
			Added support to load .SP snapshots.
			Added .BLK tape images support.
				.BLK files are identical to .TAP ones, extension
				is an only difference.
08/03/2002	KS -	#FF port emulation added.
				Arkanoid works now, but is not playable due to
				completly messed timings.

Initialisation values used when determining which model is being emulated:
 48K		Spectrum doesn't use either port.
 128K/+2	Bank switches with port 7ffd only.
 +3/+2a		Bank switches with both ports.

Notes:
 1. No contented memory.
 2. No hi-res colour effects (need contended memory first for accurate timing).
 3. Multiface 1 and Interface 1 not supported.
 4. Horace and the Spiders cartridge doesn't run properly.
 5. Tape images not supported:
    .TZX, .SPC, .ITM, .PAN, .TAP(Warajevo), .VOC, .ZXS.
 6. Snapshot images not supported:
    .ACH, .PRG, .RAW, .SEM, .SIT, .SNX, .ZX, .ZXS, .ZX82.
 7. 128K emulation is not perfect - the 128K machines crash and hang while
    running quite a lot of games.
 8. Disk errors occur on some +3 games.
 9. Video hardware of all machines is timed incorrectly.
10. EXROM and HOME cartridges are not emulated.
11. The TK90X and TK95 roms output 0 to port #df on start up.
12. The purpose of this port is unknown (probably display mode as TS2068) and
    thus is not emulated.

Very detailed infos about the ZX Spectrum +3e can be found at

http://www.z88forever.org.uk/zxplus3e/

*******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"
#include "formats/tzx_cas.h"

/* +3 hardware */
#include "machine/nec765.h"
#include "cpuintrf.h"
#include "devices/dsk.h"


extern void spectrum_128_update_memory(void);
extern void spectrum_plus3_update_memory(void);


static struct AY8910interface spectrum_ay_interface =
{
	NULL
};


/****************************************************************************************************/
/* Spectrum 48k functions */

/*
 bit 7-5: not used
 bit 4: Ear output/Speaker
 bit 3: MIC/Tape Output
 bit 2-0: border colour
*/

static int		motor_toggle_previous = 0;
static int		cassette_motor_mode = 0;

int PreviousFE = 0;

static WRITE8_HANDLER(spectrum_port_fe_w)
{
	unsigned char Changed;

	Changed = PreviousFE^data;

	/* border colour changed? */
	if ((Changed & 0x07)!=0)
	{
		/* yes - send event */
		EventList_AddItemOffset(0x0fe, data & 0x07, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
	}

	if ((Changed & (1<<4))!=0)
	{
		/* DAC output state */
		speaker_level_w(0,(data>>4) & 0x01);
	}

	if ((Changed & (1<<3))!=0)
	{
		/* write cassette data */
		cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & (1<<3)) ? -1.0 : +1.0);
	}

	PreviousFE = data;
}




static ADDRESS_MAP_START (spectrum_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x57ff) AM_READWRITE( spectrum_characterram_r, spectrum_characterram_w )
	AM_RANGE(0x5800, 0x5aff) AM_READWRITE( spectrum_colorram_r, spectrum_colorram_w )
	AM_RANGE(0x5b00, 0xffff) AM_RAM
ADDRESS_MAP_END

/* KT: more accurate keyboard reading */
/* DJR: Spectrum+ keys added */
static  READ8_HANDLER(spectrum_port_fe_r)
{
   int lines = offset>>8;
   int data = 0xff;

   int cs_extra1 = readinputport(8)  & 0x1f;
   int cs_extra2 = readinputport(9)  & 0x1f;
   int cs_extra3 = readinputport(10) & 0x1f;
   int ss_extra1 = readinputport(11) & 0x1f;
   int ss_extra2 = readinputport(12) & 0x1f;

	if ( readinputport(17) & 0x01 ) {
		if ( motor_toggle_previous == 0 ) {
			cassette_motor_mode = cassette_motor_mode ^ 0x01;
			cassette_change_state( image_from_devtype_and_index( IO_CASSETTE, 0 ), cassette_motor_mode ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR );
		}
		motor_toggle_previous = 1;
	} else {
		motor_toggle_previous = 0;
	}

   /* Caps - V */
   if ((lines & 1)==0)
   {
		data &= readinputport(0);
		/* CAPS for extra keys */
		if (cs_extra1 != 0x1f || cs_extra2 != 0x1f || cs_extra3 != 0x1f)
			data &= ~0x01;
   }

   /* A - G */
   if ((lines & 2)==0)
		data &= readinputport(1);

   /* Q - T */
   if ((lines & 4)==0)
		data &= readinputport(2);

   /* 1 - 5 */
   if ((lines & 8)==0)
		data &= readinputport(3) & cs_extra1;

   /* 6 - 0 */
   if ((lines & 16)==0)
		data &= readinputport(4) & cs_extra2;

   /* Y - P */
   if ((lines & 32)==0)
		data &= readinputport(5) & ss_extra1;

   /* H - Enter */
   if ((lines & 64)==0)
		data &= readinputport(6);

	/* B - Space */
	if ((lines & 128)==0)
	{
		data &= readinputport(7) & cs_extra3 & ss_extra2;
		/* SYMBOL SHIFT for extra keys */
		if (ss_extra1 != 0x1f || ss_extra2 != 0x1f)
			data &= ~0x02;
	}

	data |= (0xe0); /* Set bits 5-7 - as reset above */

	/* cassette input from wav */
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0038 )
	{
		data &= ~0x40;
	}

	/* Issue 2 Spectrums default to having bits 5, 6 & 7 set.
	Issue 3 Spectrums default to having bits 5 & 7 set and bit 6 reset. */
	if (readinputport(16) & 0x80)
		data ^= (0x40);
	return data;
}

/* kempston joystick interface */
static  READ8_HANDLER(spectrum_port_1f_r)
{
  return readinputport(13) & 0x1f;
}

/* fuller joystick interface */
static  READ8_HANDLER(spectrum_port_7f_r)
{
  return readinputport(14) | (0xff^0x8f);
}

/* mikrogen joystick interface */
static  READ8_HANDLER(spectrum_port_df_r)
{
  return readinputport(15) | (0xff^0x1f);
}

static  READ8_HANDLER ( spectrum_port_r )
{
		if ((offset & 1)==0)
			return spectrum_port_fe_r(offset);

		if ((offset & 0xff)==0x1f)
			return spectrum_port_1f_r(offset);

		if ((offset & 0xff)==0x7f)
			return spectrum_port_7f_r(offset);

		if ((offset & 0xff)==0xdf)
			return spectrum_port_df_r(offset);

		return cpu_getscanline()<193 ? spectrum_colorram[(cpu_getscanline()&0xf8)<<2]:0xff;
}

static WRITE8_HANDLER ( spectrum_port_w )
{
	if ((offset & 1)==0)
		spectrum_port_fe_w(offset,data);
	else
	{
		logerror("Write %02x to Port: %04x\n", data, offset);
	}
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (spectrum_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( spectrum_port_r, spectrum_port_w )
ADDRESS_MAP_END


/****************************************************************************************************/
/* functions and data used by spectrum 128, spectrum +2, spectrum +3 and scorpion */
static unsigned char *spectrum_ram = NULL;

static void spectrum_alloc_ram(int ram_size_in_k)
{
	spectrum_ram = (unsigned char *)auto_malloc(ram_size_in_k*1024);
	memset(spectrum_ram, 0, ram_size_in_k*1024);
}


/****************************************************************************************************/
/* Spectrum 128 specific functions */

int spectrum_128_port_7ffd_data = -1;
unsigned char *spectrum_128_screen_location = NULL;

static WRITE8_HANDLER(spectrum_128_port_7ffd_w)
{
	   /* D0-D2: RAM page located at 0x0c000-0x0ffff */
	   /* D3 - Screen select (screen 0 in ram page 5, screen 1 in ram page 7 */
	   /* D4 - ROM select - which rom paged into 0x0000-0x03fff */
	   /* D5 - Disable paging */

		/* disable paging? */
		if (spectrum_128_port_7ffd_data & 0x20)
				return;

		/* store new state */
		spectrum_128_port_7ffd_data = data;

		/* update memory */
		spectrum_128_update_memory();
}

extern void spectrum_128_update_memory(void)
{
		unsigned char *ChosenROM;
		int ROMSelection;

		if (spectrum_128_port_7ffd_data & 8)
		{
				logerror("SCREEN 1: BLOCK 7\n");
				spectrum_128_screen_location = spectrum_ram + (7<<14);
		}
		else
		{
				logerror("SCREEN 0: BLOCK 5\n");
				spectrum_128_screen_location = spectrum_ram + (5<<14);
		}

		/* select ram at 0x0c000-0x0ffff */
		{
				int ram_page;
				unsigned char *ram_data;

				ram_page = spectrum_128_port_7ffd_data & 0x07;
				ram_data = spectrum_ram + (ram_page<<14);

				memory_set_bankptr(4, ram_data);
				memory_set_bankptr(8, ram_data);

				logerror("RAM at 0xc000: %02x\n",ram_page);
		}

		/* ROM switching */
		ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01);

		/* rom 0 is 128K rom, rom 1 is 48 BASIC */

		ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

		memory_set_bankptr(1, ChosenROM);

		logerror("rom switch: %02x\n", ROMSelection);
}



static WRITE8_HANDLER(spectrum_128_port_bffd_w)
{
		AY8910_write_port_0_w(0, data);
}

static WRITE8_HANDLER(spectrum_128_port_fffd_w)
{
		AY8910_control_port_0_w(0, data);
}

static  READ8_HANDLER(spectrum_128_port_fffd_r)
{
		return AY8910_read_port_0_r(0);
}

static  READ8_HANDLER ( spectrum_128_port_r )
{
	 if ((offset & 1)==0)
	 {
		 return spectrum_port_fe_r(offset);
	 }

	 if ((offset & 2)==0)
	 {
		switch ((offset>>14) & 0x03)
		{
			default:
				break;

			case 3:
				return spectrum_128_port_fffd_r(offset);
		}
	 }

	 /* don't think these are correct! */
	 if ((offset & 0xff)==0x1f)
		 return spectrum_port_1f_r(offset);

	 if ((offset & 0xff)==0x7f)
		 return spectrum_port_7f_r(offset);

	 if ((offset & 0xff)==0xdf)
		 return spectrum_port_df_r(offset);

	 return cpu_getscanline()<193 ? spectrum_128_screen_location[0x1800|(cpu_getscanline()&0xf8)<<2]:0xff;
}

static WRITE8_HANDLER ( spectrum_128_port_w )
{
		if ((offset & 1)==0)
				spectrum_port_fe_w(offset,data);

		/* Only decodes on A15, A14 & A1 */
		else if ((offset & 2)==0)
		{
				switch ((offset>>8) & 0xc0)
				{
						case 0x40:
								spectrum_128_port_7ffd_w(offset, data);
								break;
						case 0x80:
								spectrum_128_port_bffd_w(offset, data);
								break;
						case 0xc0:
								spectrum_128_port_fffd_w(offset, data);
								break;
						default:
								logerror("Write %02x to 128 port: %04x\n", data, offset);
				}
		}
		else
		{
			logerror("Write %02x to 128 port: %04x\n", data, offset);
		}
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (spectrum_128_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( spectrum_128_port_r, spectrum_128_port_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START (spectrum_128_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x3fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK5 )
	AM_RANGE( 0x4000, 0x7fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK6 )
	AM_RANGE( 0x8000, 0xbfff) AM_READWRITE( MRA8_BANK3, MWA8_BANK7 )
	AM_RANGE( 0xc000, 0xffff) AM_READWRITE( MRA8_BANK4, MWA8_BANK8 )
ADDRESS_MAP_END

static MACHINE_RESET( spectrum_128 )
{
	spectrum_alloc_ram(128);

	/* 0x0000-0x3fff always holds ROM */

	/* Bank 5 is always in 0x4000 - 0x7fff */
	memory_set_bankptr(2, spectrum_ram + (5<<14));
	memory_set_bankptr(6, spectrum_ram + (5<<14));

	/* Bank 2 is always in 0x8000 - 0xbfff */
	memory_set_bankptr(3, spectrum_ram + (2<<14));
	memory_set_bankptr(7, spectrum_ram + (2<<14));

	/* set initial ram config */
	spectrum_128_port_7ffd_data = 0;
	spectrum_128_update_memory();

	machine_reset_spectrum(machine);
}

/****************************************************************************************************/
/* Spectrum + 3 specific functions */
/* This driver uses some of the spectrum_128 functions. The +3 is similar to a spectrum 128
but with a disc drive */

int spectrum_plus3_port_1ffd_data = -1;


static nec765_interface spectrum_plus3_nec765_interface =
{
		NULL,
		NULL
};


static int spectrum_plus3_memory_selections[]=
{
		0,1,2,3,
		4,5,6,7,
		4,5,6,3,
		4,7,6,3
};

static WRITE8_HANDLER(spectrum_plus3_port_3ffd_w)
{
		if (~readinputport(16) & 0x20)
				nec765_data_w(0,data);
}

static  READ8_HANDLER(spectrum_plus3_port_3ffd_r)
{
		if (readinputport(16) & 0x20)
				return 0xff;
		else
				return nec765_data_r(0);
}


static  READ8_HANDLER(spectrum_plus3_port_2ffd_r)
{
		if (readinputport(16) & 0x20)
				return 0xff;
		else
				return nec765_status_r(0);
}


void spectrum_plus3_update_memory(void)
{
	if (spectrum_128_port_7ffd_data & 8)
	{
			logerror("+3 SCREEN 1: BLOCK 7\n");
			spectrum_128_screen_location = spectrum_ram + (7<<14);
	}
	else
	{
			logerror("+3 SCREEN 0: BLOCK 5\n");
			spectrum_128_screen_location = spectrum_ram + (5<<14);
	}

	if ((spectrum_plus3_port_1ffd_data & 0x01)==0)
	{
			int ram_page;
			unsigned char *ram_data;

			/* ROM switching */
			unsigned char *ChosenROM;
			int ROMSelection;

			/* select ram at 0x0c000-0x0ffff */
			ram_page = spectrum_128_port_7ffd_data & 0x07;
			ram_data = spectrum_ram + (ram_page<<14);

			memory_set_bankptr(4, ram_data);
			memory_set_bankptr(8, ram_data);

			logerror("RAM at 0xc000: %02x\n",ram_page);

			/* Reset memory between 0x4000 - 0xbfff in case extended paging was being used */
			/* Bank 5 in 0x4000 - 0x7fff */
			memory_set_bankptr(2, spectrum_ram + (5<<14));
			memory_set_bankptr(6, spectrum_ram + (5<<14));

			/* Bank 2 in 0x8000 - 0xbfff */
			memory_set_bankptr(3, spectrum_ram + (2<<14));
			memory_set_bankptr(7, spectrum_ram + (2<<14));


			ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) |
				((spectrum_plus3_port_1ffd_data>>1) & 0x02);

			/* rom 0 is editor, rom 1 is syntax, rom 2 is DOS, rom 3 is 48 BASIC */

			ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

			memory_set_bankptr(1, ChosenROM);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MWA8_ROM);

			logerror("rom switch: %02x\n", ROMSelection);
	}
	else
	{
			/* Extended memory paging */

			int *memory_selection;
			int MemorySelection;
			unsigned char *ram_data;

			MemorySelection = (spectrum_plus3_port_1ffd_data>>1) & 0x03;

			memory_selection = &spectrum_plus3_memory_selections[(MemorySelection<<2)];

			ram_data = spectrum_ram + (memory_selection[0]<<14);
			memory_set_bankptr(1, ram_data);
			memory_set_bankptr(5, ram_data);
			/* allow writes to 0x0000-0x03fff */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MWA8_BANK5);

			ram_data = spectrum_ram + (memory_selection[1]<<14);
			memory_set_bankptr(2, ram_data);
			memory_set_bankptr(6, ram_data);

			ram_data = spectrum_ram + (memory_selection[2]<<14);
			memory_set_bankptr(3, ram_data);
			memory_set_bankptr(7, ram_data);

			ram_data = spectrum_ram + (memory_selection[3]<<14);
			memory_set_bankptr(4, ram_data);
			memory_set_bankptr(8, ram_data);

			logerror("extended memory paging: %02x\n",MemorySelection);
		}
}



static WRITE8_HANDLER(spectrum_plus3_port_7ffd_w)
{
	   /* D0-D2: RAM page located at 0x0c000-0x0ffff */
	   /* D3 - Screen select (screen 0 in ram page 5, screen 1 in ram page 7 */
	   /* D4 - ROM select - which rom paged into 0x0000-0x03fff */
	   /* D5 - Disable paging */

		/* disable paging? */
		if (spectrum_128_port_7ffd_data & 0x20)
				return;

		/* store new state */
		spectrum_128_port_7ffd_data = data;

		/* update memory */
		spectrum_plus3_update_memory();
}

static WRITE8_HANDLER(spectrum_plus3_port_1ffd_w)
{

		/* D0-D1: ROM/RAM paging */
		/* D2: Affects if d0-d1 work on ram/rom */
		/* D3 - Disk motor on/off */
		/* D4 - parallel port strobe */

		floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), data & (1<<3));
		floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), data & (1<<3));
		floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1, 1);
		floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1, 1);

		spectrum_plus3_port_1ffd_data = data;

		/* disable paging? */
		if ((spectrum_128_port_7ffd_data & 0x20)==0)
		{
				/* no */
				spectrum_plus3_update_memory();
		}
}

/* decoding as per spectrum FAQ on www.worldofspectrum.org */
static  READ8_HANDLER ( spectrum_plus3_port_r )
{
	 if ((offset & 1)==0)
	 {
		 return spectrum_port_fe_r(offset);
	 }

	 if ((offset & 2)==0)
	 {
		 switch ((offset>>14) & 0x03)
		 {
			/* +3 fdc,memory,centronics */
			case 0:
			{
				switch ((offset>>12) & 0x03)
				{
					/* +3 centronics */
					case 0:
						break;

					/* +3 fdc status */
					case 2:
						return spectrum_plus3_port_2ffd_r(offset);
					/* +3 fdc data */
					case 3:
						return spectrum_plus3_port_3ffd_r(offset);

					default:
						break;
				}
			}
			break;

			/* 128k AY data */
			case 3:
				return spectrum_128_port_fffd_r(offset);

			default:
				break;
		 }
	 }

	 return cpu_getscanline()<193 ? spectrum_128_screen_location[0x1800|(cpu_getscanline()&0xf8)<<2]:0xff;
}

static WRITE8_HANDLER ( spectrum_plus3_port_w )
{
		if ((offset & 1)==0)
				spectrum_port_fe_w(offset,data);

		/* the following is not decoded exactly, need to check
		what is correct! */
		
		if ((offset & 2)==0)
		{
			switch ((offset>>14) & 0x03)
			{
				/* +3 fdc,memory,centronics */
				case 0:
				{
					switch ((offset>>12) & 0x03)
					{
						/* +3 centronics */
						case 0:
						{


						}
						break;

						/* +3 memory */
						case 1:
							spectrum_plus3_port_1ffd_w(offset, data);
							break;
							
						/* +3 fdc data */
						case 3:
							spectrum_plus3_port_3ffd_w(offset,data);
							break;

						default:
							break;
					}
				}
				break;

				/* 128k memory */
				case 1:
					spectrum_plus3_port_7ffd_w(offset, data);
					break;

				/* 128k AY data */
				case 2:
					spectrum_128_port_bffd_w(offset, data);
					break;

				/* 128K AY register */
				case 3:
					spectrum_128_port_fffd_w(offset, data);
			
				default:
					break;
			}
		}

/*logerror("Write %02x to +3 port: %04x\n", data, offset); */
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (spectrum_plus3_io, ADDRESS_SPACE_IO, 8)
		AM_RANGE(0x0000, 0xffff) AM_READWRITE( spectrum_plus3_port_r, spectrum_plus3_port_w )
ADDRESS_MAP_END

static MACHINE_RESET( spectrum_plus3 )
{
	spectrum_alloc_ram(128);

	nec765_init(&spectrum_plus3_nec765_interface, NEC765A);

	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 0), FLOPPY_DRIVE_SS_40);
	floppy_drive_set_geometry(image_from_devtype_and_index(IO_FLOPPY, 1), FLOPPY_DRIVE_SS_40);

	/* Initial configuration */
	spectrum_128_port_7ffd_data = 0;
	spectrum_plus3_port_1ffd_data = 0;
	spectrum_plus3_update_memory();

	machine_reset_spectrum(machine);
}


/****************************************************************************************************/
/* TS2048 specific functions */


int ts2068_port_ff_data = -1; /* Display enhancement control */
int ts2068_port_f4_data = -1; /* Horizontal Select Register */

static  READ8_HANDLER(ts2068_port_f4_r)
{
		return ts2068_port_f4_data;
}

static WRITE8_HANDLER(ts2068_port_f4_w)
{
		ts2068_port_f4_data = data;
		ts2068_update_memory();
}

static WRITE8_HANDLER(ts2068_port_f5_w)
{
		AY8910_control_port_0_w(0, data);
}

static  READ8_HANDLER(ts2068_port_f6_r)
{
		/* TODO - Reading from register 14 reads the joystick ports
		   set bit 8 of address to read joystick #1
		   set bit 9 of address to read joystick #2
		   if both bits are set then OR values
		   Bit 0 up, 1 down, 2 left, 3 right, 7 fire active low. Other bits 1
		*/
		return AY8910_read_port_0_r(0);
}

static WRITE8_HANDLER(ts2068_port_f6_w)
{
		AY8910_write_port_0_w(0, data);
}

static  READ8_HANDLER(ts2068_port_ff_r)
{
		return ts2068_port_ff_data;
}

static WRITE8_HANDLER(ts2068_port_ff_w)
{
		/* Bits 0-2 Video Mode Select
		   Bits 3-5 64 column mode ink/paper selection
					(See ts2068_vh_screenrefresh for more info)
		   Bit	6	17ms Interrupt Inhibit
		   Bit	7	Cartridge (0) / EXROM (1) select
		*/
		ts2068_port_ff_data = data;
		ts2068_update_memory();
		logerror("Port %04x write %02x\n", offset, data);
}


static  READ8_HANDLER ( ts2068_port_r )
{
		switch (offset & 0xff)
		{
				/* Note: keys only decoded on port #fe not all even ports so
				   ports #f4 & #f6 correctly read */
				case 0xf4: return ts2068_port_f4_r(offset);
				case 0xf6: return ts2068_port_f6_r(offset);
				case 0xff: return ts2068_port_ff_r(offset);

				case 0xfe: return spectrum_port_fe_r(offset);
				case 0x1f: return spectrum_port_1f_r(offset);
				case 0x7f: return spectrum_port_7f_r(offset);
				case 0xdf: return spectrum_port_df_r(offset);
		}
		logerror("Read from port: %04x\n", offset);

		return 0xff;
}

static WRITE8_HANDLER ( ts2068_port_w )
{
/* Ports #fd & #fc were reserved by Timex for bankswitching and are not used
   by either the hardware or system software.
   Port #fb is the Thermal printer port and works exactly as the Sinclair
   Printer - ie not yet emulated.
*/
		switch (offset & 0xff)
		{
				case 0xfe: spectrum_port_fe_w(offset,data); break;
				case 0xf4: ts2068_port_f4_w(offset,data); break;
				case 0xf5: ts2068_port_f5_w(offset,data); break;
				case 0xf6: ts2068_port_f6_w(offset,data); break;
				case 0xff: ts2068_port_ff_w(offset,data); break;
				default:
						logerror("Write %02x to Port: %04x\n", data, offset);
		}
}



/*******************************************************************
 *
 *		Bank switch between the 3 internal memory banks HOME, EXROM
 *		and DOCK (Cartridges). The HOME bank contains 16K ROM in the
 *		0-16K area and 48K RAM fills the rest. The EXROM contains 8K
 *		ROM and can appear in every 8K segment (ie 0-8K, 8-16K etc).
 *		The DOCK is empty and is meant to be occupied by cartridges
 *		you can plug into the cartridge dock of the 2068.
 *
 *		The address space is divided into 8 8K chunks. Bit 0 of port
 *		#f4 corresponds to the 0-8K chunk, bit 1 to the 8-16K chunk
 *		etc. If the bit is 0 then the chunk is controlled by the HOME
 *		bank. If the bit is 1 then the chunk is controlled by either
 *		the DOCK or EXROM depending on bit 7 of port #ff. Note this
 *		means that that the Z80 can't see chunks of the EXROM and DOCK
 *		at the same time.
 *
 *******************************************************************/
void ts2068_update_memory(void)
{
	unsigned char *ChosenROM, *ExROM, *DOCK;
	read8_handler rh;
	write8_handler wh;

	DOCK = timex_cart_data;

	ExROM = memory_region(REGION_CPU1) + 0x014000;

	if (ts2068_port_f4_data & 0x01)
	{
		if (ts2068_port_ff_data & 0x80)
		{
				rh = MRA8_BANK1;
				wh = MWA8_ROM;
				memory_set_bankptr(1, ExROM);
				logerror("0000-1fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(1, DOCK);
				rh = MRA8_BANK1;
				if (timex_cart_chunks&0x01)
					wh = MWA8_BANK9;
				else
					wh = MWA8_ROM;
			}
			else
			{
				rh = MRA8_NOP;
				wh = MWA8_ROM;
			}
			logerror("0000-1fff Cartridge\n");
		}
	}
	else
	{
		ChosenROM = memory_region(REGION_CPU1) + 0x010000;
		memory_set_bankptr(1, ChosenROM);
		rh = MRA8_BANK1;
		wh = MWA8_ROM;
		logerror("0000-1fff HOME\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x02)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(2, ExROM);
			rh = MRA8_BANK2;
			wh = MWA8_ROM;
			logerror("2000-3fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(2, DOCK+0x2000);
				rh = MRA8_BANK2;
				if (timex_cart_chunks&0x02)
					wh = MWA8_BANK10;
				else
					wh = MWA8_ROM;
			}
			else
			{
				rh = MRA8_NOP;
				wh = MWA8_ROM;
			}
			logerror("2000-3fff Cartridge\n");
		}
	}
	else
	{
		ChosenROM = memory_region(REGION_CPU1) + 0x012000;
		memory_set_bankptr(2, ChosenROM);
		rh = MRA8_BANK2;
		wh = MWA8_ROM;
		logerror("2000-3fff HOME\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x04)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(3, ExROM);
			rh = MRA8_BANK3;
			wh = MWA8_ROM;
			logerror("4000-5fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(3, DOCK+0x4000);
				rh = MRA8_BANK3;
				if (timex_cart_chunks&0x04)
					wh = MWA8_BANK11;
				else
					wh = MWA8_ROM;
			}
			else
			{
				rh = MRA8_NOP;
				wh = MWA8_ROM;
			}
			logerror("4000-5fff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(3, mess_ram);
		memory_set_bankptr(11, mess_ram);
		rh = MRA8_BANK3;
		wh = MWA8_BANK11;
		logerror("4000-5fff RAM\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x08)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(4, ExROM);
			rh = MRA8_BANK4;
			wh = MWA8_ROM;
			logerror("6000-7fff EXROM\n");
		}
		else
		{
				if (timex_cart_type == TIMEX_CART_DOCK)
				{
					memory_set_bankptr(4, DOCK+0x6000);
					rh = MRA8_BANK4;
					if (timex_cart_chunks&0x08)
						wh = MWA8_BANK12;
					else
						wh = MWA8_ROM;
				}
				else
				{
					rh = MRA8_NOP;
					wh = MWA8_ROM;
				}
				logerror("6000-7fff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(4, mess_ram + 0x2000);
		memory_set_bankptr(12, mess_ram + 0x2000);
		rh = MRA8_BANK4;
		wh = MWA8_BANK12;
		logerror("6000-7fff RAM\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x10)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(5, ExROM);
			rh = MRA8_BANK5;
			wh = MWA8_ROM;
			logerror("8000-9fff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(5, DOCK+0x8000);
				rh = MRA8_BANK5;
				if (timex_cart_chunks&0x10)
					wh = MWA8_BANK13;
				else
					wh = MWA8_ROM;
			}
			else
			{
				rh = MRA8_NOP;
				wh = MWA8_ROM;
			}
			logerror("8000-9fff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(5, mess_ram + 0x4000);
		memory_set_bankptr(13, mess_ram + 0x4000);
		rh = MRA8_BANK5;
		wh = MWA8_BANK13;
		logerror("8000-9fff RAM\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0,rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0,wh);

	if (ts2068_port_f4_data & 0x20)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(6, ExROM);
			rh = MRA8_BANK6;
			wh = MWA8_ROM;
			logerror("a000-bfff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(6, DOCK+0xa000);
				rh = MRA8_BANK6;
				if (timex_cart_chunks&0x20)
					wh = MWA8_BANK14;
				else
					wh = MWA8_ROM;
			}
			else
			{
				rh = MRA8_NOP;
				wh = MWA8_ROM;
			}
			logerror("a000-bfff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(6, mess_ram + 0x6000);
		memory_set_bankptr(14, mess_ram + 0x6000);
		rh = MRA8_BANK6;
		wh = MWA8_BANK14;
		logerror("a000-bfff RAM\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x40)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(7, ExROM);
			rh = MRA8_BANK7;
			wh = MWA8_ROM;
			logerror("c000-dfff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(7, DOCK+0xc000);
				rh = MRA8_BANK7;
				if (timex_cart_chunks&0x40)
					wh = MWA8_BANK15;
				else
					wh = MWA8_ROM;
			}
			else
			{
				rh = MRA8_NOP;
				wh = MWA8_ROM;
			}
			logerror("c000-dfff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(7, mess_ram + 0x8000);
		memory_set_bankptr(15, mess_ram + 0x8000);
		rh = MRA8_BANK7;
		wh = MWA8_BANK15;
		logerror("c000-dfff RAM\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, wh);

	if (ts2068_port_f4_data & 0x80)
	{
		if (ts2068_port_ff_data & 0x80)
		{
			memory_set_bankptr(8, ExROM);
			rh = MRA8_BANK8;
			wh = MWA8_ROM;
			logerror("e000-ffff EXROM\n");
		}
		else
		{
			if (timex_cart_type == TIMEX_CART_DOCK)
			{
				memory_set_bankptr(8, DOCK+0xe000);
				rh = MRA8_BANK8;
				if (timex_cart_chunks&0x80)
					wh = MWA8_BANK16;
				else
					wh = MWA8_ROM;
			}
			else
			{
				rh = MRA8_NOP;
				wh = MWA8_ROM;
			}
			logerror("e000-ffff Cartridge\n");
		}
	}
	else
	{
		memory_set_bankptr(8, mess_ram + 0xa000);
		memory_set_bankptr(16, mess_ram + 0xa000);
		rh = MRA8_BANK8;
		wh = MWA8_BANK16;
		logerror("e000-ffff RAM\n");
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, wh);
}


static ADDRESS_MAP_START (ts2068_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( ts2068_port_r, ts2068_port_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START (ts2068_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK9 )
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK10 )
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE( MRA8_BANK3, MWA8_BANK11 )
	AM_RANGE(0x6000, 0x7fff) AM_READWRITE( MRA8_BANK4, MWA8_BANK12 )
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE( MRA8_BANK5, MWA8_BANK13 )
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE( MRA8_BANK6, MWA8_BANK14 )
	AM_RANGE(0xc000, 0xdfff) AM_READWRITE( MRA8_BANK7, MWA8_BANK15 )
	AM_RANGE(0xe000, 0xffff) AM_READWRITE( MRA8_BANK8, MWA8_BANK16 )
ADDRESS_MAP_END


static MACHINE_RESET( ts2068 )
{
	ts2068_port_ff_data = 0;
	ts2068_port_f4_data = 0;
	ts2068_update_memory();

	machine_reset_spectrum(machine);
}


/****************************************************************************************************/
/* TC2048 specific functions */


static void tc2048_port_ff_w(int offset, int data)
{
		ts2068_port_ff_data = data;
		logerror("Port %04x write %02x\n", offset, data);
}

static  READ8_HANDLER ( tc2048_port_r )
{
		if ((offset & 1)==0)
				return spectrum_port_fe_r(offset);
		switch (offset & 0xff)
		{
				case 0xff: return ts2068_port_ff_r(offset);
				case 0x1f: return spectrum_port_1f_r(offset);
				case 0x7f: return spectrum_port_7f_r(offset);
				case 0xdf: return spectrum_port_df_r(offset);
		}

		logerror("Read from port: %04x\n", offset);
		return 0xff;
}

static WRITE8_HANDLER ( tc2048_port_w )
{
		if ((offset & 1)==0)
				spectrum_port_fe_w(offset,data);
		else if ((offset & 0xff)==0xff)
				tc2048_port_ff_w(offset,data);
		else
		{
				logerror("Write %02x to Port: %04x\n", data, offset);
		}
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (tc2048_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( tc2048_port_r, tc2048_port_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START (tc2048_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x3fff) AM_ROM
	AM_RANGE( 0x4000, 0xffff) AM_READWRITE( MRA8_BANK1, MWA8_BANK2 )
ADDRESS_MAP_END

static MACHINE_RESET( tc2048 )
{
	memory_set_bankptr(1, mess_ram);
	memory_set_bankptr(2, mess_ram);
	ts2068_port_ff_data = 0;

	machine_reset_spectrum(machine);
}


/****************************************************************************************************/
/* BETADISK/TR-DOS disc controller emulation */
/* microcontroller KR1818VG93 is a russian wd179x clone */
#include "machine/wd17xx.h"

/*
DRQ (D6) and INTRQ (D7).
DRQ - signal showing request of data by microcontroller
INTRQ - signal of completion of execution of command.
*/

static int betadisk_status;
static int betadisk_active;
static void (*betadisk_memory_update)(void);

static OPBASE_HANDLER(betadisk_opbase_handler)
{

	int pc;

	pc = activecpu_get_pc();

	if ((pc & 0xc000)!=0x0000)
	{
		/* outside rom area */
		betadisk_active = 0;

		betadisk_memory_update();
	}
	else
	{
		/* inside rom area, switch on betadisk */
	//	betadisk_active = 1;

	//	betadisk_memory_update();
	}


	return pc & 0x0ffff;
}

static void betadisk_wd179x_callback(int state)
{
	switch (state)
	{
		case WD179X_DRQ_SET:
		{
			betadisk_status |= (1<<6);
		}
		break;

		case WD179X_DRQ_CLR:
		{
			betadisk_status &=~(1<<6);
		}
		break;

		case WD179X_IRQ_SET:
		{
			betadisk_status |= (1<<7);
		}
		break;

		case WD179X_IRQ_CLR:
		{
			betadisk_status &=~(1<<7);
		}
		break;
	}
}

/* these are active only when betadisk is enabled */
static WRITE8_HANDLER(betadisk_w)
{

	if (betadisk_active)
	{

	}
}


/* these are active only when betadisk is enabled */
static  READ8_HANDLER(betadisk_r)
{
	if (betadisk_active)
	{
		/* decoding of these ports might be wrong - to be checked! */
		if ((offset & 0x01f)==0x01f)
		{
			switch (offset & 0x0ff)
			{

			}
		}

	}

	return 0x0ff;
}

static void	 betadisk_init(void)
{
	betadisk_active = 0;
	betadisk_status = 0x03f;
	wd179x_init(WD_TYPE_179X,&betadisk_wd179x_callback);
}

/****************************************************************************************************/
/* Zs Scorpion 256 */

/*
port 7ffd. full compatibility with Zx spectrum 128. digits are:

D0-D2 - number of RAM page to put in C000-FFFF
D3    - switch of address for RAM of screen. 0 - 4000, 1 - c000
D4    - switch of ROM : 0-zx128, 1-zx48
D5    - 1 in this bit will block further output in port 7FFD, until reset.
*/

/*
port 1ffd - additional port for resources of computer.

D0    - block of ROM in 0-3fff. when set to 1 - allows read/write page 0 of RAM
D1    - selects ROM expansion. this rom contains main part of service monitor.
D2    - not used
D3    - used for output in RS-232C
D4    - extended RAM. set to 1 - connects RAM page with number 8-15 in
	C000-FFFF. number of page is given in gidits D0-D2 of port 7FFD
D5    - signal of strobe for interface centronics. to form the strobe has to be
	set to 1.
D6-D7 - not used. ( yet ? )
*/

/* rom 0=zx128, 1=zx48, 2 = service monitor, 3=tr-dos */

static int scorpion_256_port_1ffd_data = 0;

static void scorpion_update_memory(void)
{
	unsigned char *ChosenROM;
	int ROMSelection;
	read8_handler rh;
	write8_handler wh;

	if (spectrum_128_port_7ffd_data & 8)
	{
		logerror("SCREEN 1: BLOCK 7\n");
		spectrum_128_screen_location = spectrum_ram + (7<<14);
	}
	else
	{
		logerror("SCREEN 0: BLOCK 5\n");
		spectrum_128_screen_location = spectrum_ram + (5<<14);
	}

	/* select ram at 0x0c000-0x0ffff */
	{
		int ram_page;
		unsigned char *ram_data;

		ram_page = (spectrum_128_port_7ffd_data & 0x07) | ((scorpion_256_port_1ffd_data & (1<<4))>>1);
		ram_data = spectrum_ram + (ram_page<<14);

		memory_set_bankptr(4, ram_data);
		memory_set_bankptr(8, ram_data);

		logerror("RAM at 0xc000: %02x\n",ram_page);
	}

	if (scorpion_256_port_1ffd_data & (1<<0))
	{
		/* ram at 0x0000 */
		logerror("RAM at 0x0000\n");

		/* connect page 0 of ram to 0x0000 */
		rh = MRA8_BANK1;
		wh = MWA8_BANK5;
		memory_set_bankptr(1, spectrum_ram+(8<<14));
		memory_set_bankptr(5, spectrum_ram+(8<<14));
	}
	else
	{
		/* rom at 0x0000 */
		logerror("ROM at 0x0000\n");

		/* connect page 0 of rom to 0x0000 */
		rh = MRA8_BANK1;
		wh = MWA8_NOP;

		if (scorpion_256_port_1ffd_data & (1<<1))
		{
			ROMSelection = 2;
		}
		else
		{

			/* ROM switching */
			ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01);
		}

		/* rom 0 is 128K rom, rom 1 is 48 BASIC */
		ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

		memory_set_bankptr(1, ChosenROM);

		logerror("rom switch: %02x\n", ROMSelection);
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, wh);
}


static WRITE8_HANDLER(scorpion_port_7ffd_w)
{
	logerror("scorpion 7ffd w: %02x\n", data);

	/* disable paging? */
	if (spectrum_128_port_7ffd_data & 0x20)
		return;

	/* store new state */
	spectrum_128_port_7ffd_data = data;

	/* update memory */
	scorpion_update_memory();
}

static WRITE8_HANDLER(scorpion_port_1ffd_w)
{
	logerror("scorpion 1ffd w: %02x\n", data);

	scorpion_256_port_1ffd_data = data;

	/* disable paging? */
	if ((spectrum_128_port_7ffd_data & 0x20)==0)
	{
		scorpion_update_memory();
	}
}


/* not sure if decoding is full or partial on scorpion */
/* TO BE CHECKED! */
static  READ8_HANDLER(scorpion_port_r)
{
	 if ((offset & 1)==0)
	 {
		 return spectrum_port_fe_r(offset);
	 }

	 /* KT: the following is not decoded exactly, need to check what
	 is correct */
	 if ((offset & 2)==0)
	 {
		 switch ((offset>>8) & 0xff)
		 {
				case 0xff: return spectrum_128_port_fffd_r(offset);
				case 0x1f: return spectrum_port_1f_r(offset);
				case 0x7f: return spectrum_port_7f_r(offset);
				case 0xdf: return spectrum_port_df_r(offset);
		 }
	 }
#if 0
	 switch (offset & 0x0ff)
	 {
		case 0x01f:
			return wd179x_status_r(offset);
		case 0x03f:
			return wd179x_track_r(offset);
		case 0x05f:
			return wd179x_sector_r(offset);
		case 0x07f:
			return wd179x_data_r(offset);
		case 0x0ff:
			return betadisk_status;
	 }
#endif
	 logerror("Read from scorpion port: %04x\n", offset);

	 return 0xff;
}


/* not sure if decoding is full or partial on scorpion */
/* TO BE CHECKED! */
static WRITE8_HANDLER(scorpion_port_w)
{
	if ((offset & 1)==0)
		spectrum_port_fe_w(offset,data);

	else if ((offset & 2)==0)
	{
			switch ((offset>>8) & 0xf0)
			{
				case 0x70:
						scorpion_port_7ffd_w(offset, data);
						break;
				case 0xb0:
						spectrum_128_port_bffd_w(offset, data);
						break;
				case 0xf0:
						spectrum_128_port_fffd_w(offset, data);
						break;
				case 0x10:
						scorpion_port_1ffd_w(offset, data);
						break;
				default:
						logerror("Write %02x to scorpion port: %04x\n", data, offset);
			}
	}
	else
	{
		logerror("Write %02x to scorpion port: %04x\n", data, offset);
	}
}



/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (scorpion_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(scorpion_port_r, scorpion_port_w)
ADDRESS_MAP_END

static MACHINE_RESET( scorpion )
{
	spectrum_alloc_ram(256);

	/* Bank 5 is always in 0x4000 - 0x7fff */
	memory_set_bankptr(2, spectrum_ram + (5<<14));
	memory_set_bankptr(6, spectrum_ram + (5<<14));

	/* Bank 2 is always in 0x8000 - 0xbfff */
	memory_set_bankptr(3, spectrum_ram + (2<<14));
	memory_set_bankptr(7, spectrum_ram + (2<<14));

	spectrum_128_port_7ffd_data = 0;
	scorpion_256_port_1ffd_data = 0;

	scorpion_update_memory();

	betadisk_init();
}

/****************************************************************************************************/
/* pentagon */

static  READ8_HANDLER(pentagon_port_r)
{
	return 0x0ff;
}


static WRITE8_HANDLER(pentagon_port_w)
{
}



/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (pentagon_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( pentagon_port_r, pentagon_port_w )
ADDRESS_MAP_END

static MACHINE_RESET( pentagon )
{
	spectrum_alloc_ram(128);

	/* Bank 5 is always in 0x4000 - 0x7fff */
	memory_set_bankptr(2, spectrum_ram + (5<<14));
	memory_set_bankptr(6, spectrum_ram + (5<<14));

	/* Bank 2 is always in 0x8000 - 0xbfff */
	memory_set_bankptr(3, spectrum_ram + (2<<14));
	memory_set_bankptr(7, spectrum_ram + (2<<14));

	betadisk_init();
}

/****************************************************************************************************/

static gfx_layout spectrum_charlayout = {
	8,8,
	256,
	1,						/* 1 bits per pixel */

	{ 0 },					/* no bitplanes; 1 bit per pixel */

	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8*256, 16*256, 24*256, 32*256, 40*256, 48*256, 56*256 },

	8				/* every char takes 1 consecutive byte */
};

static gfx_decode spectrum_gfxdecodeinfo[] = {
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ -1 } /* end of array */
};

INPUT_PORTS_START( spectrum )
	PORT_START /* [0] 0xFEFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z  COPY    :      LN       BEEP") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X  CLEAR   Pound  EXP      INK") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C  CONT    ?      LPRINT   PAPER") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V  CLS     /      LLIST    FLASH") PORT_CODE(KEYCODE_V)

	PORT_START /* [1] 0xFDFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A  NEW     STOP   READ     ~") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S  SAVE    NOT    RESTORE  |") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D  DIM     STEP   DATA     \\") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F  FOR     TO     SGN      {") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G  GOTO    THEN   ABS      }") PORT_CODE(KEYCODE_G)

	PORT_START /* [2] 0xFBFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q  PLOT    <=     SIN      ASN") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W  DRAW    <>     COS      ACS") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E  REM     >=     TAN      ATN") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R  RUN     <      INT      VERIFY") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T  RAND    >      RND      MERGE") PORT_CODE(KEYCODE_T)

	/* interface II uses this port for joystick */
	PORT_START /* [3] 0xF7FE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1          !      BLUE     DEF FN") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2          @      RED      FN") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3          #      MAGENTA  LINE") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4          $      GREEN    OPEN#") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5          %      CYAN     CLOSE#") PORT_CODE(KEYCODE_5)

	/* protek clashes with interface II! uses 5 = left, 6 = down, 7 = up, 8 = right, 0 = fire */
	PORT_START /* [4] 0xEFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0          _      BLACK    FORMAT") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9          )               POINT") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8          (               CAT") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7          '      WHITE    ERASE") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6          &      YELLOW   MOVE") PORT_CODE(KEYCODE_6)

	PORT_START /* [5] 0xDFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P  PRINT   \"      TAB      (c)") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O  POKE    ;      PEEK     OUT") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I  INPUT   AT     CODE     IN") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U  IF      OR     CHR$     ]") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y  RETURN  AND    STR$     [") PORT_CODE(KEYCODE_Y)

	PORT_START /* [6] 0xBFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L  LET     =      USR      ATTR") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K  LIST    +      LEN      SCREEN$") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J  LOAD    -      VAL      VAL$") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H  GOSUB   ^      SQR      CIRCLE") PORT_CODE(KEYCODE_H)

	PORT_START /* [7] 0x7FFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M  PAUSE   .      PI       INVERSE") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N  NEXT    ,      INKEY$   OVER") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B  BORDER  *      BIN      BRIGHT") PORT_CODE(KEYCODE_B)

	PORT_START /* [8] Spectrum+ Keys (set CAPS + 1-5) */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EDIT          (CAPS + 1)") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS LOCK     (CAPS + 2)") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TRUE VID      (CAPS + 3)") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INV VID       (CAPS + 4)") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor left   (CAPS + 5)") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* [9] Spectrum+ Keys (set CAPS + 6-0) */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL           (CAPS + 0)") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRAPH         (CAPS + 9)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor right  (CAPS + 8)") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor up     (CAPS + 7)") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor down   (CAPS + 6)") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* [10] Spectrum+ Keys (set CAPS + SPACE and CAPS + SYMBOL */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_PAUSE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXT MODE") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* [11] Spectrum+ Keys (set SYMBOL SHIFT + O/P */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\"") PORT_CODE(KEYCODE_F4)
	/*		  PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_QUOTE,  CODE_NONE ) */
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* [12] Spectrum+ Keys (set SYMBOL SHIFT + N/M */
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0xf3, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* [13] Kempston joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK RIGHT") PORT_CODE(JOYCODE_1_RIGHT)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK LEFT") PORT_CODE(JOYCODE_1_LEFT)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK DOWN") PORT_CODE(JOYCODE_1_DOWN)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK UP") PORT_CODE(JOYCODE_1_UP)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK FIRE") PORT_CODE(JOYCODE_1_BUTTON1)

	PORT_START /* [14] Fuller joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK UP") PORT_CODE(JOYCODE_1_UP)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK DOWN") PORT_CODE(JOYCODE_1_DOWN)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK LEFT") PORT_CODE(JOYCODE_1_LEFT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK RIGHT") PORT_CODE(JOYCODE_1_RIGHT)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK FIRE") PORT_CODE(JOYCODE_1_BUTTON1)

	PORT_START /* [15] Mikrogen joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK UP") PORT_CODE(JOYCODE_1_UP)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK DOWN") PORT_CODE(JOYCODE_1_DOWN)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK RIGHT") PORT_CODE(JOYCODE_1_RIGHT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK LEFT") PORT_CODE(JOYCODE_1_LEFT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK FIRE") PORT_CODE(JOYCODE_1_BUTTON1)


	PORT_START /* [16] */
	PORT_DIPNAME(0x80, 0x00, "Hardware Version")
	PORT_DIPSETTING(0x00, "Issue 2" )
	PORT_DIPSETTING(0x80, "Issue 3" )
	PORT_DIPNAME(0x40, 0x00, "End of .TAP action")
	PORT_DIPSETTING(0x00, "Disable .TAP support" )
	PORT_DIPSETTING(0x40, "Rewind tape to start (to reload earlier levels)" )
	PORT_DIPNAME(0x20, 0x00, "+3/+2a etc. Disk Drive")
	PORT_DIPSETTING(0x00, "Enabled" )
	PORT_DIPSETTING(0x20, "Disabled" )
	PORT_BIT(0x1f, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START /* [17] Toggle cassette motor */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Motor") PORT_CODE(KEYCODE_F1)

INPUT_PORTS_END

static unsigned char spectrum_palette[16*3] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0xbf,
	0xbf, 0x00, 0x00, 0xbf, 0x00, 0xbf,
	0x00, 0xbf, 0x00, 0x00, 0xbf, 0xbf,
	0xbf, 0xbf, 0x00, 0xbf, 0xbf, 0xbf,

	0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0xff, 0x00, 0xff,
	0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
};

static unsigned short spectrum_colortable[128*2] = {
	0,0, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0, 2,1, 2,2, 2,3, 2,4, 2,5, 2,6, 2,7,
	3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
	4,0, 4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7,
	5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
	6,0, 6,1, 6,2, 6,3, 6,4, 6,5, 6,6, 6,7,
	7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7,

	 8,8,  8,9,  8,10,	8,11,  8,12,  8,13,  8,14,	8,15,
	 9,8,  9,9,  9,10,	9,11,  9,12,  9,13,  9,14,	9,15,
	10,8, 10,9, 10,10, 10,11, 10,12, 10,13, 10,14, 10,15,
	11,8, 11,9, 11,10, 11,11, 11,12, 11,13, 11,14, 11,15,
	12,8, 12,9, 12,10, 12,11, 12,12, 12,13, 12,14, 12,15,
	13,8, 13,9, 13,10, 13,11, 13,12, 13,13, 13,14, 13,15,
	14,8, 14,9, 14,10, 14,11, 14,12, 14,13, 14,14, 14,15,
	15,8, 15,9, 15,10, 15,11, 15,12, 15,13, 15,14, 15,15
};
/* Initialise the palette */
static PALETTE_INIT( spectrum )
{
	palette_set_colors(machine, 0, spectrum_palette, sizeof(spectrum_palette) / 3);
	memcpy(colortable, spectrum_colortable, sizeof(spectrum_colortable));
}

static INTERRUPT_GEN( spec_interrupt )
{
	cpunum_set_input_line(0, 0, HOLD_LINE);
}



static MACHINE_DRIVER_START( spectrum )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3500000)        /* 3.5 Mhz */
	MDRV_CPU_PROGRAM_MAP(spectrum_mem, 0)
	MDRV_CPU_IO_MAP(spectrum_io, 0)
	MDRV_CPU_VBLANK_INT(spec_interrupt,1)
	MDRV_SCREEN_REFRESH_RATE(50.08)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(2500))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( spectrum )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(SPEC_SCREEN_WIDTH, SPEC_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1)
	MDRV_GFXDECODE( spectrum_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(256)
	MDRV_PALETTE_INIT( spectrum )

	MDRV_VIDEO_START( spectrum )
	MDRV_VIDEO_UPDATE( spectrum )
	MDRV_VIDEO_EOF( spectrum )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spectrum_128 )
	MDRV_IMPORT_FROM( spectrum )

	MDRV_CPU_REPLACE("main", Z80, 3500000)        /* 3.5 Mhz */
	MDRV_CPU_PROGRAM_MAP(spectrum_128_mem, 0)
	MDRV_CPU_IO_MAP(spectrum_128_io, 0)
	MDRV_SCREEN_REFRESH_RATE(50.021)

	MDRV_MACHINE_RESET( spectrum_128 )

    /* video hardware */
	MDRV_VIDEO_START( spectrum_128 )
	MDRV_VIDEO_UPDATE( spectrum_128 )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, 1773400)
	MDRV_SOUND_CONFIG(spectrum_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)	
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spectrum_plus3 )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(spectrum_plus3_io, 0)
	MDRV_SCREEN_REFRESH_RATE(50.01)

	MDRV_MACHINE_RESET( spectrum_plus3 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ts2068 )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_REPLACE("main", Z80, 3580000)        /* 3.58 Mhz */
	MDRV_CPU_PROGRAM_MAP(ts2068_mem, 0)
	MDRV_CPU_IO_MAP(ts2068_io, 0)
	MDRV_SCREEN_REFRESH_RATE(60)

	MDRV_MACHINE_RESET( ts2068 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(TS2068_SCREEN_WIDTH, TS2068_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, TS2068_SCREEN_WIDTH-1, 0, TS2068_SCREEN_HEIGHT-1)

	MDRV_VIDEO_UPDATE( ts2068 )
	MDRV_VIDEO_EOF( ts2068 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( uk2086 )
	MDRV_IMPORT_FROM( ts2068 )
	MDRV_SCREEN_REFRESH_RATE(50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tc2048 )
	MDRV_IMPORT_FROM( spectrum )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tc2048_mem, 0)
	MDRV_CPU_IO_MAP(tc2048_io, 0)
	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_MACHINE_RESET( tc2048 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(TS2068_SCREEN_WIDTH, SPEC_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, TS2068_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1)

	MDRV_VIDEO_START( spectrum_128 )
	MDRV_VIDEO_UPDATE( tc2048 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( scorpion )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(scorpion_io, 0)
	MDRV_MACHINE_RESET( scorpion )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pentagon )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(pentagon_io, 0)
	MDRV_MACHINE_RESET( pentagon )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(spectrum)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("spectrum.rom", 0x0000, 0x4000, CRC(ddee531f) SHA1(5ea7c2b824672e914525d1d5c419d71b84a426a2))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specbusy)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-busy.rom", 0x0000, 0x4000, CRC(1511cddb) SHA1(ab3c36daad4325c1d3b907b6dc9a14af483d14ec))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specpsch)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-psych.rom", 0x0000, 0x4000, CRC(cd60b589) SHA1(0853e25857d51dd41b20a6dbc8e80f028c5befaa))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specgrot)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-groot.rom", 0x0000, 0x4000, CRC(abf18c45) SHA1(51165cde68e218512d3145467074bc7e786bf307))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specimc)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-imc.rom", 0x0000, 0x4000, CRC(d1be99ee) SHA1(dee814271c4d51de257d88128acdb324fb1d1d0d))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(speclec)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("80-lec.rom", 0x0000, 0x4000, CRC(5b5c92b1) SHA1(bb7a77d66e95d2e28ebb610e543c065e0d428619))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(spec128)
	ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("zx128_0.rom",0x10000,0x4000, CRC(e76799d2) SHA1(4f4b11ec22326280bdb96e3baf9db4b4cb1d02c5))
	ROM_LOAD("zx128_1.rom",0x14000,0x4000, CRC(b96a36be) SHA1(80080644289ed93d71a1103992a154cc9802b2fa))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(spec128s)
	ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("zx128s0.rom",0x10000,0x4000, CRC(453d86b2) SHA1(968937b1c750f0ef6205f01c6db4148da4cca4e3))
	ROM_LOAD("zx128s1.rom",0x14000,0x4000, CRC(6010e796) SHA1(bea3f397cc705eafee995ea629f4a82550562f90))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specpls2)
	ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("zxp2_0.rom",0x10000,0x4000, CRC(5d2e8c66) SHA1(72703f9a3e734f3c23ec34c0727aae4ccbef9a91))
	ROM_LOAD("zxp2_1.rom",0x14000,0x4000, CRC(98b1320b) SHA1(de8b0d2d0379cfe7c39322a086ca6da68c7f23cb))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specpl2a)
	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("p2a41_0.rom",0x10000,0x4000, CRC(30c9f490) SHA1(62ec15a4af56cd1d206d0bd7011eac7c889a595d))
	ROM_LOAD("p2a41_1.rom",0x14000,0x4000, CRC(a7916b3f) SHA1(1a7812c383a3701e90e88d1da086efb0c033ac72))
	ROM_LOAD("p2a41_2.rom",0x18000,0x4000, CRC(c9a0b748) SHA1(8df145d10ff78f98138682ea15ebccb2874bf759))
	ROM_LOAD("p2a41_3.rom",0x1c000,0x4000, CRC(b88fd6e3) SHA1(be365f331942ec7ec35456b641dac56a0dbfe1f0))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specpls3)
	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("pl3-0.rom",0x10000,0x4000, CRC(17373da2) SHA1(e319ed08b4d53a5e421a75ea00ea02039ba6555b))
	ROM_LOAD("pl3-1.rom",0x14000,0x4000, CRC(f1d1d99e) SHA1(c9969fc36095a59787554026a9adc3b87678c794))
	ROM_LOAD("pl3-2.rom",0x18000,0x4000, CRC(3dbf351d) SHA1(22e50c6ba4157a3f6a821bd9937cd26e292775c6))
	ROM_LOAD("pl3-3.rom",0x1c000,0x4000, CRC(04448eaa) SHA1(65f031caa8148a5493afe42c41f4929deab26b4e))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specpls4)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("plus4.rom",0x0000,0x4000, CRC(7e0f47cb) SHA1(c103e89ef58e6ade0c01cea0247b332623bd9a30))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(tk90x)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("tk90x.rom",0x0000,0x4000, CRC(3e785f6f) SHA1(9a943a008be13194fb006bddffa7d22d2277813f))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(tk95)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("tk95.rom",0x0000,0x4000, CRC(17368e07) SHA1(94edc401d43b0e9a9cdc1d35de4b6462dc414ab3))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(inves)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("inves.rom",0x0000,0x4000, CRC(8ff7a4d1) SHA1(d020440638aff4d39467128413ef795677be9c23))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(tc2048)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("tc2048.rom",0x0000,0x4000, CRC(f1b5fa67) SHA1(febb2d495b6eda7cdcb4074935d6e9d9f328972d))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(ts2068)
	ROM_REGION(0x16000,REGION_CPU1,0)
	ROM_LOAD("ts2068_h.rom",0x10000,0x4000, CRC(bf44ec3f) SHA1(1446cb2780a9dedf640404a639fa3ae518b2d8aa))
	ROM_LOAD("ts2068_x.rom",0x14000,0x2000, CRC(ae16233a) SHA1(7e265a2c1f621ed365ea23bdcafdedbc79c1299c))
ROM_END

ROM_START(uk2086)
	ROM_REGION(0x16000,REGION_CPU1,0)
	ROM_LOAD("uk2086_h.rom",0x10000,0x4000, CRC(5ddc0ca2) SHA1(1d525fe5cdc82ab46767f665ad735eb5363f1f51))
	ROM_LOAD("ts2068_x.rom",0x14000,0x2000, CRC(ae16233a) SHA1(7e265a2c1f621ed365ea23bdcafdedbc79c1299c))
ROM_END

ROM_START(specp2fr)
	ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("plus2fr0.rom",0x10000,0x4000, CRC(c684c535) SHA1(56684c4c85a616e726a50707483b9a42d8e724ed))
	ROM_LOAD("plus2fr1.rom",0x14000,0x4000, CRC(f5e509c5) SHA1(7e398f62689c9d90a36d3a101351ec9987207308))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specp2sp)
	ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("plus2sp0.rom",0x10000,0x4000, CRC(e807d06e) SHA1(8259241b28ff85441f1bedc2bee53445767c51c5))
	ROM_LOAD("plus2sp1.rom",0x14000,0x4000, CRC(41981d4b) SHA1(ec0d5a158842d20601b4fbeaefc6668db979d0e1))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specp3sp)
	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("plus3sp0.rom",0x10000,0x4000, CRC(1f86147a) SHA1(e9b0a60a1a8def511d59090b945d175bdc646346))
	ROM_LOAD("plus3sp1.rom",0x14000,0x4000, CRC(a8ac4966) SHA1(4e48f196427596c7990c175d135c15a039c274a4))
	ROM_LOAD("plus3sp2.rom",0x18000,0x4000, CRC(f6bb0296) SHA1(09fc005625589ef5992515957ce7a3167dec24b2))
	ROM_LOAD("plus3sp3.rom",0x1c000,0x4000, CRC(f6d25389) SHA1(ec8f644a81e2e9bcb58ace974103ea960361bad2))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specpl3e)
	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("roma-en.rom",0x10000,0x8000, CRC(2d533344) SHA1(5ff2dae32eb745d87e0b54c595d1d20a866f316f))
	ROM_LOAD("romb-en.rom",0x18000,0x8000, CRC(ef8d5d92) SHA1(983aa53aa76e25a3af123c896016bacf6829b72b))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specp3es)
	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("roma-es.rom",0x10000,0x8000, CRC(ba694b4b) SHA1(d15d9e43950483cffc79f1cfa89ecb114a88f6c2))
	ROM_LOAD("romb-es.rom",0x18000,0x8000, CRC(61ed94db) SHA1(935b14c13db75d872de8ad0d591aade0adbbc355))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(scorpion)
	ROM_REGION(0x020000, REGION_CPU1, 0)
	ROM_LOAD("scorp0.rom",0x010000, 0x4000, CRC(0eb40a09))
	ROM_LOAD("scorp1.rom",0x014000, 0x4000, CRC(9d513013))
	ROM_LOAD("scorp2.rom",0x018000, 0x4000, CRC(fd0d3ce1))
	ROM_LOAD("scorp3.rom",0x01c000, 0x4000, CRC(1fe1d003))
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(pentagon)
	ROM_REGION(0x020000, REGION_CPU1, 0)
	ROM_CART_LOAD(0, "rom\0", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

static void spectrum_common_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:				info->i = 1; break;
		case DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_PLAY | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_DISABLED; break;
		case DEVINFO_PTR_CASSETTE_FORMATS:		info->p = (void *)tzx_cassette_formats; break;
		default:					cassette_device_getinfo(devclass, state, info); break;
	}
}

static void spectrum_common_snapshot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* snapshot */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sna,z80,sp"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_SNAPSHOT_LOAD:					info->f = (genf *) snapshot_load_spectrum; break;

		default:										snapshot_device_getinfo(devclass, state, info); break;
	}
}

static void spectrum_common_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "scr"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_spectrum; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(spectrum_common)
	CONFIG_DEVICE(spectrum_common_cassette_getinfo)
	CONFIG_DEVICE(spectrum_common_snapshot_getinfo)
	CONFIG_DEVICE(spectrum_common_quickload_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(spectrum)
	CONFIG_IMPORT_FROM(spectrum_common)
	CONFIG_DEVICE(cartslot_device_getinfo)
SYSTEM_CONFIG_END

static void specpls3_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		default:										legacydsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(specpls3)
	CONFIG_IMPORT_FROM(spectrum_common)
	CONFIG_DEVICE(specpls3_floppy_getinfo)
SYSTEM_CONFIG_END

static void ts2068_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_timex_cart; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_timex_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dck"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(ts2068)
	CONFIG_IMPORT_FROM(spectrum_common)
	CONFIG_DEVICE(ts2068_cartslot_getinfo)
	CONFIG_RAM_DEFAULT(48 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(tc2048)
	CONFIG_IMPORT_FROM(spectrum)
	CONFIG_RAM_DEFAULT(48 * 1024)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY		FULLNAME */
COMP( 1982, spectrum, 0,        0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum" , 0)
COMP( 2000, specpls4, spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum +4", GAME_COMPUTER_MODIFIED )
COMP( 1994, specbusy, spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum (BusySoft Upgrade v1.18)", GAME_COMPUTER_MODIFIED )
COMP( ????, specpsch, spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum (Maly's Psycho Upgrade)", GAME_COMPUTER_MODIFIED )
COMP( ????, specgrot, spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum (De Groot's Upgrade)", GAME_COMPUTER_MODIFIED )
COMP( 1985, specimc,  spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum (Collier's Upgrade)", GAME_COMPUTER_MODIFIED )
COMP( 1987, speclec,  spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Sinclair Research",	"ZX Spectrum (LEC Upgrade)", GAME_COMPUTER_MODIFIED )
COMP( 1986, inves,    spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Investronica",	"Inves Spectrum 48K+" , 0)
COMP( 1985, tk90x,    spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Micro Digital",	"TK-90x Color Computer" , 0)
COMP( 1986, tk95,     spectrum, 0,		spectrum,		spectrum,	0,		spectrum,	"Micro Digital",	"TK-95 Color Computer" , 0)
COMP( 1984, tc2048,   spectrum, 0,		tc2048,			spectrum,	0,		tc2048,		"Timex of Portugal",	"TC-2048" , 0)
COMP( 1983, ts2068,   spectrum, 0,		ts2068,			spectrum,	0,		ts2068,		"Timex Sinclair",	"TS-2068" , 0)
COMP( 1986, uk2086,   spectrum, 0,		uk2086,			spectrum,	0,		ts2068,		"Unipolbrit",	"UK-2086 ver. 1.2" , 0)

COMP( 1986, spec128,  0,		 0,		spectrum_128,	spectrum,	0,		spectrum,	"Sinclair Research",    "ZX Spectrum 128" ,GAME_NOT_WORKING)
COMP( 1985, spec128s, spec128,  0,		spectrum_128,	spectrum,	0,		spectrum,	"Sinclair Research",    "ZX Spectrum 128 (Spain)" ,GAME_NOT_WORKING)
COMP( 1986, specpls2, spec128,  0,		spectrum_128,	spectrum,	0,		spectrum,	"Amstrad plc",          "ZX Spectrum +2" ,GAME_NOT_WORKING)
COMP( 1987, specpl2a, spec128,  0,		spectrum_plus3,spectrum,	0,		specpls3,	"Amstrad plc",          "ZX Spectrum +2a" ,GAME_NOT_WORKING)
COMP( 1987, specpls3, spec128,  0,		spectrum_plus3,spectrum,	0,		specpls3,	"Amstrad plc",          "ZX Spectrum +3" ,GAME_NOT_WORKING)

COMP( 1986, specp2fr, spec128,  0,		spectrum_128,	spectrum,	0,		spectrum,	"Amstrad plc",          "ZX Spectrum +2 (France)" ,GAME_NOT_WORKING)
COMP( 1986, specp2sp, spec128,  0,		spectrum_128,	spectrum,	0,		spectrum,	"Amstrad plc",          "ZX Spectrum +2 (Spain)" ,GAME_NOT_WORKING)
COMP( 1987, specp3sp, spec128,  0,		spectrum_plus3,spectrum,	0,		specpls3,	"Amstrad plc",          "ZX Spectrum +3 (Spain)" ,GAME_NOT_WORKING)
COMP( 2000, specpl3e, spec128,  0,		spectrum_plus3,spectrum,	0,		specpls3,	"Amstrad plc",          "ZX Spectrum +3e" , GAME_NOT_WORKING|GAME_COMPUTER_MODIFIED )
COMP( 2000, specp3es, spec128,  0,		spectrum_plus3,spectrum,	0,		specpls3,	"Amstrad plc",          "ZX Spectrum +3e (Spain)" , GAME_NOT_WORKING|GAME_COMPUTER_MODIFIED )

COMP( ????, scorpion, 0,		 0,		scorpion,		spectrum,	0,		specpls3,	"Zonov and Co.",		"Zs Scorpion 256", GAME_NOT_WORKING)
COMP( ????, pentagon, spectrum, 0,		pentagon,		spectrum,	0,		specpls3,	"???",		"Pentagon", GAME_NOT_WORKING)
