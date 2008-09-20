/*

Megadrive / Genesis Rewrite, Take 65498465432356345250432.3  August 06

Thanks to:
Charles Macdonald for much useful information (cgfm2.emuviews.com)

Long Description names mostly taken from the GoodGen database

ToDo:

The Code here is terrible for now, this is just for testing
Fix HV Counter & Raster Implementation (One line errors in some games, others not working eg. Dracula)
Fix Horizontal timings (currently a kludge, currently doesn't change with resolution changes)
Add Real DMA timings (using a timer)
Add All VDP etc. Mirror addresses (not done yet as I prefer to catch odd cases for now)
Investigate other Bugs (see list below)
Rewrite (again) using cleaner, more readable and better optimized code with knowledge gained
Add support for other peripherals (6 player pad, Teamplay Adapters, Lightguns, Sega Mouse etc.)
Sort out set info, making sure all games have right manufacturers, dates etc.
Make sure everything that needs backup RAM has it setup and working
Fix Reset glitches
Add 32X / SegaCD support
Add Megaplay, Megatech support (needs SMS emulation too)
Add other obscure features (palette flicker for mid-screen CRAM changes, undocumented register behavior)
Figure out how sprite masking *really* works
Add EEprom support in games that need it

Known Issues:
    Bass Masters Classic Pro Edition (U) [!] - Sega Logo is corrupt
    Bram Stoker's Dracula (U) [!] - Doesn't Work (HV Timing)
    Double Dragon 2 - The Revenge (J) [!] - Too Slow?
    International Superstar Soccer Deluxe (E) [!] - Single line Raster Glitch
    Lemmings (JU) (REV01) [!] - Rasters off by ~7-8 lines (strange case)
    Mercs - Sometimes sound doesn't work..

    Some beta games without proper sound programs seem to crash because the z80 crashes

Known Non-Issues (confirmed on Real Genesis)
    Castlevania - Bloodlines (U) [!] - Pause text is missing on upside down level
    Blood Shot (E) (M4) [!] - corrupt texture in level 1 is correct...

----------------------

Cartridge support by Gareth S. Long

MESS adaptation by R. Belmont

*/

#include "driver.h"
#include "sound/2612intf.h"
#include "../../mame/drivers/megadriv.h"
#include "deprecat.h"

/* cart device, custom mappers & sram init */
#include "includes/genesis.h"


static int megadrive_region_export = 0;
static int megadrive_region_pal = 0;


/*************************************
 *
 *  Input handlers
 *
 *************************************/

/* We need to always initialize 6 buttons pad */
static emu_timer *mess_io_timeout[3];
static int mess_io_stage[3];

static TIMER_CALLBACK( mess_io_timeout_timer_callback )
{
	mess_io_stage[(int)(FPTR)ptr] = -1;
}

static void mess_init_6buttons_pad(void)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		mess_io_timeout[i] = timer_alloc(mess_io_timeout_timer_callback, (void*)(FPTR)i);
		mess_io_stage[i] = -1;
	}
}

