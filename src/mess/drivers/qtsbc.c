/***************************************************************************

        QT Computer Systems SBC +2/4

        11/12/2009 Skeleton driver.

    It expects a rom or similar at E377-up, so currently it crashes.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"


class qtsbc_state : public driver_device
{
public:
	qtsbc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram;
	UINT8 m_term_data;
};



static WRITE8_HANDLER( qtsbc_06_w )
{
	device_t *terminal = space->machine().device(TERMINAL_TAG);

	terminal_write(terminal, 0, data);
}

static READ8_HANDLER( qtsbc_06_r )
{
	qtsbc_state *state = space->machine().driver_data<qtsbc_state>();
	UINT8 ret = state->m_term_data;
	state->m_term_data = 0;
	return ret;
}

static READ8_HANDLER( qtsbc_43_r )
{
	return 0;
}

static ADDRESS_MAP_START(qtsbc_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_RAM AM_BASE_MEMBER(qtsbc_state, m_ram) AM_REGION("maincpu",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qtsbc_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x06, 0x06) AM_READWRITE(qtsbc_06_r,qtsbc_06_w)
	AM_RANGE(0x43, 0x43) AM_READ(qtsbc_43_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( qtsbc )
INPUT_PORTS_END


static MACHINE_RESET(qtsbc)
{
	qtsbc_state *state = machine.driver_data<qtsbc_state>();
	UINT8* bios = machine.region("maincpu")->base()+0x10000;
	memcpy(state->m_ram,bios, 0x800);
}

static WRITE8_DEVICE_HANDLER( qtsbc_kbd_put )
{
	qtsbc_state *state = device->machine().driver_data<qtsbc_state>();
	state->m_term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( qtsbc_terminal_intf )
{
	DEVCB_HANDLER(qtsbc_kbd_put)
};

static MACHINE_CONFIG_START( qtsbc, qtsbc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz) // Mostek MK3880
	MCFG_CPU_PROGRAM_MAP(qtsbc_mem)
	MCFG_CPU_IO_MAP(qtsbc_io)

	MCFG_MACHINE_RESET(qtsbc)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, qtsbc_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( qtsbc )
	ROM_REGION( 0x10800, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "qtsbc.bin", 0x10000, 0x0800, CRC(823fd942) SHA1(64c4f74dd069ae4d43d301f5e279185f32a1efa0))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 19??, qtsbc,  0,       0, 	qtsbc,	qtsbc,	 0, 		 "Computer Systems Inc.",   "QT SBC +2/4",		GAME_NOT_WORKING | GAME_NO_SOUND)
