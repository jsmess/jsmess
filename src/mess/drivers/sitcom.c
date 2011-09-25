/***************************************************************************

    SITCOM

    25/09/2011 Skeleton driver.

    http://www.izabella.me.uk/html/sitcom_.html

    It has a few LED digits to be connected up.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"

class sitcom_state : public driver_device
{
public:
	sitcom_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
};

static ADDRESS_MAP_START(sitcom_mem, AS_PROGRAM, 8, sitcom_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(sitcom_io, AS_IO, 8, sitcom_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	// AM_RANGE(0x00, 0x1f) 8255
	// AM_RANGE(0xc0, 0xdf) left display
	// AM_RANGE(0xe0, 0xff) right display
	AM_RANGE(0xc0, 0xff) AM_DEVWRITE_LEGACY(TERMINAL_TAG, terminal_write) //temp
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sitcom )
INPUT_PORTS_END

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_NULL
};

static MACHINE_CONFIG_START( sitcom, sitcom_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8080, XTAL_4MHz) // no idea of clock.
	MCFG_CPU_PROGRAM_MAP(sitcom_mem)
	MCFG_CPU_IO_MAP(sitcom_io)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sitcom )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "boot8085.bin", 0x0000, 0x06b8, CRC(1b5e3310) SHA1(3323b65f0c10b7ab6bb75ec824e6d5fb643693a8))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT               COMPANY              FULLNAME       FLAGS */
COMP( 2002, sitcom, 0,      0,       sitcom,    sitcom,  0,   "San Bergmans & Izabella Malcolm", "Sitcom", GAME_NOT_WORKING | GAME_NO_SOUND_HW)
