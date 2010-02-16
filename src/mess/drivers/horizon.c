/***************************************************************************

        NorthStar Horizon

        07/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"

static ADDRESS_MAP_START(horizon_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xe7ff) AM_RAM
	AM_RANGE(0xe800, 0xe8ff) AM_ROM
	AM_RANGE(0xec00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( horizon_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START(horizon_sd_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xe8ff) AM_RAM
	AM_RANGE(0xe900, 0xe9ff) AM_ROM
	AM_RANGE(0xec00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( horizon_sd_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( horizon )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(horizon)
{
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, 0xe800);
}

static MACHINE_RESET(horizon_sd)
{
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, 0xe900);
}

static WRITE8_DEVICE_HANDLER( horizon_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( horizon_terminal_intf )
{
	DEVCB_HANDLER(horizon_kbd_put)
};

static MACHINE_DRIVER_START( horizon )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(horizon_mem)
    MDRV_CPU_IO_MAP(horizon_io)

    MDRV_MACHINE_RESET(horizon)

    /* video hardware */
    MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,horizon_terminal_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( horizsd )
	MDRV_IMPORT_FROM( horizon )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( horizon_sd_mem)
    MDRV_CPU_IO_MAP(horizon_sd_io)

    MDRV_MACHINE_RESET(horizon_sd)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( horizon )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "horizon.bin", 0xe800, 0x0100, CRC(7aafa134) SHA1(bf1552c4818f30473798af4f54e65e1957e0db48))
ROM_END

ROM_START( horizsd )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "horizon-sd.bin", 0xe900, 0x0100, CRC(754e53e5) SHA1(875e42942d639b972252b87d86c3dc2133304967))
ROM_END

ROM_START( vector1 ) // This one have different I/O
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "horizon.bin", 0xe800, 0x0100, CRC(7aafa134) SHA1(bf1552c4818f30473798af4f54e65e1957e0db48))
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 1979, horizon,  0,       0,	horizon,	horizon,	 0,  "NorthStar",   "Horizon (DD drive)",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1979, horizsd,  horizon, 0,	horizsd,	horizon,	 0,  "NorthStar",   "Horizon (SD drive)",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1979, vector1,  horizon, 0,	horizon,	horizon,	 0,  "Vector Graphic",   "Vector 1+ (DD drive)",		GAME_NOT_WORKING | GAME_NO_SOUND)

