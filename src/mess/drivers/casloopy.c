/*****************************************************************************

    Casio Loopy (c) 1995 Casio

    skeleton driver

    Note:
    - just a placeholder for any HW discovery, until we decap/trojan the BIOS,
      the idea is to understand the HW enough to extract the SH-1 internal BIOS
      data via a trojan;

******************************************************************************/

#include "emu.h"
#include "cpu/sh2/sh2.h"
#include "devices/cartslot.h"

static VIDEO_START( casloopy )
{

}

static VIDEO_UPDATE( casloopy )
{
	return 0;
}

static ADDRESS_MAP_START( casloopy_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM AM_REGION("cart",0) // wrong
ADDRESS_MAP_END


static INPUT_PORTS_START( casloopy )
INPUT_PORTS_END

static MACHINE_RESET( casloopy )
{
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, ASSERT_LINE); //halt the CPU until we find enough data to proceed
}

static MACHINE_CONFIG_START( casloopy, driver_data_t )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",SH1,8000000)
	MDRV_CPU_PROGRAM_MAP(casloopy_map)

	MDRV_MACHINE_RESET(casloopy)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(casloopy)
	MDRV_VIDEO_UPDATE(casloopy)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_MANDATORY

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( casloopy )
	ROM_REGION( 0x80000, "maincpu", 0)
	ROM_LOAD( "bios", 0x0000, 0x4000, NO_DUMP )

	ROM_REGION( 0x200000, "cart", 0 )
	ROM_CART_LOAD("cart",    0x00000, 0x200000, ROM_NOMIRROR)
ROM_END

GAME( 1995, casloopy,  0,   casloopy,  casloopy,  0, ROT0, "Casio", "Loopy", GAME_NOT_WORKING | GAME_NO_SOUND )
