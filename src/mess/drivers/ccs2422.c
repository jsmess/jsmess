/***************************************************************************

        CCS Model 2422B

        11/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"


class ccs2422_state : public driver_device
{
public:
	ccs2422_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *ccs_ram;
};



static WRITE8_HANDLER(ccs2422_terminal_w)
{
	running_device *devconf = space->machine->device("terminal");
	terminal_write(devconf,0,data);
}

static ADDRESS_MAP_START(ccs2422_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE_MEMBER(ccs2422_state, ccs_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ccs2422_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x10) AM_WRITE(ccs2422_terminal_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ccs2422 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(ccs2422)
{
	ccs2422_state *state = machine->driver_data<ccs2422_state>();
	UINT8* user1 = memory_region(machine, "user1");
	memcpy((UINT8*)state->ccs_ram,user1,0x0800);

	// this should be rom/ram banking
}

static WRITE8_DEVICE_HANDLER( ccs2422_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( ccs2422_terminal_intf )
{
	DEVCB_HANDLER(ccs2422_kbd_put)
};

static MACHINE_CONFIG_START( ccs2422, ccs2422_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(ccs2422_mem)
    MDRV_CPU_IO_MAP(ccs2422_io)

    MDRV_MACHINE_RESET(ccs2422)

    /* video hardware */
    MDRV_FRAGMENT_ADD( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,ccs2422_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( ccs2422 )
    ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "2422b.u24", 0x0000, 0x0800, CRC(6cf22e31) SHA1(9aa3327cd8c23d0eab82cb6519891aff13ebe1d0))
	ROM_LOAD( "2422.u23",  0x0900, 0x0100, CRC(b279cada) SHA1(6cc6e00ec49ba2245c8836d6f09266b09d6e7648))
	ROM_LOAD( "2422.u22",  0x0a00, 0x0100, CRC(e41858bb) SHA1(0be53725032ebea16e32cb720f099551a357e761))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 19??, ccs2422,  0,       0,	ccs2422,	ccs2422,	 0,   "California Computer Systems",   "CCS Model 2422B",		GAME_NOT_WORKING | GAME_NO_SOUND)

