/***************************************************************************

        QT Computer Systems SBC +2/4

        11/12/2009 Skeleton driver.

	It expects a rom or similar at E377-up, so currently it crashes.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"

static UINT8 *qtsbc_ram;
static UINT8 term_data;

static WRITE8_HANDLER( qtsbc_06_w )
{
	running_device *terminal = space->machine->device("terminal");

	terminal_write(terminal, 0, data);
}

static READ8_HANDLER( qtsbc_06_r )
{
	UINT8 ret = term_data;
	term_data = 0;
	return ret;
}

static READ8_HANDLER( qtsbc_43_r )
{
	return 0;
}

static ADDRESS_MAP_START(qtsbc_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_RAM AM_BASE(&qtsbc_ram) AM_REGION("maincpu",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qtsbc_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x06, 0x06) AM_READWRITE(qtsbc_06_r,qtsbc_06_w)
	AM_RANGE(0x43, 0x43) AM_READ(qtsbc_43_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( qtsbc )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(qtsbc)
{
	UINT8* bios = memory_region(machine, "maincpu")+0x10000;
	memcpy(qtsbc_ram,bios, 0x800);
}

static WRITE8_DEVICE_HANDLER( qtsbc_kbd_put )
{
	term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( qtsbc_terminal_intf )
{
	DEVCB_HANDLER(qtsbc_kbd_put)
};

static MACHINE_CONFIG_START( qtsbc, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz) // Mostek MK3880
	MDRV_CPU_PROGRAM_MAP(qtsbc_mem)
	MDRV_CPU_IO_MAP(qtsbc_io)

	MDRV_MACHINE_RESET(qtsbc)

	/* video hardware */
	MDRV_FRAGMENT_ADD( generic_terminal )

	MDRV_GENERIC_TERMINAL_ADD("terminal", qtsbc_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( qtsbc )
	ROM_REGION( 0x10800, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "qtsbc.bin", 0x10000, 0x0800, CRC(823fd942) SHA1(64c4f74dd069ae4d43d301f5e279185f32a1efa0))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 19??, qtsbc,  0,       0, 	qtsbc,	qtsbc,	 0, 		 "Computer Systems Inc.",   "QT SBC +2/4",		GAME_NOT_WORKING | GAME_NO_SOUND)
