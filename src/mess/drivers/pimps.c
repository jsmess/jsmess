/***************************************************************************

        P.I.M.P.S. (Personal Interactive MicroProcessor System)

        06/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"


class pimps_state : public driver_device
{
public:
	pimps_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 received_char;
};


// should be 8251 UART


static READ8_HANDLER(pimps_terminal_status_r)
{
	pimps_state *state = space->machine->driver_data<pimps_state>();
	if (state->received_char!=0) return 3; // char received
	return 1; // ready
}

static READ8_DEVICE_HANDLER(pimps_terminal_r)
{
	pimps_state *state = device->machine->driver_data<pimps_state>();
	UINT8 retVal = state->received_char;
	state->received_char = 0;
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
static INPUT_PORTS_START( pimps )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(pimps)
{
	pimps_state *state = machine->driver_data<pimps_state>();
	state->received_char = 0;
}

static WRITE8_DEVICE_HANDLER( pimps_kbd_put )
{
	pimps_state *state = device->machine->driver_data<pimps_state>();
	state->received_char = data;
}

static GENERIC_TERMINAL_INTERFACE( pimps_terminal_intf )
{
	DEVCB_HANDLER(pimps_kbd_put)
};

static MACHINE_CONFIG_START( pimps, pimps_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8085A, XTAL_2MHz)
    MCFG_CPU_PROGRAM_MAP(pimps_mem)
    MCFG_CPU_IO_MAP(pimps_io)

    MCFG_MACHINE_RESET(pimps)

    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,pimps_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pimps )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "pimps.bin", 0xf000, 0x1000, CRC(5da1898f) SHA1(d20e31d0981a1f54c83186dbdfcf4280e49970d0))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 197?, pimps,  0,       0, 	pimps,	pimps,	 0, 		"Henry Colford",   "P.I.M.P.S.",		GAME_NOT_WORKING | GAME_NO_SOUND)