/* These overwrite the MAME ones in DRIVER_INIT */
/* They're needed to give the users the choice between different controllers */
static UINT8 mess_md_io_read_data_port(int portnum)
{
	static const char *const pad6names[2][4] = {{ "PAD1_6B", "PAD2_6B", "UNUSED", "UNUSED" }, 
												{ "EXTRA1", "EXTRA2", "UNUSED", "UNUSED" }};
	static const char *const pad3names[4] = { "PAD1_3B", "PAD2_3B", "UNUSED", "UNUSED" };

	UINT8 retdata; 
	int controller;
	UINT8 helper_6b = (megadrive_io_ctrl_regs[portnum] & 0x3f) | 0xc0; // bits 6 & 7 always come from megadrive_io_data_regs
	UINT8 helper_3b = (megadrive_io_ctrl_regs[portnum] & 0x7f) | 0x80; // bit 7 always comes from megadrive_io_data_regs

	switch (portnum)
	{
		case 0:
			controller = (input_port_read(Machine, "CTRLSEL") & 0x0f);
			break;

		case 1:
			controller = (input_port_read(Machine, "CTRLSEL") & 0xf0);
			break;

		default:
			controller = 0;
			break;
	}

	/* Are we using a 6 buttons Joypad? */
	if (controller)
	{
		if (megadrive_io_data_regs[portnum] & 0x40)
		{
			if (mess_io_stage[portnum] == 2)
			{
				/* here we read B, C & the additional buttons */
				retdata = (megadrive_io_data_regs[portnum] & helper_6b) | 
							(((input_port_read_safe(Machine, pad6names[0][portnum], 0) & 0x30) | 
								(input_port_read_safe(Machine, pad6names[1][portnum], 0) & 0x0f)) & ~helper_6b);
			}
			else
			{
				/* here we read B, C & the directional buttons */
				retdata = (megadrive_io_data_regs[portnum] & helper_6b) | 
							((input_port_read_safe(Machine, pad6names[0][portnum], 0) & 0x3f) & ~helper_6b);
			}
		}
		else
		{
			if (mess_io_stage[portnum] == 1)
			{
				/* here we read ((Start & A) >> 2) | 0x00 */
				retdata = (megadrive_io_data_regs[portnum] & helper_6b) | 
							(((input_port_read_safe(Machine, pad6names[0][portnum], 0) & 0xc0) >> 2) & ~helper_6b);
			}
			else if (mess_io_stage[portnum]==2)
			{
				/* here we read ((Start & A) >> 2) | 0x0f */
				retdata = (megadrive_io_data_regs[portnum] & helper_6b) | 
							((((input_port_read_safe(Machine, pad6names[0][portnum], 0) & 0xc0) >> 2) | 0x0f) & ~helper_6b);
			}
			else
			{
				/* here we read ((Start & A) >> 2) | Up and Down */
				retdata = (megadrive_io_data_regs[portnum] & helper_6b) | 
							((((input_port_read_safe(Machine, pad6names[0][portnum], 0) & 0xc0) >> 2) | 
								(input_port_read_safe(Machine, pad6names[0][portnum], 0) & 0x03)) & ~helper_6b);
			}
		}

	//  mame_printf_debug("read io data port stage %d port %d %02x\n",mess_io_stage[portnum],portnum,retdata);

		retdata |= (retdata << 8);
	}
	/* Otherwise it's a 3 buttons Joypad */
	else
	{
		if (megadrive_io_data_regs[portnum] & 0x40)
		{
			/* here we read B, C & the directional buttons */
			retdata = (megadrive_io_data_regs[portnum] & helper_3b) | 
						(((input_port_read_safe(Machine, pad3names[portnum], 0) & 0x3f) | 0x40) & ~helper_3b);
		}
		else
		{
			/* here we read ((Start & A) >> 2) | Up and Down */
			retdata = (megadrive_io_data_regs[portnum] & helper_3b) | 
						((((input_port_read_safe(Machine, pad3names[portnum], 0) & 0xc0) >> 2) | 
							(input_port_read_safe(Machine, pad3names[portnum], 0) & 0x03) | 0x40) & ~helper_3b);
		}
	}

	return retdata;
}


static void mess_md_io_write_data_port(int portnum, UINT16 data)
{
	int controller;

	switch (portnum)
	{
		case 0:
			controller = (input_port_read(Machine, "CTRLSEL") & 0x0f);
			break;

		case 1:
			controller = (input_port_read(Machine, "CTRLSEL") & 0xf0);
			break;

		default:
			controller = 0;
			break;
	}

	if (controller)
	{
		if (megadrive_io_ctrl_regs[portnum] & 0x40)
		{
			if (((megadrive_io_data_regs[portnum] & 0x40) == 0x00) && ((data & 0x40) == 0x40))
			{
				mess_io_stage[portnum]++;
				timer_adjust_oneshot(mess_io_timeout[portnum], ATTOTIME_IN_CYCLES(8192,0), 0);
			}

		}
	}
	megadrive_io_data_regs[portnum] = data;
	//mame_printf_debug("Writing IO Data Register #%d data %04x\n",portnum,data);
}


