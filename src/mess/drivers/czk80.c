/***************************************************************************

        CZK-80

        30/08/2010 Skeleton driver
		
		On main board there are Z80A CPU, Z80A PIO, Z80A DART and Z80A CTC
			there is 8K ROM and XTAL 16MHz
		FDC board contains Z80A DMA and NEC 765A (XTAL on it is 8MHZ)
		Mega board contains 74LS612 and memory chips

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static UINT8 *czk80_ram;

static ADDRESS_MAP_START(czk80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&czk80_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( czk80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( czk80 )
INPUT_PORTS_END

static MACHINE_RESET(czk80)
{
	UINT8* bios = memory_region(machine, "maincpu") + 0xe000;

	memcpy(czk80_ram,bios, 0x2000);
	memcpy(czk80_ram+0xe000,bios, 0x2000);
}

static VIDEO_START( czk80 )
{
}

static VIDEO_UPDATE( czk80 )
{
	return 0;
}

static MACHINE_CONFIG_START( czk80, driver_data_t )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MDRV_CPU_PROGRAM_MAP(czk80_mem)
	MDRV_CPU_IO_MAP(czk80_io)

    MDRV_MACHINE_RESET(czk80)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192) /* border size not accurate */
	MDRV_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)

    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(czk80)
    MDRV_VIDEO_UPDATE(czk80)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( czk80 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "czk80.rom", 0xe000, 0x2000, CRC(7081b7c6) SHA1(13f75b14ea73b252bdfa2384e6eead6e720e49e3))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT  		COMPANY   FULLNAME       FLAGS */
COMP( 198?, czk80,  0,       0, 	czk80, 	czk80, 	 0,  	  "<unknown>",   "CZK-80",		GAME_NOT_WORKING | GAME_NO_SOUND)
