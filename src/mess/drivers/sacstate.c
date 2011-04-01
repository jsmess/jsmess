/***************************************************************************
   
        SacState 8008

        23/02/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8008/i8008.h"
#include "machine/terminal.h"

class sacstate_state : public driver_device
{
public:
	sacstate_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_term_data;
};

UINT8 val = 0x00;
static READ8_HANDLER(status_r)
{
	UINT8 old_val = val;	
	sacstate_state *state = space->machine().driver_data<sacstate_state>();
	if (state->m_term_data!=0) old_val |= 0x04; // data in
	val = val ^ 0x40;
	return old_val;
}

static READ8_HANDLER(tty_r)
{
	sacstate_state *state = space->machine().driver_data<sacstate_state>();
	UINT8 retVal = state->m_term_data;
	state->m_term_data = 0;
	return retVal;
}
static WRITE8_HANDLER(tty_w)
{
	device_t *devconf = space->machine().device("terminal");
	logerror("TTY : %02x [%c]\n",data,data);
	terminal_write(devconf,0,data);
}

static READ8_HANDLER(unknown_r)
{
	logerror("unknown_r\n");
	return 0;
}

static ADDRESS_MAP_START(sacstate_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000,0x7ff) AM_ROM
	AM_RANGE(0x800,0xfff) AM_RAM	
ADDRESS_MAP_END

static ADDRESS_MAP_START( sacstate_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00,0x00) AM_READ(tty_r)
	AM_RANGE(0x01,0x01) AM_READ(status_r)
	AM_RANGE(0x04,0x04) AM_READ(unknown_r)
	AM_RANGE(0x16,0x16) AM_READ(tty_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( sacstate )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END

static WRITE8_DEVICE_HANDLER( sacstate_kbd_put )
{
	sacstate_state *state = device->machine().driver_data<sacstate_state>();
	state->m_term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( sacstate_terminal_intf )
{
	DEVCB_HANDLER(sacstate_kbd_put)
};

static MACHINE_RESET(sacstate) 
{	
}

static MACHINE_CONFIG_START( sacstate, sacstate_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8008, 800000)
    MCFG_CPU_PROGRAM_MAP(sacstate_mem)
    MCFG_CPU_IO_MAP(sacstate_io)	

    MCFG_MACHINE_RESET(sacstate)
	
    /* video hardware */
    MCFG_FRAGMENT_ADD( generic_terminal )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG,sacstate_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sacstate )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sacst1.bin", 0x0700, 0x0100, CRC(ba020160) SHA1(6337cdf65583808768664653c937e50040aec6d4))
	ROM_LOAD( "sacst2.bin", 0x0600, 0x0100, CRC(26f3e505) SHA1(3526060dbd1bf885c2e686bc9a6082387630952a))
	ROM_LOAD( "sacst3.bin", 0x0500, 0x0100, CRC(965b3474) SHA1(6d9142e68d375fb000fd6ea48369d0801274ded6))
	ROM_LOAD( "sacst4.bin", 0x0400, 0x0100, CRC(3cd3e169) SHA1(75a99e8e4dbd6e054209a4979bb498f37e962697))
	ROM_LOAD( "sacst5.bin", 0x0300, 0x0100, CRC(30619454) SHA1(cb498880bec27c9adc44dc1267858555000452c6))
	ROM_LOAD( "sacst6.bin", 0x0200, 0x0100, CRC(a4cd2ff6) SHA1(3f4da5510c0778eb770c96c01f91f5cb7f5285fa))
	ROM_LOAD( "sacst7.bin", 0x0100, 0x0100, CRC(33971d8b) SHA1(9e0bbeef6a6a15107f270e8b285300284ee7f63f))
	ROM_LOAD( "sacst8.bin", 0x0000, 0x0100, CRC(931252ef) SHA1(e06ea6947f432f0a4ce944de74978d929920fb53))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, sacstate,  0,       0, 	sacstate, 	sacstate, 	 0,  "SacState",   "SacState 8008",		GAME_NOT_WORKING | GAME_NO_SOUND)

