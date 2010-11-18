/***************************************************************************

        mc-CP/M-Computer

        31/08/2010 Skeleton driver.
        18/11/2010 Connected to a terminal

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"

static UINT8 *mccpm_ram;
static UINT8 term_data;

static WRITE8_HANDLER( mccpm_f0_w )
{
	running_device *terminal = space->machine->device("terminal");

	terminal_write(terminal, 0, data);
}

static READ8_HANDLER( mccpm_f0_r )
{
	UINT8 ret = term_data;
	term_data = 0;
	return ret;
}

static ADDRESS_MAP_START(mccpm_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&mccpm_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mccpm_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf0) AM_READWRITE(mccpm_f0_r,mccpm_f0_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mccpm )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(mccpm)
{
	UINT8* bios = memory_region(machine, "maincpu");

	memcpy(mccpm_ram,bios, 0x1000);
}

static WRITE8_DEVICE_HANDLER( mccpm_kbd_put )
{
	term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( mccpm_terminal_intf )
{
	DEVCB_HANDLER(mccpm_kbd_put)
};

static MACHINE_CONFIG_START( mccpm, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(mccpm_mem)
	MDRV_CPU_IO_MAP(mccpm_io)

	MDRV_MACHINE_RESET(mccpm)

	/* video hardware */
	MDRV_FRAGMENT_ADD( generic_terminal )

	MDRV_GENERIC_TERMINAL_ADD("terminal", mccpm_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mccpm )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon36.j15", 0x0000, 0x1000, CRC(9c441537) SHA1(f95bad52d9392b8fc9d9b8779b7b861672a0022b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, mccpm,  0,       0, 		mccpm,	mccpm,	 0, 	 "GRAF Elektronik Systeme GmbH",   "mc-CP/M-Computer",		GAME_NOT_WORKING | GAME_NO_SOUND)

