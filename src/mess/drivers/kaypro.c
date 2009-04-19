/*************************************************************************************************


	Kaypro 2/83 computer - the very first Kaypro II - 2 full size floppy drives.
	Each disk was single sided, and could hold 191k. The computer had 2x pio
	and 1x sio. One of the sio ports communicated with the keyboard with a coiled
	telephone cord, complete with modular plug on each end. The keyboard carries
	its own Intel 8748 processor and is an intelligent device.

	There are 2 major problems preventing this driver from working

	- MESS is not capable of conducting realtime serial communications between devices

	- MAME's z80sio implementation is lacking important deatures:
	-- cannot change baud rate on the fly
	-- cannot specify different rates for each channel
	-- cannot specify different rates for Receive and Transmit
	-- the callback doesn't appear to support channel B ??


	Things that need doing:
	- Floppy disks (I have no knowledge of how to set these up)


**************************************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/ctronics.h"
#include "machine/kay_kbd.h"
#include "machine/wd17xx.h"
#include "sound/beep.h"
#include "video/mc6845.h"
#include "includes/kaypro.h"



/***********************************************************

	Address Maps

************************************************************/

static ADDRESS_MAP_START( kayproii_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM AM_REGION("maincpu", 0x0000)
	AM_RANGE(0x3000, 0x3fff) AM_RAM AM_REGION("maincpu", 0x3000) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0xffff) AM_RAM AM_REGION("rambank", 0x4000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kayproii_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_WRITE(kaypro2_baud_a_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("z80sio", kaypro2_sio_r, kaypro2_sio_w)
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("z80pio_g", kaypro2_pio_r, kaypro2_pio_w)
	AM_RANGE(0x0c, 0x0f) AM_WRITE(kaypro2_baud_b_w)
	AM_RANGE(0x10, 0x10) AM_DEVREADWRITE("wd1793", wd17xx_status_r, wd17xx_command_w)
	AM_RANGE(0x11, 0x11) AM_DEVREADWRITE("wd1793", wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0x12, 0x12) AM_DEVREADWRITE("wd1793", wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0x13, 0x13) AM_DEVREADWRITE("wd1793", wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0x1c, 0x1f) AM_DEVREADWRITE("z80pio_s", kaypro2_pio_r, kaypro2_pio_w)
ADDRESS_MAP_END



/***********************************************************

	Machine Driver

************************************************************/

static MACHINE_DRIVER_START( kayproii )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 2500000)	/* 2.5 MHz */
	MDRV_CPU_PROGRAM_MAP(kayproii_map, 0)
	MDRV_CPU_IO_MAP(kayproii_io, 0)
	MDRV_CPU_VBLANK_INT("screen", kay_kbd_interrupt)	/* this doesn't actually exist, it is to run the keyboard */

	MDRV_MACHINE_RESET( kayproii )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*7, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*7-1,0,24*10-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kaypro)

	MDRV_VIDEO_UPDATE( kayproii )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_WD179X_ADD("wd1793", kaypro2_wd1793_interface )
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_Z80PIO_ADD( "z80pio_g", kaypro2_pio_g_intf )
	MDRV_Z80PIO_ADD( "z80pio_s", kaypro2_pio_s_intf )
	MDRV_Z80SIO_ADD( "z80sio", 4800, kaypro2_sio_intf )	/* start at 300 baud */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( omni2 )
	MDRV_IMPORT_FROM( kayproii )
	MDRV_VIDEO_UPDATE( omni2 )
MACHINE_DRIVER_END
#if 0
static MACHINE_DRIVER_START( kaypro2x )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(kaypro2x_map, 0)
	MDRV_CPU_IO_MAP(kaypro2x_io, 0)
	MDRV_CPU_VBLANK_INT("screen", kay_kbd_interrupt)	/* this doesn't actually exist, it is to run the keyboard */

	MDRV_MACHINE_RESET( kaypro2x )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*7, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*7-1,0,24*10-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kaypro)

	MDRV_VIDEO_UPDATE( kaypro2x )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_WD179X_ADD("wd1793", kaypro2_wd1793_interface )
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_Z80SIO_ADD( "z80sio", 4800, kaypro2_sio_intf )	/* same job as in kayproii */
	MDRV_Z80SIO_ADD( "z80sio_2", 4800, kaypro2_sio_2_intf )	/* extra sio for the modem */
MACHINE_DRIVER_END
#endif
/***********************************************************

	Game driver

************************************************************/

/* The roms need to be renamed to their part numbers (81-xxx) when known */

ROM_START(kayproii)
	/* The board could take a 2716 or 2732 */
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-149.u47",   0x0000, 0x0800, CRC(28264bc1) SHA1(a12afb11a538fc0217e569bc29633d5270dfa51b) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD("81-146.u43",   0x0000, 0x0800, CRC(4cc7d206) SHA1(5cb880083b94bd8220aac1f87d537db7cfeb9013) )
ROM_END

ROM_START(kaypro4)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-232.u47",   0x0000, 0x1000, CRC(4918fb91) SHA1(cd9f45cc3546bcaad7254b92c5d501c40e2ef0b2) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD("81-146.u43",   0x0000, 0x0800, CRC(4cc7d206) SHA1(5cb880083b94bd8220aac1f87d537db7cfeb9013) )
ROM_END

ROM_START(omni2)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("omni2.u47",    0x0000, 0x1000, CRC(2883f9e0) SHA1(d98c784e62853582d298bf7ca84c75872847ac9b) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD("omni2.u43",    0x0000, 0x0800, CRC(049b3381) SHA1(46f1d4f038747ba9048b075dc617361be088f82a) )
ROM_END

ROM_START(kaypro2x)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-292.u34",   0x0000, 0x2000, CRC(5eb69aec) SHA1(525f955ca002976e2e30ac7ee37e4a54f279fe96) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD("81-817.u9",    0x0000, 0x1000, CRC(5f72da5b) SHA1(8a597000cce1a7e184abfb7bebcb564c6bf24fb7) )
ROM_END

ROM_START(kaypro10)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-302.u42",   0x0000, 0x1000, CRC(3f9bee20) SHA1(b29114a199e70afe46511119b77a662e97b093a0) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD("81-817.u31",   0x0000, 0x1000, CRC(5f72da5b) SHA1(8a597000cce1a7e184abfb7bebcb564c6bf24fb7) )
ROM_END


static SYSTEM_CONFIG_START(kaypro2)
	CONFIG_DEVICE(kaypro2_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE	  INPUT    INIT      CONFIG       COMPANY  FULLNAME */
COMP( 1983, kayproii, 0,        0,      kayproii, kay_kbd, 0,        kaypro2,	  "Non Linear Systems",  "Kaypro II - 2/83" , GAME_NOT_WORKING )
COMP( 198?, kaypro4,  kayproii, 0,      kayproii, kay_kbd, 0,        kaypro2,	  "Non Linear Systems",  "Kaypro 4 - 4/83" , GAME_NOT_WORKING )
COMP( 198?, omni2,    kayproii, 0,      omni2,    kay_kbd, 0,        kaypro2,	  "Non Linear Systems",  "Omni II" , GAME_NOT_WORKING )
COMP( 1984, kaypro2x, 0,        0,      kayproii, kay_kbd, 0,        kaypro2,	  "Non Linear Systems",  "Kaypro 2x" , GAME_NOT_WORKING )
COMP( 198?, kaypro10, 0,        0,      kayproii, kay_kbd, 0,        kaypro2,	  "Non Linear Systems",  "Kaypro 10" , GAME_NOT_WORKING )
