/*************************************************************************************************


	Kaypro 2/83 computer - the very first Kaypro II - 2 full size floppy drives.
	Each disk was single sided, and could hold 191k. The computer had 2x pio
	and 1x sio. One of the sio ports communicated with the keyboard with a coiled
	telephone cord, complete with modular plug on each end. The keyboard carries
	its own processor and is an intelligent device.

	There are 2 major problems preventing this driver from working

	- MESS is not capable of conducting realtime serial communications between devices

	- MAME's z80sio implementation is lacking important deatures:
	-- cannot change baud rate on the fly
	-- cannot specify different rates for each channel
	-- cannot specify different rates for Receive and Transmit
	-- the callback doesn't appear to support channel B ??


	Things that need doing:
	- How to specify interrupts in callbacks when a daisy-chain is in use?
	- Which order are devices to be specified in the daisy chain?
	- Keyboard is a separate device and not emulated (includes a beeper)
	- Floppy disks (I have no knowledge of how to set these up)
	- Bank switching
	- Rom SHA1s
	- Split this code to video, machine

	This code has never been compiled. There will most likely be heaps of errors!
 
**************************************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/ctronics.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
//#include "machine/beeper.h"


static const device_config *kaypro2_z80pio_g;
static const device_config *kaypro2_z80pio_s;
static const device_config *kaypro2_z80sio;
//static const device_config *kaypro2_beeper;
static const device_config *kaypro2_printer;
static const device_config *kaypro2_fdc;

static printer_data;


/***********************************************************

	PIO

	It seems there must have been a bulk-purchase
	special on pios - port B is unused on both of them

************************************************************/

static WRITE8_DEVICE_HANDLER( pio_printer_w )
{
/*	d7..d0 output to Centronics printer */

	printer_data = data;
}

static READ8_DEVICE_HANDLER( pio_system_r )
{
/*	d3 Centronics ready flag */

	return ~centronics_busy_r(kaypro2_printer) << 3;
}

