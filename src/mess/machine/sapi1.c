/***************************************************************************

        SAPI-1 driver by Miodrag Milanovic

        09/09/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "includes/sapi1.h"


/* Driver initialization */
DRIVER_INIT(sapi1)
{
}

MACHINE_RESET( sapi1 )
{
	sapi1_state *state = machine.driver_data<sapi1_state>();
	state->m_keyboard_mask = 0;
}

MACHINE_START(sapi1)
{
}

READ8_HANDLER (sapi1_keyboard_r )
{
	sapi1_state *state = space->machine().driver_data<sapi1_state>();
	UINT8 key = 0xff;
	if ((state->m_keyboard_mask & 0x01)!=0) { key &= input_port_read(space->machine(),"LINE0"); }
	if ((state->m_keyboard_mask & 0x02)!=0) { key &= input_port_read(space->machine(),"LINE1"); }
	if ((state->m_keyboard_mask & 0x04)!=0) { key &= input_port_read(space->machine(),"LINE2"); }
	if ((state->m_keyboard_mask & 0x08)!=0) { key &= input_port_read(space->machine(),"LINE3"); }
	if ((state->m_keyboard_mask & 0x10)!=0) { key &= input_port_read(space->machine(),"LINE4"); }
	return key;
}
WRITE8_HANDLER(sapi1_keyboard_w )
{
	sapi1_state *state = space->machine().driver_data<sapi1_state>();
	state->m_keyboard_mask = (data ^ 0xff ) & 0x1f;
}


MACHINE_RESET( sapizps3 )
{
	sapi1_state *state = machine.driver_data<sapi1_state>();
	state->m_keyboard_mask = 0;
	memory_set_bank(machine, "bank1", 1);
}

DRIVER_INIT( sapizps3 )
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "bank1", 0, 2, &RAM[0x0000], 0xf800);
}
