/***************************************************************************

        Mikro-80 machine driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "imagedev/cassette.h"
#include "machine/i8255.h"
#include "includes/mikro80.h"

/* Driver initialization */
DRIVER_INIT(mikro80)
{
	mikro80_state *state = machine.driver_data<mikro80_state>();
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = state->memregion("maincpu")->base();
	memset(RAM,0x0000,0x0800); // make frist page empty by default
	state->membank("bank1")->configure_entries(1, 2, RAM, 0x0000);
	state->membank("bank1")->configure_entries(0, 2, RAM, 0xf800);
	state->m_key_mask = 0x7f;
}

DRIVER_INIT(radio99)
{
	mikro80_state *state = machine.driver_data<mikro80_state>();
	DRIVER_INIT_CALL(mikro80);
	state->m_key_mask = 0xff;
}

READ8_MEMBER(mikro80_state::mikro80_8255_portb_r)
{
	UINT8 key = 0xff;
	if ((m_keyboard_mask & 0x01)!=0) { key &= input_port_read(machine(),"LINE0"); }
	if ((m_keyboard_mask & 0x02)!=0) { key &= input_port_read(machine(),"LINE1"); }
	if ((m_keyboard_mask & 0x04)!=0) { key &= input_port_read(machine(),"LINE2"); }
	if ((m_keyboard_mask & 0x08)!=0) { key &= input_port_read(machine(),"LINE3"); }
	if ((m_keyboard_mask & 0x10)!=0) { key &= input_port_read(machine(),"LINE4"); }
	if ((m_keyboard_mask & 0x20)!=0) { key &= input_port_read(machine(),"LINE5"); }
	if ((m_keyboard_mask & 0x40)!=0) { key &= input_port_read(machine(),"LINE6"); }
	if ((m_keyboard_mask & 0x80)!=0) { key &= input_port_read(machine(),"LINE7"); }
	return key & m_key_mask;
}

READ8_MEMBER(mikro80_state::mikro80_8255_portc_r)
{
	return input_port_read(machine(), "LINE8");
}

WRITE8_MEMBER(mikro80_state::mikro80_8255_porta_w)
{
	m_keyboard_mask = data ^ 0xff;
}

WRITE8_MEMBER(mikro80_state::mikro80_8255_portc_w)
{
}

I8255_INTERFACE( mikro80_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(mikro80_state, mikro80_8255_porta_w),
	DEVCB_DRIVER_MEMBER(mikro80_state, mikro80_8255_portb_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(mikro80_state, mikro80_8255_portc_r),
	DEVCB_NULL,
};


static TIMER_CALLBACK( mikro80_reset )
{
	mikro80_state *state = machine.driver_data<mikro80_state>();
	state->membank("bank1")->set_entry(0);
}

MACHINE_RESET( mikro80 )
{
	mikro80_state *state = machine.driver_data<mikro80_state>();
	machine.scheduler().timer_set(attotime::from_usec(10), FUNC(mikro80_reset));
	state->membank("bank1")->set_entry(1);
	state->m_keyboard_mask = 0;
}


READ8_MEMBER(mikro80_state::mikro80_keyboard_r)
{
	i8255_device *ppi = machine().device<i8255_device>("ppi8255");
	return ppi->read(space, offset^0x03);
}

WRITE8_MEMBER(mikro80_state::mikro80_keyboard_w)
{
	i8255_device *ppi = machine().device<i8255_device>("ppi8255");
	ppi->write(space, offset^0x03, data);
}


WRITE8_MEMBER(mikro80_state::mikro80_tape_w)
{
	machine().device<cassette_image_device>(CASSETTE_TAG)->output(data & 0x01 ? 1 : -1);
}


READ8_MEMBER(mikro80_state::mikro80_tape_r)
{
	double level = machine().device<cassette_image_device>(CASSETTE_TAG)->input();
	if (level <  0) {
			return 0x00;
	}
	return 0xff;
}