static WRITE8_DEVICE_HANDLER( pio_system_w )
{
/*	d7 bank select
	d6 disk drive motors - not emulated
	d5 double-density enable
	d4 Centronics strobe
	d1 drive B
	d0 drive A */

	/* get address space */
	const address_space *mem = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (data & 0x80)
	{
		memory_install_readwrite8_handler (mem, 0x0000, 0x3fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		memory_install_read8_handler (mem, 0x0000, 0x0fff, 0, 0, SMH_BANK1);
		memory_set_bankptr(mem->machine, 1, memory_region(mem->machine, "maincpu"+0x10000));
		memory_install_readwrite8_handler (mem, 0x3000, 0x3fff, 0, 0, kaypro2_videoram_r, kaypro2_videoram_w);
	}
	else
	{
		memory_install_readwrite8_handler (mem, 0x0000, 0x3fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
 		memory_install_readwrite8_handler (mem, 0x0000, 0x3fff, 0, 0, SMH_BANK2, SMH_BANK3);
		memory_set_bankptr(mem->machine, 2, memory_region(mem->machine, "maincpu"));
		memory_set_bankptr(mem->machine, 3, memory_region(mem->machine, "maincpu"));
	}

	wd17xx_set_density((data & 0x20) ? 1 : 0);

	if (data & 0x10)
	{
		centronics_strobe_w(kaypro2_printer, 1);
		centronics_data_w(kaypro2_printer, 0, printer_data);
	}
	else
		centronics_strobe_w(kaypro2_printer, 0);

	if (data & 1)
		wd17xx_set_drive(kaypro2_fdc, 0);
	else
	if (data & 2)
		wd17xx_set_drive(kaypro2_fdc, 1);

	if (data & 3)
		wd17xx_set_side(kaypro2_fdc, 0);	/* only has 1 side */
}

static const z80pio_interface kaypro2_pio_g_intf =
{
	DEVCB_NULL,		/* callback when change interrupt status - assume this is handled by the daisy-chain? */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pio_printer_w),	/* output data to printer */
	DEVCB_NULL,
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL			/* portB ready active callback */
};

static const z80pio_interface kaypro_pio_s_intf =
{
	DEVCB_NULL,		/* callback when change interrupt status - assume this is handled by the daisy-chain? */
	DEVCB_HANDLER(pio_system_r),	/* read printer status */
	DEVCB_NULL,
	DEVCB_HANDLER(pio_system_w),	/* activate various internal devices */
	DEVCB_NULL,
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL			/* portB ready active callback */
};

READ8_DEVICE_HANDLER( kaypro2_pio_r )
{
	if (!offset)
		return z80pio_d_r(device, 0);
	else
	if (offset == 1)
		return z80pio_c_r(device, 0);
	else
	if (offset == 2)
		return z80pio_d_r(device, 1);
	else
		return z80pio_c_r(device, 1);
}
		
WRITE8_DEVICE_HANDLER( kaypro2_pio_w )
{
	if (!offset)
		z80pio_d_w(device, 0, data);
	else
	if (offset == 1)
		z80pio_c_w(device, 0, data);
	else
	if (offset == 2)
		z80pio_d_w(device, 1, data);
	else
		z80pio_c_w(device, 1, data);
}

/***********************************************************

	SIO

************************************************************/

static WRITE8_HANDLER( kaypro2_baud_w )
{
/* Set baud rate. bits 0..3 Rx and Tx are tied together. Baud Rate Generator is a AY-5-8116.
	00h    50  
	11h    75  
	22h    110  
	33h    134.5  
	44h    150  
	55h    300  
	66h    600  
	77h    1200  
	88h    1800  
	99h    2000  
	AAh    2400  
	BBh    3600  
	CCh    4800  
	DDh    7200  
	EEh    9600  
	FFh    19200 */

	int baud_clock[]={ 800, 1200, 1760, 2152, 2400, 4800, 9600, 19200, 28800, 32000, 38400, 57600, 76800, 115200, 153600, 307200 };

	data &= 0x0f;

	if (offset < 4)
	{
	/* channel A */
//		z80sio_set_rx_clock( kaypro2_z80sio, baud_clock[data], 0);
//		z80sio_set_tx_clock( kaypro2_z80sio, baud_clock[data], 0);
	}
	else
	{
	/* channel B */
//		z80sio_set_rx_clock( kaypro2_z80sio, baud_clock[data], 1);
//		z80sio_set_tx_clock( kaypro2_z80sio, baud_clock[data], 1);
	}
}


static const z80sio_interface kaypro2_sio_intf =
{
	0,			/* interrupt handler - assume handled by daisy-chain? */
	0,				/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	0,	/* transmit handler - which channel is this for? */
	0		/* receive handler - which channel is this for? */
};

static const z80_daisy_chain kaypro2_daisy_chain[] =
{
/* Which order are these supposed to be in? assumed highest priority at the top */
	{ "z80sio" },		/* sio */
	{ "z80pio_s" },		/* System pio */
	{ "z80pio_g" },		/* General purpose pio */
	{ NULL }
};

READ8_DEVICE_HANDLER( kaypro2_sio_r )
{
	if (!offset)
		return z80sio_d_r(device, 0);
	else
	if (offset == 1)
		return z80sio_d_r(device, 1);
	else
	if (offset == 2)
		return z80sio_c_r(device, 0);
	else
		return z80sio_c_r(device, 1);
}
		
WRITE8_DEVICE_HANDLER( kaypro2_sio_w )
{
	if (!offset)
		z80sio_d_w(device, 0, data);
	else
	if (offset == 1)
		z80sio_d_w(device, 1, data);
	else
	if (offset == 2)
		z80sio_c_w(device, 0, data);
	else
		z80sio_c_w(device, 1, data);
}

/***********************************************************

	Address Maps

************************************************************/

static ADDRESS_MAP_START( kaypro2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1000, 0xffff) AM_RAM AM_REGION("maincpu", 0x1000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kaypro2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_WRITE(kaypro2_baud_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("z80sio", kaypro2_sio_r, kaypro2_sio_w)
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("z80pio_g", kaypro2_pio_r, kaypro2_pio_w)
	AM_RANGE(0x0c, 0x0f) AM_WRITE(kaypro2_baud_w)
	AM_RANGE(0x10, 0x10) AM_DEVREADWRITE("wd1793", wd17xx_status_r, wd17xx_command_w)
	AM_RANGE(0x11, 0x11) AM_DEVREADWRITE("wd1793", wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0x12, 0x12) AM_DEVREADWRITE("wd1793", wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0x13, 0x13) AM_DEVREADWRITE("wd1793", wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0x1c, 0x1f) AM_DEVREADWRITE("z80pio_s", kaypro2_pio_r, kaypro2_pio_w)
ADDRESS_MAP_END

/***********************************************************

	Keyboard Layout

************************************************************/

static INPUT_PORTS_START( kaypro2 )
INPUT_PORTS_END

/***********************************************************

	Video

************************************************************/

static PALETTE_INIT( kaypro2 )
{
	palette_set_color(machine,0,RGB_BLACK); /* black */
	palette_set_color(machine,1,MAKE_RGB(0, 200, 0)); /* green */	
}

static VIDEO_UPDATE( kaypro2 )
{
/* Bit 7 is the blink attribute */
	static UINT8 framecnt=0;	/* exact flash rate not documented anywhere */
	UINT8 y,ra,chr,gfx,gfxbit;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = memory_region(screen->machine, "gfx1");

	framecnt++;

	for (y = 0; y < 24; y++)			/* could be 25 lines, depending on which document you read */
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x+=skip)
			{
				chr = videoram[x];

				if ((chr < 0x80) || (framecnt & 0x10))
					chr |= 0x80;

				/* get pattern of pixels for that character scanline */
				if (ra < 8) 
					gfx = FNT[(chr<<3) | ra ];
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p = 0; p++;
				*p = ( gfx & 0x10 ) ? 0 : 1; p++;
				*p = ( gfx & 0x08 ) ? 0 : 1; p++;
				*p = ( gfx & 0x04 ) ? 0 : 1; p++;
				*p = ( gfx & 0x02 ) ? 0 : 1; p++;
				*p = ( gfx & 0x01 ) ? 0 : 1; p++;
				*p = 0; p++;
				}
			}
		}
		ma+=80;
	}
	return 0;
}

