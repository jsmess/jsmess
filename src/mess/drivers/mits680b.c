/***************************************************************************

        MITS Altair 680b

        03/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "machine/6551.h"
#include "machine/terminal.h"

static ADDRESS_MAP_START(mits680b_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff ) AM_RAM // 1024 bytes RAM
	AM_RANGE( 0xf000, 0xf003 ) AM_DEVREADWRITE("acia",  acia_6551_r, acia_6551_w )
	AM_RANGE( 0xff00, 0xffff ) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mits680b )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(mits680b)
{
}

static WRITE8_DEVICE_HANDLER( mits680b_kbd_put )
{

}

static GENERIC_TERMINAL_INTERFACE( mits680b_terminal_intf )
{
	DEVCB_HANDLER(mits680b_kbd_put)
};

static MACHINE_CONFIG_START( mits680b, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6800, XTAL_1MHz / 2)
    MDRV_CPU_PROGRAM_MAP(mits680b_mem)

    MDRV_MACHINE_RESET(mits680b)

    /* video hardware */
    MDRV_FRAGMENT_ADD( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,mits680b_terminal_intf)

	/* acia */
	MDRV_ACIA6551_ADD("acia")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mits680b )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mits680b.bin", 0xff00, 0x0100, CRC(397e717f) SHA1(257d3eb1343b8611dc05455aeed33615d581f29c))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1976, mits680b,  0,     0,	mits680b,	mits680b,	 0,   "MITS",   "Altair 680b",		GAME_NOT_WORKING | GAME_NO_SOUND)

