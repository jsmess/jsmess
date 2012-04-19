/***************************************************************************

        SAPI-1 driver by Miodrag Milanovic

        09/09/2008 Preliminary driver.

****************************************************************************/

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

READ8_MEMBER( sapi1_state::sapi1_keyboard_r )
{
	UINT8 key = 0xff;
	if (BIT(m_keyboard_mask, 0)) { key &= input_port_read(machine(),"LINE0"); }
	if (BIT(m_keyboard_mask, 1)) { key &= input_port_read(machine(),"LINE1"); }
	if (BIT(m_keyboard_mask, 2)) { key &= input_port_read(machine(),"LINE2"); }
	if (BIT(m_keyboard_mask, 3)) { key &= input_port_read(machine(),"LINE3"); }
	if (BIT(m_keyboard_mask, 4)) { key &= input_port_read(machine(),"LINE4"); }
	return key;
}

WRITE8_MEMBER( sapi1_state::sapi1_keyboard_w )
{
	m_keyboard_mask = (data ^ 0xff ) & 0x1f;
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
