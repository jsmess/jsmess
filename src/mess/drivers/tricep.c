/***************************************************************************

        Morrow Tricep

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/terminal.h"

static UINT16* tricep_ram;

static READ16_HANDLER(tricep_terminal_r)
{
	return 0xffff;
}

static WRITE16_HANDLER(tricep_terminal_w)
{
	running_device *devconf = space->machine->device("terminal");
	terminal_write(devconf,0,data >> 8);
}

static ADDRESS_MAP_START(tricep_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE(&tricep_ram)
	AM_RANGE(0x00fd0000, 0x00fd1fff) AM_ROM AM_REGION("user1",0)
	AM_RANGE(0x00ff0028, 0x00ff0029) AM_READWRITE(tricep_terminal_r,tricep_terminal_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( tricep )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(tricep)
{
	UINT8* user1 = memory_region(machine, "user1");

	memcpy((UINT8*)tricep_ram,user1,0x2000);

	machine->device("maincpu")->reset();
}

static WRITE8_DEVICE_HANDLER( tricep_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( tricep_terminal_intf )
{
	DEVCB_HANDLER(tricep_kbd_put)
};

static MACHINE_CONFIG_START( tricep, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(tricep_mem)

    MDRV_MACHINE_RESET(tricep)

    /* video hardware */
    MDRV_FRAGMENT_ADD( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,tricep_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( tricep )
    ROM_REGION( 0x2000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "tri2.4_odd.u37",  0x0000, 0x1000, CRC(31eb2dcf) SHA1(2d9df9262ee1096d0398505e10d209201ac49a5d))
	ROM_LOAD16_BYTE( "tri2.4_even.u36", 0x0001, 0x1000, CRC(4414dcdc) SHA1(00a3d293617dc691748ae85b6ccdd6723daefc0a))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY             FULLNAME       FLAGS */
COMP( 1985, tricep,  0,       0,	tricep, 	tricep, 	 0,  "Morrow Designs",   "Tricep",		GAME_NOT_WORKING | GAME_NO_SOUND)

