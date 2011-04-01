/***************************************************************************

        CZK-80

        30/08/2010 Skeleton driver

        On main board there are Z80A CPU, Z80A PIO, Z80A DART and Z80A CTC
            there is 8K ROM and XTAL 16MHz
        FDC board contains Z80A DMA and NEC 765A (XTAL on it is 8MHZ)
        Mega board contains 74LS612 and memory chips

    27/11/2010 Connected to a terminal

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"


class czk80_state : public driver_device
{
public:
	czk80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram;
	UINT8 m_term_data;
};


static WRITE8_HANDLER( czk80_80_w )
{
	device_t *terminal = space->machine().device(TERMINAL_TAG);

	terminal_write(terminal, 0, data);
}

static READ8_HANDLER( czk80_80_r )
{
	czk80_state *state = space->machine().driver_data<czk80_state>();
	UINT8 ret = state->m_term_data;
	state->m_term_data = 0;
	return ret;
}

static READ8_HANDLER( czk80_c0_r )
{
	return 0x80;
}

static READ8_HANDLER( czk80_81_r )
{
	czk80_state *state = space->machine().driver_data<czk80_state>();
	return 1 | ((state->m_term_data) ? 2 : 0);
}

static ADDRESS_MAP_START(czk80_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE_MEMBER(czk80_state, m_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( czk80_io, AS_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x80) AM_READWRITE(czk80_80_r,czk80_80_w)
	AM_RANGE(0x81, 0x81) AM_READ(czk80_81_r)
	AM_RANGE(0xc0, 0xc0) AM_READ(czk80_c0_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( czk80 )
INPUT_PORTS_END

static MACHINE_RESET(czk80)
{
	czk80_state *state = machine.driver_data<czk80_state>();
	UINT8* bios = machine.region("maincpu")->base() + 0xe000;

	memcpy(state->m_ram,bios, 0x2000);
	memcpy(state->m_ram+0xe000,bios, 0x2000);
}


static WRITE8_DEVICE_HANDLER( czk80_kbd_put )
{
	czk80_state *state = device->machine().driver_data<czk80_state>();
	state->m_term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( czk80_terminal_intf )
{
	DEVCB_HANDLER(czk80_kbd_put)
};

static MACHINE_CONFIG_START( czk80, czk80_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
	MCFG_CPU_PROGRAM_MAP(czk80_mem)
	MCFG_CPU_IO_MAP(czk80_io)

	MCFG_MACHINE_RESET(czk80)

	MCFG_FRAGMENT_ADD( generic_terminal )

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, czk80_terminal_intf)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( czk80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "czk80.rom", 0xe000, 0x2000, CRC(7081b7c6) SHA1(13f75b14ea73b252bdfa2384e6eead6e720e49e3))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 198?, czk80,  0,       0, 	czk80,	czk80,	 0, 	  "<unknown>",   "CZK-80",		GAME_NOT_WORKING | GAME_NO_SOUND)
