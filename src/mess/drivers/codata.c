/***************************************************************************

        Contel Codata Corporation Codata

        11/01/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"


class codata_state : public driver_device
{
public:
	codata_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};



static ADDRESS_MAP_START(codata_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( codata )
INPUT_PORTS_END


static MACHINE_RESET(codata)
{
}

static VIDEO_START( codata )
{
}

static SCREEN_UPDATE( codata )
{
    return 0;
}

static MACHINE_CONFIG_START( codata, codata_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M68000, XTAL_16MHz / 2)
    MCFG_CPU_PROGRAM_MAP(codata_mem)

    MCFG_MACHINE_RESET(codata)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(codata)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(codata)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( codata )
    ROM_REGION( 0x8000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "27-0042-01a boot 00 u101 rev 3.6.2 9-28-83.u101", 0x0000, 0x2000, CRC(70014b16) SHA1(19a82000894d79817358d40ae520200e976be310))
	ROM_LOAD16_BYTE( "27-0043-01a boot 01 u102 rev 3.6.2 9-28-83.u102", 0x4000, 0x2000, CRC(fca9c314) SHA1(2f8970fad479000f28536003867066d6df9e33d9))
	ROM_LOAD16_BYTE( "27-0044-01a boot e0 u103 rev 3.6.2 9-28-83.u103", 0x0001, 0x2000, CRC(dc5d5cea) SHA1(b3e9248abf89d674c463d21d2f7be34508cf16c2))
	ROM_LOAD16_BYTE( "27-0045-01a boot e1 u104 rev 3.6.2 9-28-83.u104", 0x4001, 0x2000, CRC(a937e7b3) SHA1(d809bbd437fe7d925325958072b9e0dc33dd36a6))

	ROM_REGION( 0x240, "proms", 0 )
	ROM_LOAD( "p0.u502", 0x0000, 0x0020, CRC(20eb1183) SHA1(9b268792b28d858d6b6a1b6c4148af88a8d6b735) )
	ROM_LOAD( "p1.u602", 0x0020, 0x0020, CRC(ee1e5a14) SHA1(0d3346cb3b647fa2475bd7b4fa36ea6ecfdaf805) )
	ROM_LOAD( "p2.u503", 0x0040, 0x0200, CRC(12d9a6be) SHA1(fca99f9c5afc630ac67cbd4e5ba4e5242b826848) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1982, codata,  0,       0,	codata, 	codata, 	 0,   "Contel Codata Corporation",   "Codata",		GAME_NOT_WORKING | GAME_NO_SOUND)
