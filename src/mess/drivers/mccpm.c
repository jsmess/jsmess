/***************************************************************************

        mc-CP/M-Computer

        31/08/2010 Skeleton driver.
        18/11/2010 Connected to a terminal

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"


class mccpm_state : public driver_device
{
public:
	mccpm_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram;
	UINT8 m_term_data;
};



static WRITE8_HANDLER( mccpm_f0_w )
{
	device_t *terminal = space->machine().device(TERMINAL_TAG);

	terminal_write(terminal, 0, data);
}

static READ8_HANDLER( mccpm_f0_r )
{
	mccpm_state *state = space->machine().driver_data<mccpm_state>();
	UINT8 ret = state->m_term_data;
	state->m_term_data = 0;
	return ret;
}

static ADDRESS_MAP_START(mccpm_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE_MEMBER(mccpm_state, m_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mccpm_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf0) AM_READWRITE(mccpm_f0_r,mccpm_f0_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mccpm )
INPUT_PORTS_END


static MACHINE_RESET(mccpm)
{
	mccpm_state *state = machine.driver_data<mccpm_state>();
	UINT8* bios = machine.region("maincpu")->base();

	memcpy(state->m_ram,bios, 0x1000);
}

static WRITE8_DEVICE_HANDLER( mccpm_kbd_put )
{
	mccpm_state *state = device->machine().driver_data<mccpm_state>();
	state->m_term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( mccpm_terminal_intf )
{
	DEVCB_HANDLER(mccpm_kbd_put)
};

static MACHINE_CONFIG_START( mccpm, mccpm_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(mccpm_mem)
	MCFG_CPU_IO_MAP(mccpm_io)

	MCFG_MACHINE_RESET(mccpm)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, mccpm_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mccpm )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon36.j15", 0x0000, 0x1000, CRC(9c441537) SHA1(f95bad52d9392b8fc9d9b8779b7b861672a0022b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, mccpm,  0,       0, 		mccpm,	mccpm,	 0, 	 "GRAF Elektronik Systeme GmbH",   "mc-CP/M-Computer",		GAME_NOT_WORKING | GAME_NO_SOUND)

