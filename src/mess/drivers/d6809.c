/***************************************************************************

        6809 Protable

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"


class d6809_state : public driver_device
{
public:
	d6809_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(d6809_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0xdfff) AM_RAM
	AM_RANGE(0xe000,0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( d6809_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( d6809 )
INPUT_PORTS_END


static MACHINE_RESET(d6809)
{
}

static VIDEO_START( d6809 )
{
}

static SCREEN_UPDATE( d6809 )
{
    return 0;
}

static MACHINE_CONFIG_START( d6809, d6809_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M6809E, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(d6809_mem)
    MCFG_CPU_IO_MAP(d6809_io)

    MCFG_MACHINE_RESET(d6809)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(d6809)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(d6809)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( d6809 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "d6809.rom", 0xe000, 0x2000, CRC(2ceb40b8) SHA1(780111541234b4f0f781a118d955df61daa56e7e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 198?, d6809,  0,       0, 	d6809,	d6809,	 0, 		 "Dunfield",   "6809 Portable",		GAME_NOT_WORKING | GAME_NO_SOUND)

