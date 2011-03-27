/***************************************************************************

        Elektronika MK-85

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/t11/t11.h"


class mk85_state : public driver_device
{
public:
	mk85_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(mk85_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0xffff ) AM_RAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mk85 )
INPUT_PORTS_END


static MACHINE_RESET(mk85)
{
}

static VIDEO_START( mk85 )
{
}

static SCREEN_UPDATE( mk85 )
{
    return 0;
}

static const struct t11_setup t11_data =
{
	5 << 13			/* start from 0000 */
};

static MACHINE_CONFIG_START( mk85, mk85_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",T11, XTAL_4MHz)
    MCFG_CPU_CONFIG(t11_data)
    MCFG_CPU_PROGRAM_MAP(mk85_mem)

    MCFG_MACHINE_RESET(mk85)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(mk85)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(mk85)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mk85 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mk85.rom", 0x0000, 0x4000, CRC(398e4fd1) SHA1(5e2f877d0f451b46840f01190004552bad5248c8))
	ROM_COPY("maincpu", 0x0000,0x4000,0x4000)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1986, mk85,  0,       0,			mk85,	mk85,	 0, 	 "Elektronika",   "MK-85",		GAME_NOT_WORKING | GAME_NO_SOUND)

