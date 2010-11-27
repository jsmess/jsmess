/***************************************************************************

        Digital Microsystems ZSBC-3

        11/01/2010 Skeleton driver.
        28/11/2010 Connected to a terminal

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"

static UINT8 term_data;

static WRITE8_HANDLER( zsbc3_28_w )
{
	running_device *terminal = space->machine->device("terminal");

	terminal_write(terminal, 0, data);
}

static READ8_HANDLER( zsbc3_28_r )
{
	UINT8 ret = term_data;
	term_data = 0;
	return ret;
}

static READ8_HANDLER( zsbc3_2a_r )
{
	return 4 | ((term_data) ? 1 : 0);
}

static ADDRESS_MAP_START(zsbc3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	AM_RANGE( 0x0800, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( zsbc3_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x28, 0x28) AM_READWRITE(zsbc3_28_r,zsbc3_28_w)
	AM_RANGE(0x2a, 0x2a) AM_READ(zsbc3_2a_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( zsbc3 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(zsbc3)
{
}

static WRITE8_DEVICE_HANDLER( zsbc3_kbd_put )
{
	term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( zsbc3_terminal_intf )
{
	DEVCB_HANDLER(zsbc3_kbd_put)
};


static MACHINE_CONFIG_START( zsbc3, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_16MHz /4)
	MDRV_CPU_PROGRAM_MAP(zsbc3_mem)
	MDRV_CPU_IO_MAP(zsbc3_io)

	MDRV_MACHINE_RESET(zsbc3)

	/* video hardware */
	MDRV_FRAGMENT_ADD( generic_terminal )

	MDRV_GENERIC_TERMINAL_ADD("terminal", zsbc3_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( zsbc3 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "54-3002_zsbc_monitor_1.09.bin", 0x0000, 0x0800, CRC(628588e9) SHA1(8f0d489147ec8382ca007236e0a95a83b6ebcd86))
	ROM_REGION( 0x10000, "hdc", ROMREGION_ERASEFF )
	ROM_LOAD( "54-8622_hdc13.bin", 0x0000, 0x0400, CRC(02c7cd6d) SHA1(494281ba081a0f7fbadfc30a7d2ea18c59e55101))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY               FULLNAME       FLAGS */
COMP( 1980, zsbc3,  0,       0, 		zsbc3,	zsbc3,	 0, 	  "Digital Microsystems",   "ZSBC-3",		GAME_NOT_WORKING | GAME_NO_SOUND)

