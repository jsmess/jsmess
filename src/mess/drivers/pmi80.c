/***************************************************************************

        Tesla PMI-80

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"


class pmi80_state : public driver_device
{
public:
	pmi80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(pmi80_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff ) AM_ROM
	AM_RANGE( 0x0400, 0x1fff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pmi80_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pmi80 )
INPUT_PORTS_END


static MACHINE_RESET(pmi80)
{
}

static VIDEO_START( pmi80 )
{
}

static SCREEN_UPDATE( pmi80 )
{
    return 0;
}

static MACHINE_CONFIG_START( pmi80, pmi80_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8080, XTAL_1MHz)
    MCFG_CPU_PROGRAM_MAP(pmi80_mem)
    MCFG_CPU_IO_MAP(pmi80_io)

    MCFG_MACHINE_RESET(pmi80)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(pmi80)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(pmi80)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pmi80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "pmi80_monitor.rom", 0x0000, 0x0400, CRC(b93f4407) SHA1(43153441070ed0572f33d2815635eb7bae878e38))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, pmi80,  0,       0, 	pmi80,	pmi80,	 0, 		 "Tesla",   "PMI-80",		GAME_NOT_WORKING | GAME_NO_SOUND)

