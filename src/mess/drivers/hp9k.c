/***************************************************************************

    Skeleton driver for HP9816 (HP Series 200 series, Technical Desktops)
 
    see for docs: http://www.hpmuseum.net/collection_document.php
 
	TODO: everything!

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"


class hp9k_state : public driver_device
{
public:
	hp9k_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(hp9k_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xfe0000, 0xffffff) AM_ROM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( hp9k )
INPUT_PORTS_END


static MACHINE_RESET(hp9k)
{
}

static VIDEO_START( hp9k )
{
}

static SCREEN_UPDATE( hp9k )
{
    return 0;
}

static MACHINE_CONFIG_START( hp9k, hp9k_state )
    /* basic machine hardware */
	MCFG_CPU_ADD("maincpu",M68000, XTAL_8MHz)
	MCFG_CPU_PROGRAM_MAP(hp9k_mem)

    MCFG_MACHINE_RESET(hp9k)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(hp9k)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(hp9k)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( hp9816 )
	ROM_REGION16_BE(0x1000000, "maincpu", 0)
	ROM_DEFAULT_BIOS("bios40")
	ROM_SYSTEM_BIOS(0, "bios40",  "Bios v4.0")
	ROMX_LOAD( "rom40.bin", 0xfe0000, 0x10000, CRC(36005480) SHA1(645a077ffd95e4c31f05cd8bbd6e4554b12813f1), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "bios30",  "Bios v3.0")
	ROMX_LOAD( "rom30.bin", 0xfe0000, 0x10000, CRC(05c07e75) SHA1(3066a65e6137482041f9a77d09ee2289fe0974aa), ROM_BIOS(2) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, hp9816,	0,       0, 		hp9k,	hp9k,	 0, 	  "Hewlett Packard",   "HP 9816",		GAME_NOT_WORKING | GAME_NO_SOUND)
