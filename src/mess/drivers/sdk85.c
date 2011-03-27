/***************************************************************************

        Intel SDK-85

        09/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"


class sdk85_state : public driver_device
{
public:
	sdk85_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(sdk85_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM // Monitor rom (A14)
	AM_RANGE(0x0800, 0x0fff) AM_ROM // Expansion rom (A15)
	AM_RANGE(0x1800, 0x1fff) AM_RAM // i8279 (A13)
	AM_RANGE(0x2000, 0x27ff) AM_RAM // i8155 (A16)
	AM_RANGE(0x2800, 0x2fff) AM_RAM // i8155 (A17)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sdk85_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sdk85 )
INPUT_PORTS_END


static MACHINE_RESET(sdk85)
{
}

static VIDEO_START( sdk85 )
{
}

static SCREEN_UPDATE( sdk85 )
{
    return 0;
}

static MACHINE_CONFIG_START( sdk85, sdk85_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8085A, XTAL_2MHz)
    MCFG_CPU_PROGRAM_MAP(sdk85_mem)
    MCFG_CPU_IO_MAP(sdk85_io)

    MCFG_MACHINE_RESET(sdk85)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(sdk85)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(sdk85)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sdk85 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sdk85.a14", 0x0000, 0x0800, CRC(9d5a983f) SHA1(54e218560fbec009ac3de5cfb64b920241ef2eeb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 1977, sdk85,  0,       0, 		sdk85,	sdk85,	 0,		 "Intel",   "SDK-85",		GAME_NOT_WORKING | GAME_NO_SOUND)

