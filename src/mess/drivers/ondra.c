/***************************************************************************

        Ondra driver by Miodrag Milanovic

        08/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/ondra.h"


/* Address maps */
static ADDRESS_MAP_START(ondra_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK(1)
	AM_RANGE(0x0800, 0x0fff) AM_RAMBANK(2)
	AM_RANGE(0x1000, 0x17ff) AM_RAMBANK(3)
	AM_RANGE(0x1800, 0x1fff) AM_RAMBANK(4)
	AM_RANGE(0x2000, 0x27ff) AM_RAMBANK(5)
	AM_RANGE(0x2800, 0x2fff) AM_RAMBANK(6)
	AM_RANGE(0x3000, 0x37ff) AM_RAMBANK(7)
	AM_RANGE(0x3800, 0x3fff) AM_RAMBANK(8)
	AM_RANGE(0x4000, 0xdfff) AM_RAMBANK(9)
	AM_RANGE(0xe000, 0xffff) AM_RAMBANK(10)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ondra_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x0b)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x03, 0x03) AM_WRITE(ondra_port_03_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(ondra_port_09_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(ondra_port_0a_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ondra )
INPUT_PORTS_END

/* Machine driver */
static MACHINE_DRIVER_START( ondra )
	  /* basic machine hardware */
	  MDRV_CPU_ADD("main", Z80, 2000000)
	  MDRV_CPU_PROGRAM_MAP(ondra_mem, 0)
	  MDRV_CPU_IO_MAP(ondra_io, 0)

	  MDRV_MACHINE_START( ondra )
	  MDRV_MACHINE_RESET( ondra )

    /* video hardware */
		MDRV_SCREEN_ADD("main", RASTER)
		MDRV_SCREEN_REFRESH_RATE(50)
		MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
		MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
		MDRV_SCREEN_SIZE(320, 256)
		MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 256-1)
   	MDRV_PALETTE_LENGTH(2)
		MDRV_PALETTE_INIT(black_and_white)


		MDRV_VIDEO_START(ondra)
    MDRV_VIDEO_UPDATE(ondra)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(ondra)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

/* ROM definition */

ROM_START( ondrat )
    ROM_REGION( 0x11000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "tesla_a.rom", 0x10000, 0x0800, CRC(6d56b815) SHA1(7feb4071d5142e4c2f891747b75fa4d48ccad262) )
	  ROM_LOAD( "tesla_b.rom", 0x10800, 0x0800, CRC(5f145eaa) SHA1(c1eac68b13fedc4d0d6f98b15e2a5397f0139dc3) )
ROM_END

ROM_START( ondrav )
    ROM_REGION( 0x11000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "vili_a.rom", 0x10000, 0x0800, CRC(76932657) SHA1(1f3700f670f158e4bed256aed751e2c1331a28e8) )
    ROM_LOAD( "vili_b.rom", 0x10800, 0x0800, CRC(03a6073f) SHA1(66f198e63f473e09350bcdbb10fe0cf440111bec) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT       INIT     CONFIG COMPANY              FULLNAME   FLAGS */
COMP( 1989, ondrat, 	0, 	 		0,			ondra, 		ondra, 		ondra, 	 ondra,  	"Tesla",		 "Ondra",	 GAME_NOT_WORKING)
COMP( 1989, ondrav, 	ondrat,	0,			ondra, 		ondra, 		ondra, 	 ondra,  	"ViLi",		 	 "Ondra ViLi",	 GAME_NOT_WORKING)

