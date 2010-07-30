/***************************************************************************
   
        Acorn Archimedes 7000/7000+

        30/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/arm7/arm7.h"


static ADDRESS_MAP_START( a7000_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x003FFFFF) AM_ROM AM_REGION("user1", 0x0)
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( a7000 )
INPUT_PORTS_END


static MACHINE_RESET(a7000) 
{	
}

static VIDEO_START( a7000 )
{
}

static VIDEO_UPDATE( a7000 )
{
    return 0;
}

static MACHINE_DRIVER_START( a7000 )

	/* Basic machine hardware */
	MDRV_CPU_ADD( "maincpu", ARM7, XTAL_32MHz )
	MDRV_CPU_PROGRAM_MAP( a7000_mem)

	MDRV_MACHINE_RESET( a7000 )

	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(a7000)
    MDRV_VIDEO_UPDATE(a7000)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a7000p )
	MDRV_IMPORT_FROM( a7000 )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_CLOCK(XTAL_48MHz)
MACHINE_DRIVER_END

ROM_START(a7000)
	ROM_REGION32_LE( 0x400000, "user1", 0 )
	ROM_LOAD( "rom1.bin", 0x000000, 0x100000, CRC(ff0e3d12) SHA1(fa489bebede3d13dc43cddec5b5c9b6829a28914))
	ROM_LOAD( "rom2.bin", 0x100000, 0x100000, CRC(4ae4fd8b) SHA1(1b30d5905d5364dfa48bad69257b0ef8190e9bf6))
	ROM_LOAD( "rom3.bin", 0x200000, 0x100000, CRC(3108fb2b) SHA1(865b01583f3fb5f4ed5e9201676db327cdeb40b3))
	ROM_LOAD( "rom4.bin", 0x300000, 0x100000, CRC(55a51980) SHA1(a7191727edd5babf679ebbdea6585833a1fb34e6))
ROM_END

ROM_START(a7000p)
	ROM_REGION32_LE( 0x400000, "user1", 0 )
	ROM_LOAD( "riscos-3.71.rom", 0x000000, 0x400000, CRC(211cf888) SHA1(c5fe0645e48894fb4b245abeefdc9a65d659af59))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY FULLNAME        FLAGS */
CONS( 1995, a7000,      0,      0,      a7000,      a7000,	0,      "Acorn",  "Archimedes 7000",   GAME_NOT_WORKING | GAME_NO_SOUND )
CONS( 1997, a7000p,     a7000,  0,      a7000,      a7000,	0,      "Acorn",  "Archimedes 7000+",  GAME_NOT_WORKING | GAME_NO_SOUND )
