/***************************************************************************

        Schachcomputer SC1

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class sc1_state : public driver_device
{
public:
	sc1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(sc1_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sc1_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sc1 )
INPUT_PORTS_END


static MACHINE_RESET(sc1)
{
}

static VIDEO_START( sc1 )
{
}

static SCREEN_UPDATE( sc1 )
{
    return 0;
}

static MACHINE_CONFIG_START( sc1, sc1_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(sc1_mem)
    MCFG_CPU_IO_MAP(sc1_io)

    MCFG_MACHINE_RESET(sc1)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(sc1)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(sc1)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sc1 )			/* These are different bios versions */
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sc1.rom", 0x0000, 0x1000, CRC(26965b23) SHA1(01568911446eda9f05ec136df53da147b7c6f2bf))
	ROM_LOAD( "sc1-v2.bin", 0x0000, 0x1000, CRC(1f122a85) SHA1(d60f89f8b59d04f4cecd6e3ecfe0a24152462a36))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 19??, sc1,  0,       0,	sc1,	sc1,	 0, 			 "VEB Mikroelektronik Erfurt",   "Schachcomputer SC1",		GAME_NOT_WORKING | GAME_NO_SOUND)