/*************************************
 *
 *  Input ports
 *
 *************************************/


INPUT_PORTS_START( md )
	PORT_START("CTRLSEL")	/* Controller selection */
	PORT_CATEGORY_CLASS( 0x0f, 0x00, "Player 1 Controller" )
	PORT_CATEGORY_ITEM( 0x00, "Joystick 3 Buttons", 10 )
	PORT_CATEGORY_ITEM( 0x01, "Joystick 6 Buttons", 11 )
//	PORT_CATEGORY_ITEM( 0x02, "Sega Mouse", 12 )
/* there exists both a 2 buttons version of the Mouse (Jpn ver, to be used with RPGs, it
	can aslo be used as trackball) and a 3 buttons version (US ver, no trackball feats.) */
//	PORT_CATEGORY_ITEM( 0x03, "Sega Menacer", 13 )
//	PORT_CATEGORY_ITEM( 0x04, "Konami Justifier", 14 )
//	PORT_CATEGORY_ITEM( 0x05, "Team Player (Sega Multitap)", 15 )
//	PORT_CATEGORY_ITEM( 0x06, "4-Play (EA Multitap)", 16 )
//	PORT_CATEGORY_ITEM( 0x07, "J-Cart", 17 )
	PORT_CATEGORY_CLASS( 0xf0, 0x00, "Player 2 Controller" )
	PORT_CATEGORY_ITEM( 0x00, "Joystick 3 Buttons", 20 )
	PORT_CATEGORY_ITEM( 0x10, "Joystick 6 Buttons", 21 )

	PORT_START("PAD1_3B")		/* Joypad 1 (3 button + start) NOT READ DIRECTLY */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(10)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(10)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(10)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(10)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 B") PORT_CATEGORY(10)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 C") PORT_CATEGORY(10)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 A") PORT_CATEGORY(10)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START ) PORT_PLAYER(1) PORT_CATEGORY(10)

	PORT_START("PAD2_3B")		/* Joypad 2 (3 button + start) NOT READ DIRECTLY */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(20)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(20)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(20)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(20)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 B") PORT_CATEGORY(20)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 C") PORT_CATEGORY(20)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 A") PORT_CATEGORY(20)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START ) PORT_PLAYER(2) PORT_CATEGORY(20)

	PORT_START("PAD1_6B")		/* Joypad 1 (6 button + start + mode) NOT READ DIRECTLY */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1) PORT_CATEGORY(11)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 B") PORT_CATEGORY(11)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 C") PORT_CATEGORY(11)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 A") PORT_CATEGORY(11)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START ) PORT_PLAYER(1) PORT_CATEGORY(11)

	PORT_START("EXTRA1")	/* Extra buttons for Joypad 1 (6 button + start + mode) NOT READ DIRECTLY */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1) PORT_NAME("P1 Z") PORT_CATEGORY(11)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1) PORT_NAME("P1 Y") PORT_CATEGORY(11)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 X") PORT_CATEGORY(11)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(1) PORT_NAME("P1 Mode") PORT_CATEGORY(11)

	PORT_START("PAD2_6B")		/* Joypad 2 (6 button + start + mode) NOT READ DIRECTLY */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2) PORT_CATEGORY(21)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 B") PORT_CATEGORY(21)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 C") PORT_CATEGORY(21)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 A") PORT_CATEGORY(21)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START ) PORT_PLAYER(2) PORT_CATEGORY(21)

	PORT_START("EXTRA2")	/* Extra buttons for Joypad 2 (6 button + start + mode) NOT READ DIRECTLY */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2) PORT_NAME("P2 Z") PORT_CATEGORY(21)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2) PORT_NAME("P2 Y") PORT_CATEGORY(21)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 X") PORT_CATEGORY(21)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(2) PORT_NAME("P2 Mode") PORT_CATEGORY(21)

	PORT_START("RESET")		/* Buttons on Genesis Console */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_NAME("Reset Button") PORT_IMPULSE(1) // reset, resets 68k (and..?)
