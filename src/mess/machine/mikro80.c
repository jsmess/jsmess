/***************************************************************************

        Mikro-80 machine driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/i8255a.h"
#include "includes/mikro80.h"

static int mikro80_keyboard_mask;
static int mikro80_key_mask;
/* Driver initialization */
DRIVER_INIT(mikro80)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(machine, "maincpu");
	memset(RAM,0x0000,0x0800); // make frist page empty by default
  	memory_configure_bank(machine, 1, 1, 2, RAM, 0x0000);
	memory_configure_bank(machine, 1, 0, 2, RAM, 0xf800);
	mikro80_key_mask = 0x7f;
}

DRIVER_INIT(radio99)
{
	DRIVER_INIT_CALL(mikro80);
	mikro80_key_mask = 0xff;
}

READ8_HANDLER (mikro80_8255_portb_r )
{
	UINT8 key = 0xff;
	if ((mikro80_keyboard_mask & 0x01)!=0) { key &= input_port_read(space->machine,"LINE0"); }
	if ((mikro80_keyboard_mask & 0x02)!=0) { key &= input_port_read(space->machine,"LINE1"); }
	if ((mikro80_keyboard_mask & 0x04)!=0) { key &= input_port_read(space->machine,"LINE2"); }
	if ((mikro80_keyboard_mask & 0x08)!=0) { key &= input_port_read(space->machine,"LINE3"); }
	if ((mikro80_keyboard_mask & 0x10)!=0) { key &= input_port_read(space->machine,"LINE4"); }
	if ((mikro80_keyboard_mask & 0x20)!=0) { key &= input_port_read(space->machine,"LINE5"); }
	if ((mikro80_keyboard_mask & 0x40)!=0) { key &= input_port_read(space->machine,"LINE6"); }
	if ((mikro80_keyboard_mask & 0x80)!=0) { key &= input_port_read(space->machine,"LINE7"); }
	return key & mikro80_key_mask;
}

READ8_HANDLER (mikro80_8255_portc_r )
{
	return input_port_read(space->machine, "LINE8");
}

WRITE8_HANDLER (mikro80_8255_porta_w )
{
	mikro80_keyboard_mask = data ^ 0xff;
}

WRITE8_HANDLER (mikro80_8255_portc_w )
{
}

static READ8_DEVICE_HANDLER( mikro80_8255_portb_device_r ) { return mikro80_8255_portb_r(cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM), offset); }
static READ8_DEVICE_HANDLER( mikro80_8255_portc_device_r ) { return mikro80_8255_portc_r(cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM), offset); }
static WRITE8_DEVICE_HANDLER( mikro80_8255_porta_device_w ) { mikro80_8255_porta_w(cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM), offset, data); }

I8255A_INTERFACE( mikro80_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_HANDLER(mikro80_8255_portb_device_r),
	DEVCB_HANDLER(mikro80_8255_portc_device_r),
	DEVCB_HANDLER(mikro80_8255_porta_device_w),
	DEVCB_NULL,
	DEVCB_NULL,
};


static TIMER_CALLBACK( mikro80_reset )
{
	memory_set_bank(machine,1, 0);
}

MACHINE_RESET( mikro80 )
{
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, mikro80_reset);
	memory_set_bank(machine, 1, 1);
	mikro80_keyboard_mask = 0;
}


READ8_DEVICE_HANDLER( mikro80_keyboard_r )
{
	return i8255a_r(device, offset^0x03);
}

WRITE8_DEVICE_HANDLER( mikro80_keyboard_w )
{
	i8255a_w(device, offset^0x03, data);
}


WRITE8_HANDLER( mikro80_tape_w )
{
	cassette_output(devtag_get_device(space->machine, "cassette"),data & 0x01 ? 1 : -1);
}


READ8_HANDLER( mikro80_tape_r )
{
	double level = cassette_input(devtag_get_device(space->machine, "cassette"));
	if (level <  0) {
		 	return 0x00;
 	}
	return 0xff;
}