/***********************************************************

	Machine

************************************************************/

static MACHINE_RESET( kaypro2 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	kaypro2_z80pio_g = devtag_get_device(machine, "z80pio_g");
	kaypro2_z80pio_s = devtag_get_device(machine, "z80pio_s");
//	kaypro2_beeper = devtag_get_device(machine, "beeper");
	kaypro2_printer = devtag_get_device(machine, "centronics");
	kaypro2_fdc = devtag_get_device(machine, "wd1793");
	pio_system_w(kaypro_z80pio_s, 0, 0x80);
}

/***********************************************************

	Machine Driver

************************************************************/

static MACHINE_DRIVER_START( kaypro2 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 2500000)	/* 2.5 MHz */
	MDRV_CPU_PROGRAM_MAP(kaypro2_map, 0)
	MDRV_CPU_IO_MAP(kaypro2_io, 0)

	MDRV_MACHINE_RESET( kaypro2 )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 25*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*8-1,0,25*10-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kaypro2)

	MDRV_VIDEO_UPDATE( kaypro2 )

	/* sound hardware */
//	MDRV_SPEAKER_STANDARD_MONO("mono")
//	MDRV_SOUND_ADD("beeper", BEEPER, 0)
//	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_WD179X_ADD("wd1793", kaypro2_wd1793_interface )
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_Z80PIO_ADD( "z80pio_g", kaypro2_pio_g_intf )
	MDRV_Z80PIO_ADD( "z80pio_s", kaypro2_pio_s_intf )
	MDRV_Z80SIO_ADD( "z80sio", 4800, kaypro2_sio_intf )	/* start at 300 baud */
MACHINE_DRIVER_END

/***********************************************************

	Game driver

************************************************************/

ROM_START(kaypro2) /* wrong sha1's here */
	/* The board could take a 2716 or 2732 */
	ROM_REGION(0x12000, "maincpu",0)
	ROM_LOAD("kaypro2.u47", 0x10000, 0x0800, CRC(28264bc1) SHA1(e84cf3121f9e0bb9e1b01b095f7a9581dcfaaae4))
//	ROM_LOAD("kaypro2.u47", 0x10000, 0x1000, CRC(4918fb91) SHA1(e84cf3121f9e0bb9e1b01b095f7a9581dcfaaae4))

	ROM_REGION(0x01000, "gfx1",0)
	ROM_LOAD("kaypro2.u43", 0x00000, 0x0800, CRC(4cc7d206) SHA1(6361ee9667bf59d50059d09b0baf8672fdb2e8af))
ROM_END

static DRIVER_INIT( lnw80 )
{
	videoram = memory_region(machine, "maincpu")+0x11000;
}

/* I have no idea how to set up floppies - these ones have 40 tracks, 10 sectors, 512 bytes per track, single sided */

static void kaypro2_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:			info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:			info->load = DEVICE_IMAGE_LOAD_NAME(kaypro2_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "dsk"); break;

		default:					legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(kaypro2)
	CONFIG_DEVICE(kaypro2_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE	  INPUT    INIT      CONFIG       COMPANY  FULLNAME */
COMP( 1983, kaypro2,  0,      0,      kaypro2,    kaypro2, 0,        kaypro2,	  "Non Linear Systems",  "Kaypro 2/83" , GAME_NOT_WORKING )