INPUT_PORTS_END


INPUT_PORTS_START( md_svp )
	PORT_INCLUDE( md )

	/* We need this as long as we only have the US version of the SVP add-on,
	  otherwise we could not play Non-US Virtua Racing versions  */
	PORT_START("REGION")
	/* Region setting for Console */
	PORT_CONFNAME( 0x000f, 0x0000, DEF_STR( Region ) )
	PORT_CONFSETTING(      0x0000, "Use Default Choice" )
	PORT_CONFSETTING(      0x0001, "US (NTSC, 60fps)" )
	PORT_CONFSETTING(      0x0002, "Japan (NTSC, 60fps)" )
	PORT_CONFSETTING(      0x0003, "Europe (PAL, 50fps)" )
INPUT_PORTS_END


/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_RESET( ms_megadriv )
{
	MACHINE_RESET_CALL( megadriv );
	MACHINE_RESET_CALL( md_mappers );
	
	mess_init_6buttons_pad();
}

static MACHINE_DRIVER_START( ms_megadriv )
	MDRV_IMPORT_FROM(megadriv)

	MDRV_MACHINE_RESET( ms_megadriv )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ms_megdsvp )
	MDRV_IMPORT_FROM(megdsvp)

	MDRV_MACHINE_RESET( ms_megadriv )
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/


/* we don't use the bios rom (it's not needed and only provides security on early models) */

ROM_START(genesis)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(gensvp)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(megadriv)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(megadrij)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END


/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( genesis )
{
	DRIVER_INIT_CALL(gencommon);
	DRIVER_INIT_CALL(megadriv);
	
	megadrive_io_read_data_port_ptr	= mess_md_io_read_data_port;
	megadrive_io_write_data_port_ptr = mess_md_io_write_data_port;
}

static DRIVER_INIT( gensvp )
{
	DRIVER_INIT_CALL(gencommon);
	DRIVER_INIT_CALL(megadsvp);
}

static DRIVER_INIT( md_eur )
{
	DRIVER_INIT_CALL(gencommon);
	DRIVER_INIT_CALL(megadrie);
}

static DRIVER_INIT( md_jpn )
{
	DRIVER_INIT_CALL(gencommon);
	DRIVER_INIT_CALL(megadrij);
}


/*************************************
 *
 *	System configuration(s)
 *
 *************************************/


static SYSTEM_CONFIG_START( genesis )
	CONFIG_DEVICE(genesis_cartslot_getinfo)
SYSTEM_CONFIG_END




/****************************************** PICO emulation ****************************************/

