/***************************************************************************

        MITS Altair 8800b Turnkey

        04/12/2009 Initial driver by Miodrag Milanovic

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"

static UINT8 term_data;

static READ8_HANDLER(sio_status_r)
{
	if (term_data!=0) return 0x01; // data in
	return 0x02; // ready
}

static WRITE8_HANDLER(sio_command_w)
{

}

static READ8_HANDLER(sio_data_r)
{
	UINT8 retVal = term_data;
	term_data = 0;
	return retVal;
}

static WRITE8_HANDLER(sio_data_w)
{
	const device_config	*devconf = devtag_get_device(space->machine, "terminal");
	terminal_write(devconf,0,data);
}

static ADDRESS_MAP_START(altair_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xfcff ) AM_RAM
	AM_RANGE( 0xfd00, 0xfdff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( altair_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x10, 0x10 ) AM_READWRITE(sio_status_r,sio_command_w)
	AM_RANGE( 0x11, 0x11 ) AM_READWRITE(sio_data_r,sio_data_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( altair )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(altair)
{
	// Set startup addess done by turn-key
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), I8085_PC, 0xFD00);
}

static WRITE8_DEVICE_HANDLER( altair_kbd_put )
{
	term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( altair_terminal_intf )
{
	DEVCB_HANDLER(altair_kbd_put)
};

static MACHINE_DRIVER_START( altair )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", 8080, XTAL_2MHz)
    MDRV_CPU_PROGRAM_MAP(altair_mem)
    MDRV_CPU_IO_MAP(altair_io)

    MDRV_MACHINE_RESET(altair)

	/* video hardware */
	MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,altair_terminal_intf)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( al8800bt )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "turnmon.bin", 0xfd00, 0x0100, CRC(5c629294) SHA1(125c76216954b681721fff84a3aca05094b21a28))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG     COMPANY   FULLNAME       FLAGS */
COMP( 1975, al8800bt,  0,       0, 	altair, 	altair, 	 0,  	  0,    "MITS",   "Altair 8800bt",		GAME_NOT_WORKING)

