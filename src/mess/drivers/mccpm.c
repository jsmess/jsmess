/***************************************************************************

        mc-CP/M-Computer

        31/08/2010 Skeleton driver.
        18/11/2010 Connected to a terminal

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

class mccpm_state : public driver_device
{
public:
	mccpm_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	DECLARE_READ8_MEMBER( mccpm_f0_r );
	DECLARE_WRITE8_MEMBER( kbd_put );
	UINT8 *m_ram;
	UINT8 m_term_data;
	virtual void machine_reset();
};



READ8_MEMBER( mccpm_state::mccpm_f0_r )
{
	UINT8 ret = m_term_data;
	m_term_data = 0;
	return ret;
}

static ADDRESS_MAP_START(mccpm_mem, AS_PROGRAM, 8, mccpm_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(m_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mccpm_io, AS_IO, 8, mccpm_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf0) AM_READ(mccpm_f0_r) AM_DEVWRITE_LEGACY(TERMINAL_TAG, terminal_write)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mccpm )
INPUT_PORTS_END


MACHINE_RESET_MEMBER(mccpm_state)
{
	UINT8* bios = m_machine.region("maincpu")->base();
	memcpy(m_ram, bios, 0x1000);
}

WRITE8_MEMBER( mccpm_state::kbd_put )
{
	m_term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(mccpm_state, kbd_put)
};

static MACHINE_CONFIG_START( mccpm, mccpm_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(mccpm_mem)
	MCFG_CPU_IO_MAP(mccpm_io)

	/* video hardware */
	MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mccpm )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon36.j15", 0x0000, 0x1000, CRC(9c441537) SHA1(f95bad52d9392b8fc9d9b8779b7b861672a0022b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, mccpm,  0,       0,      mccpm,     mccpm,	 0, "GRAF Elektronik Systeme GmbH", "mc-CP/M-Computer",	GAME_NOT_WORKING | GAME_NO_SOUND)