/*
   Pico Implementation By ElBarto (Emmanuel Vadot, elbarto@megadrive.org)
   Still missing the PCM custom chip
   Some game will not boot due to this

 Pico Info from Notaz (http://notaz.gp2x.de/docs/picodoc.txt)

 addr   acc   description
-------+-----+------------
800001  byte  Version register.
              ?vv? ????, where v can be:
                00 - hardware is for Japan
                01 - European version
                10 - USA version
                11 - ?
800003  byte  Buttons, 0 for pressed, 1 for released:
                bit 0: UP (white)
                bit 1: DOWN (orange)
                bit 2: LEFT (blue)
                bit 3: RIGHT (green)
                bit 4: red button
                bit 5: unused?
                bit 6: unused?
                bit 7: pen button
800005  byte  Most significant byte of pen x coordinate.
800007  byte  Least significant byte of pen x coordinate.
800009  byte  Most significant byte of pen y coordinate.
80000b  byte  Least significant byte of pen y coordinate.
80000d  byte  Page register. One bit means one uncovered page sensor.
                00 - storyware closed
                01, 03, 07, 0f, 1f, 3f - pages 1-6
                either page 5 or page 6 is often unused.
800010  word  PCM data register.
        r/w   read returns free bytes left in PCM FIFO buffer
              writes write data to buffer.
800012  word  PCM control register.
        r/w   For writes, it has following possible meanings:
              ?p?? ???? ???? ?rrr
                p - set to enable playback?
                r - sample rate / PCM data type?
                  0: 8kHz 4bit ADPCM?
                  1-7: 16kHz variants?
              For reads, if bit 15 is cleared, it means PCM is 'busy' or
              something like that, as games sometimes wait for it to become 1.
800019  byte  Games write 'S'
80001b  byte  Games write 'E'
80001d  byte  Games write 'G'
80001f  byte  Games write 'A'

*/

#define PICO_PENX	1
#define PICO_PENY	2

static UINT16 pico_read_penpos(int pen)
{
  UINT16 penpos;

  switch (pen)
    {
    case PICO_PENX:
      penpos = input_port_read_safe(Machine, "PENX", 0);
      penpos |= 0x6;
      penpos = penpos * 320 / 255;
      penpos += 0x3d;
      break;
    case PICO_PENY:
      penpos = input_port_read_safe(Machine, "PENY", 0);
      penpos |= 0x6;
      penpos = penpos * 251 / 255;
      penpos += 0x1fc;
      break;
    }
  return penpos;
}

static READ16_HANDLER( pico_68k_io_read )
{
	UINT8 retdata;
	static UINT8 page_register = 0;


	retdata = 0;

	switch (offset)
	  {
	  case 0:	/* Version register ?XX?????? where XX is 00 for japan, 01 for europe and 10 for USA*/
		retdata = (megadrive_region_export << 6) | (megadrive_region_pal << 5);
	    break;
	  case 1:
	    retdata = input_port_read_safe(Machine, "PAD", 0);
	    break;

	    /*
	       Still notes from notaz for the pen :

	       The pen can be used to 'draw' either on the drawing pad or on the storyware
	       itself. Both storyware and drawing pad are mapped on single virtual plane, where
	       coordinates range:

	       x: 0x03c - 0x17c
	       y: 0x1fc - 0x2f7 (drawing pad)
	          0x2f8 - 0x3f3 (storyware)
	     */
	  case 2:
	    retdata = pico_read_penpos(PICO_PENX) >> 8;
	    break;
	  case 3:
	    retdata = pico_read_penpos(PICO_PENX) & 0x00ff;
	    break;
	  case 4:
	    retdata = pico_read_penpos(PICO_PENY) >> 8;
	    break;
	  case 5:
	    retdata = pico_read_penpos(PICO_PENY) & 0x00ff;
	    break;
	  case 6:
	    /* Page register :
	       00 - storyware closed
	       01, 03, 07, 0f, 1f, 3f - pages 1-6
	       either page 5 or page 6 is often unused.
	    */
	    {
	      UINT8 tmp;

	      tmp = input_port_read_safe(Machine, "PAGE", 0);
	      if (tmp == 2 && page_register != 0x3f)
		{
		  page_register <<= 1;
		  page_register |= 1;
		}
	      if (tmp == 1 && page_register != 0x00)
		page_register >>= 1;
	      retdata = page_register;
	      break;
	    }
	  case 7:
	    /* Returns free bytes left in the PCM FIFO buffer */
	    retdata = 0x00;
	    break;
	  case 8:
	    /*
	       For reads, if bit 15 is cleared, it means PCM is 'busy' or
	       something like that, as games sometimes wait for it to become 1.
	    */
	    retdata = 0x00;
	  }
	return retdata | retdata <<8;
}

