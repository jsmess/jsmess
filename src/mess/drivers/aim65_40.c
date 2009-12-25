/***************************************************************************

    Rockwell AIM-65/40

    Skeleton driver

***************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "aim65_40.lh"


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( aim65_40_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( aim65_40 )
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_DRIVER_START( aim65_40 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6502, 1000000)
	MDRV_CPU_PROGRAM_MAP(aim65_40_mem)

	MDRV_DEFAULT_LAYOUT(layout_aim65_40)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( aim65_40 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("mon a v1.0 r32u5-11 8210.z65", 0xa000, 0x1000, CRC(8c970c67) SHA1(5c8aecb2155a10777a57d4f0f2d16b7ba7b1fb45))
	ROM_LOAD("mon b v1.0 r32u6-11 8246.z66", 0xb000, 0x1000, CRC(38a1e0cd) SHA1(37c34e32ad25d27e9590ee3f325801ca311be7b1))
	ROM_LOAD("r2332lp r3224-11 8224.z70",    0xc000, 0x1000, CRC(0878b399) SHA1(483e92b57d64be51643a9f6490521a8572aa2f68)) // Assembler
	ROM_LOAD("i-of v1.0 r32t3-12 8152.z73",  0xf000, 0x1000, CRC(a62bec4a) SHA1(a2fc69a33dc3b7684bf3399beff7b22eaf05c843))
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY     FULLNAME     FLAGS */
COMP( 1981, aim65_40, 0,      0,      aim65_40, aim65_40, 0,    "Rockwell", "AIM-65/40", GAME_NOT_WORKING )
