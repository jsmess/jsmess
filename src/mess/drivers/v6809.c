/***************************************************************************

        Vegas 6809

        Skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"


class v6809_state : public driver_device
{
public:
	v6809_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};


static ADDRESS_MAP_START(v6809_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xf800,0xffff) AM_ROM AM_REGION("maincpu", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( v6809_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( v6809 )
INPUT_PORTS_END


static MACHINE_RESET(v6809)
{
}

static VIDEO_START( v6809 )
{
}

static SCREEN_UPDATE( v6809 )
{
    return 0;
}

static MACHINE_CONFIG_START( v6809, v6809_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M6809, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(v6809_mem)
    MCFG_CPU_IO_MAP(v6809_io)

    MCFG_MACHINE_RESET(v6809)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(v6809)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(v6809)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( v6809 )
	ROM_REGION( 0x800, "maincpu", 0 )
	ROM_LOAD( "v6809.rom", 0x000, 0x800, CRC(54bf5f32) SHA1(10d1d70f0b51e2b90e5c29249d3eab4c6b0033a1))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1982, v6809,  0,       0, 	v6809,	v6809,	 0, 		 "Microkit",   "Vegas 6809",		GAME_NOT_WORKING | GAME_NO_SOUND)