static WRITE16_HANDLER( pico_68k_io_write )
{
	switch (offset)
	  {
	  }
}

static ADDRESS_MAP_START( _pico_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000 , 0x3fffff) AM_READ(SMH_ROM)

	AM_RANGE(0x800000 , 0x80001f) AM_READ(pico_68k_io_read)

	AM_RANGE(0xc00000 , 0xc0001f) AM_READ(megadriv_vdp_r)
	AM_RANGE(0xe00000 , 0xe0ffff) AM_READ(SMH_RAM) AM_MIRROR(0x1f0000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( _pico_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000 , 0x3fffff) AM_WRITE(SMH_ROM)

	AM_RANGE(0x800000 , 0x80001f) AM_WRITE(pico_68k_io_write)

	AM_RANGE(0xc00000 , 0xc0001f) AM_WRITE(megadriv_vdp_w)
	AM_RANGE(0xe00000 , 0xe0ffff) AM_WRITE(SMH_RAM) AM_MIRROR(0x1f0000) AM_BASE(&megadrive_ram)
ADDRESS_MAP_END


INPUT_PORTS_START( pico )
	PORT_START("PAD")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("Red Button")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("Pen Button")

	PORT_START("PAGE")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("Increment Page")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("Decrement Page")

	PORT_START("PENX")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_MINMAX(0, 255) PORT_CATEGORY(5) PORT_PLAYER(1) PORT_NAME("PEN X")

	PORT_START("PENY")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_MINMAX(0,255 ) PORT_CATEGORY(5) PORT_PLAYER(1) PORT_NAME("PEN Y")

	PORT_START("REGION")
	/* Region setting for Console */
	PORT_DIPNAME( 0x000f, 0x0000, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, "Use HazeMD Default Choice" )
	PORT_DIPSETTING(      0x0001, "US (NTSC, 60fps)" )
	PORT_DIPSETTING(      0x0002, "JAPAN (NTSC, 60fps)" )
	PORT_DIPSETTING(      0x0003, "EUROPE (PAL, 50fps)" )
INPUT_PORTS_END


MACHINE_DRIVER_START( pico )
	MDRV_IMPORT_FROM(megadriv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(_pico_readmem,_pico_writemem)

	MDRV_MACHINE_RESET( ms_megadriv )
MACHINE_DRIVER_END



ROM_START( pico )
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START( picou )
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START( picoj )
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END


static SYSTEM_CONFIG_START( pico )
	CONFIG_DEVICE(pico_cartslot_getinfo)
SYSTEM_CONFIG_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME		PARENT		COMPAT  MACHINE    	  INPUT     INIT  		CONFIG		COMPANY   FULLNAME */
CONS( 1989, genesis,	0,			0,      ms_megadriv,  md,		genesis,	genesis,	"Sega",   "Genesis (USA, NTSC)", 0)
CONS( 1993, gensvp,		genesis,	0,      ms_megdsvp,   md_svp,	gensvp,		genesis,	"Sega",   "Genesis (USA, NTSC, w/SVP)", 0)
CONS( 1990, megadriv,	genesis,	0,      ms_megadriv,  md,		md_eur,		genesis,	"Sega",   "Mega Drive (Europe, PAL)", 0)
CONS( 1988, megadrij,	genesis,	0,      ms_megadriv,  md,		md_jpn,		genesis,	"Sega",   "Mega Drive (Japan, NTSC)", 0)
CONS( 1994, pico,		0,			0,      pico,	      pico,		md_eur,		pico,		"Sega",   "Pico (Europe, PAL)", 0)
CONS( 1994, picou,		pico,		0,      pico,	      pico,		genesis,	pico,		"Sega",   "Pico (USA, NTSC)", 0)
CONS( 1993, picoj,		pico,		0,      pico,	      pico,		md_jpn,		pico,		"Sega",   "Pico (Japan, NTSC)", 0)
