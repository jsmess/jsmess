/***************************************************************************

        Indiana University 68030 board

        08/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"


class indiana_state : public driver_device
{
public:
	indiana_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(indiana_mem, AS_PROGRAM, 32)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0000ffff) AM_MIRROR(0x7f800000) AM_ROM AM_REGION("user1",0) // 64Kb of EPROM
	AM_RANGE(0x00100000, 0x00107fff) AM_MIRROR(0x7f8f8000) AM_RAM // SRAM 32Kb of SRAM
	AM_RANGE(0x00200000, 0x002fffff) AM_MIRROR(0x7f800000) AM_RAM // MFP
	AM_RANGE(0x00400000, 0x004fffff) AM_MIRROR(0x7f800000) AM_RAM // 16 bit PC IO
	AM_RANGE(0x00500000, 0x005fffff) AM_MIRROR(0x7f800000) AM_RAM // 16 bit PC MEM
	AM_RANGE(0x00600000, 0x006fffff) AM_MIRROR(0x7f800000) AM_RAM // 8 bit PC IO
	AM_RANGE(0x00700000, 0x007fffff) AM_MIRROR(0x7f800000) AM_RAM // 8 bit PC MEM // 7f7b8000 location of VGA RAM
	AM_RANGE(0x80000000, 0x803fffff) AM_MIRROR(0x7fc00000) AM_RAM // 4 MB RAM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( indiana )
INPUT_PORTS_END


static MACHINE_RESET(indiana)
{
}

static VIDEO_START( indiana )
{
}

static SCREEN_UPDATE( indiana )
{
    return 0;
}

static MACHINE_CONFIG_START( indiana, indiana_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M68030, XTAL_16MHz)
    MCFG_CPU_PROGRAM_MAP(indiana_mem)

    MCFG_MACHINE_RESET(indiana)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(indiana)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(indiana)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( indiana )
	ROM_REGION32_BE( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v9", "ver 0.9" )
	ROMX_LOAD( "prom0_9.bin", 0x0000, 0x10000, CRC(746ad75e) SHA1(7d5c123c8568b1e02ab683e8f3188d0fef78d740), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v8", "ver 0.8" )
	ROMX_LOAD( "prom0_8.bin", 0x0000, 0x10000, CRC(9d8dafee) SHA1(c824e5fe6eec08f51ef287c651a5034fe3c8b718), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v7", "ver 0.7" )
	ROMX_LOAD( "prom0_7.bin", 0x0000, 0x10000, CRC(d6a3b6bc) SHA1(01d8cee989ab29646d9d3f8b7262b10055653d41), ROM_BIOS(3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                  FULLNAME                               FLAGS */
COMP( 1993, indiana,  0,       0,	indiana,	indiana,	 0,  "Indiana University",   "Indiana University 68030 board",		GAME_NOT_WORKING | GAME_NO_SOUND)

