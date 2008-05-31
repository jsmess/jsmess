/***************************************************************************

        Mikro-80 machine driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"

static int mikro80_keyboard_mask;

/* Driver initialization */
DRIVER_INIT(mikro80)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(REGION_CPU1);
	memset(RAM,0x0000,0x0800); // make frist page empty by default
  	memory_configure_bank(1, 1, 2, RAM, 0x0000);
	memory_configure_bank(1, 0, 2, RAM, 0xf800);
}

READ8_HANDLER (mikro80_8255_portb_r )
{
	UINT8 key = 0xff;
	if ((mikro80_keyboard_mask & 0x01)!=0) { key &= input_port_read(machine,"LINE0"); }
	if ((mikro80_keyboard_mask & 0x02)!=0) { key &= input_port_read(machine,"LINE1"); }
	if ((mikro80_keyboard_mask & 0x04)!=0) { key &= input_port_read(machine,"LINE2"); }
	if ((mikro80_keyboard_mask & 0x08)!=0) { key &= input_port_read(machine,"LINE3"); }
	if ((mikro80_keyboard_mask & 0x10)!=0) { key &= input_port_read(machine,"LINE4"); }
	if ((mikro80_keyboard_mask & 0x20)!=0) { key &= input_port_read(machine,"LINE5"); }
	if ((mikro80_keyboard_mask & 0x40)!=0) { key &= input_port_read(machine,"LINE6"); }
	if ((mikro80_keyboard_mask & 0x80)!=0) { key &= input_port_read(machine,"LINE7"); }
	return key;
}

READ8_HANDLER (mikro80_8255_portc_r )
{
	return input_port_read(machine, "LINE8");
}

WRITE8_HANDLER (mikro80_8255_porta_w )
{
	mikro80_keyboard_mask = data ^ 0xff;
}

const ppi8255_interface mikro80_ppi8255_interface =
{
	NULL,
	mikro80_8255_portb_r,
	mikro80_8255_portc_r,
	mikro80_8255_porta_w,
	NULL,
	NULL,
};

static TIMER_CALLBACK( mikro80_reset )
{
	memory_set_bank(1, 0);
}

MACHINE_RESET( mikro80 )
{
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, mikro80_reset);
	memory_set_bank(1, 1);
	mikro80_keyboard_mask = 0;
}


READ8_DEVICE_HANDLER( mikro80_keyboard_r )
{
	return ppi8255_r(device, offset^0x03);
}

WRITE8_DEVICE_HANDLER( mikro80_keyboard_w )
{
	ppi8255_w(device, offset^0x03, data);
}


WRITE8_HANDLER( mikro80_tape_w )
{
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0),data & 0x01 ? 1 : -1);
}


READ8_HANDLER( mikro80_tape_r )
{
	double level = cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));
	if (level <  0) {
		 	return 0x00;
 	}
	return 0xff;
}
