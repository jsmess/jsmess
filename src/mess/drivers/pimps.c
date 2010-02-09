/***************************************************************************

        P.I.M.P.S. (Personal Interactive MicroProcessor System)

        06/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"

// should be 8251 UART

static UINT8 received_char = 0;

static READ8_HANDLER(pimps_terminal_status_r)
{
	if (received_char!=0) return 3; // char received
	return 1; // ready
}

static READ8_DEVICE_HANDLER(pimps_terminal_r)
{
	UINT8 retVal = received_char;
	received_char = 0;
	return retVal;
}

static ADDRESS_MAP_START(pimps_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pimps_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xf0, 0xf0) AM_DEVREADWRITE("terminal", pimps_terminal_r, terminal_write)
	AM_RANGE(0xf1, 0xf1) AM_READ(pimps_terminal_status_r)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pimps )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(pimps)
{
	received_char = 0;
}

static WRITE8_DEVICE_HANDLER( pimps_kbd_put )
{
	received_char = data;
}

static GENERIC_TERMINAL_INTERFACE( pimps_terminal_intf )
{
	DEVCB_HANDLER(pimps_kbd_put)
};

static MACHINE_DRIVER_START( pimps )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8085A, XTAL_2MHz)
    MDRV_CPU_PROGRAM_MAP(pimps_mem)
    MDRV_CPU_IO_MAP(pimps_io)

    MDRV_MACHINE_RESET(pimps)

    /* video hardware */
    MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,pimps_terminal_intf)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pimps )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "pimps.bin", 0xf000, 0x1000, CRC(5da1898f) SHA1(d20e31d0981a1f54c83186dbdfcf4280e49970d0))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( ????, pimps,  0,       0, 	pimps, 	pimps, 	 0,  		"Henry Colford",   "PIMPS",		GAME_NOT_WORKING | GAME_NO_SOUND)

