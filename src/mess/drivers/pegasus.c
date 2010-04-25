/***************************************************************************
   
        Aamber Pegasus computer (New Zealand)

	http://web.mac.com/lord_philip/aamber_pegasus/Aamber_Pegasus.html

        23/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "includes/pegasus.h"


static ADDRESS_MAP_START(pegasus_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xbdff) AM_RAM
	AM_RANGE(0xbe00, 0xbfff) AM_RAM AM_BASE(&pegasus_video_ram) // Video RAM
/*	AM-RANGE(0xe200, 0xe3ff) PCG
	AM-RANGE(0xe400, 0xe5ff) PIA-1
	AM-RANGE(0xe600, 0xe7ff) PIA-0
	AM-RANGE(0xe800, 0xe807) Printer Port
	AM-RANGE(0xe808, 0xe80f) Cassette Interface
	AM-RANGE(0xe810, 0xe817) General Purpose User Interface */
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pegasus )
INPUT_PORTS_END


static MACHINE_RESET(pegasus) 
{	
}

static MACHINE_DRIVER_START( pegasus )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809E, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(pegasus_mem)

	MDRV_MACHINE_RESET(pegasus)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*10, 16*16)
	MDRV_SCREEN_VISIBLE_AREA(0, 32*10-1, 0, 16*15-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(pegasus)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pegasus )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon1_1_2674.bin", 0xf000, 0x1000, CRC(1640ff7e) SHA1(8199643749fb40fb8be05e9f311c75620ca939b1) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, pegasus,  0,     0, 	pegasus, 	pegasus, 	 0,  "Technosys",   "Aamber Pegasus", GAME_NOT_WORKING | GAME_NO_SOUND)

