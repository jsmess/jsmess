/***************************************************************************

        dolphin

        08/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"


class dolphin_state : public driver_device
{
public:
	dolphin_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(dolphin_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x00ff) AM_ROM
	AM_RANGE( 0x0100, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( dolphin_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dolphin )
INPUT_PORTS_END


static MACHINE_RESET(dolphin)
{
}

static VIDEO_START( dolphin )
{
}

static VIDEO_UPDATE( dolphin )
{
    return 0;
}

static MACHINE_CONFIG_START( dolphin, dolphin_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(dolphin_mem)
    MDRV_CPU_IO_MAP(dolphin_io)

    MDRV_MACHINE_RESET(dolphin)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(dolphin)
    MDRV_VIDEO_UPDATE(dolphin)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dolphin )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "dolphin.rom", 0x0000, 0x0100, CRC(1ac4ac18) SHA1(62a63de6fcd6cd5fcee930d31c73fe603647f06c))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1979, dolphin,  0,       0,	dolphin,	dolphin,	 0,   "<unknown>",   "Dolphin",		GAME_NOT_WORKING | GAME_NO_SOUND )

