/***************************************************************************

        Digital Microsystems DMS-86

        11/01/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"


class dms86_state : public driver_device
{
public:
	dms86_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(dms86_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xfe000, 0xfffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dms86_io , AS_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dms86 )
INPUT_PORTS_END


static MACHINE_RESET(dms86)
{
}

static VIDEO_START( dms86 )
{
}

static SCREEN_UPDATE( dms86 )
{
    return 0;
}

static MACHINE_CONFIG_START( dms86, dms86_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8086, XTAL_9_8304MHz)
    MCFG_CPU_PROGRAM_MAP(dms86_mem)
    MCFG_CPU_IO_MAP(dms86_io)

    MCFG_MACHINE_RESET(dms86)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(dms86)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(dms86)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dms86 )
	ROM_REGION( 0x2000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "hns-86_54-8678.bin", 0x0000, 0x1000, CRC(95f58e1c) SHA1(6fc8f087f0c887d8b429612cd035c6c1faab570c))
	ROM_LOAD16_BYTE( "hns-86_54-8677.bin", 0x0001, 0x1000, CRC(78fad756) SHA1(ddcbff1569ec6975b8489935cdcfa80eba413502))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1982, dms86,  0,       0, 	dms86,	dms86,	 0, 	 "Digital Microsystems",   "DMS-86",		GAME_NOT_WORKING | GAME_NO_SOUND)

