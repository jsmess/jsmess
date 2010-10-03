/*

	Atari Portfolio


*/

#include "emu.h"
#include "cpu/i86/i86.h"

class portfolio_state : public driver_device
{
public:
	portfolio_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};

static ADDRESS_MAP_START(portfolio_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x3efff) AM_RAM
	AM_RANGE(0x3f000, 0x9efff) AM_RAM // expansion
	AM_RANGE(0xb0000, 0xb0fff) AM_MIRROR(0xf000) AM_RAM // video RAM
	AM_RANGE(0xc0000, 0xdffff) AM_ROM AM_REGION("maincpu", 0) // or credit card memory
	AM_RANGE(0xe0000, 0xfffff) AM_ROM AM_REGION("maincpu", 0x20000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(portfolio_io, ADDRESS_SPACE_IO, 8)
ADDRESS_MAP_END

static INPUT_PORTS_START( portfolio )
INPUT_PORTS_END

static PALETTE_INIT( portfolio )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_UPDATE( portfolio )
{
	return 0;
}

static MACHINE_START(portfolio)
{
}

static MACHINE_CONFIG_START( portfolio, portfolio_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", I8088, 4915200)
    MDRV_CPU_PROGRAM_MAP(portfolio_mem)
    MDRV_CPU_IO_MAP(portfolio_io)
	
	MDRV_MACHINE_START(portfolio)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(72)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 64-1)

	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(portfolio)

	MDRV_VIDEO_UPDATE(portfolio)
MACHINE_CONFIG_END

ROM_START( portfoli )
    ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "rom b", 0x00000, 0x20000, BAD_DUMP CRC(c9852766) SHA1(c74430281bc717bd36fd9b5baec1cc0f4489fe82) ) // dumped with debug.com
	ROM_LOAD( "rom a", 0x20000, 0x20000, BAD_DUMP CRC(b8fb730d) SHA1(1b9d82b824cab830256d34912a643a7d048cd401) ) // dumped with debug.com
ROM_END

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                FLAGS */
COMP( 1989, portfoli,       0,          0,      portfolio,  portfolio, 0,      "Atari",   "Portfolio",						GAME_NOT_WORKING | GAME_NO_SOUND )
