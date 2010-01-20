/***************************************************************************

        UT88 machine driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"
#include "devices/cassette.h"
#include "machine/i8255a.h"
#include "includes/ut88.h"

static int ut88_keyboard_mask;

/* Driver initialization */
DRIVER_INIT(ut88)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(machine, "maincpu");
	memset(RAM,0x0000,0x0800); // make frist page empty by default
  	memory_configure_bank(machine, "bank1", 1, 2, RAM, 0x0000);
	memory_configure_bank(machine, "bank1", 0, 2, RAM, 0xf800);
}

static READ8_DEVICE_HANDLER (ut88_8255_portb_r )
{
	UINT8 key = 0xff;
	if ((ut88_keyboard_mask & 0x01)!=0) { key &= input_port_read(device->machine,"LINE0"); }
	if ((ut88_keyboard_mask & 0x02)!=0) { key &= input_port_read(device->machine,"LINE1"); }
	if ((ut88_keyboard_mask & 0x04)!=0) { key &= input_port_read(device->machine,"LINE2"); }
	if ((ut88_keyboard_mask & 0x08)!=0) { key &= input_port_read(device->machine,"LINE3"); }
	if ((ut88_keyboard_mask & 0x10)!=0) { key &= input_port_read(device->machine,"LINE4"); }
	if ((ut88_keyboard_mask & 0x20)!=0) { key &= input_port_read(device->machine,"LINE5"); }
	if ((ut88_keyboard_mask & 0x40)!=0) { key &= input_port_read(device->machine,"LINE6"); }
	if ((ut88_keyboard_mask & 0x80)!=0) { key &= input_port_read(device->machine,"LINE7"); }
	return key;
}

static READ8_DEVICE_HANDLER (ut88_8255_portc_r )
{
	return input_port_read(device->machine, "LINE8");
}

static WRITE8_DEVICE_HANDLER (ut88_8255_porta_w )
{
	ut88_keyboard_mask = data ^ 0xff;
}

I8255A_INTERFACE( ut88_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_HANDLER(ut88_8255_portb_r),
	DEVCB_HANDLER(ut88_8255_portc_r),
	DEVCB_HANDLER(ut88_8255_porta_w),
	DEVCB_NULL,
	DEVCB_NULL,
};

static TIMER_CALLBACK( ut88_reset )
{
	memory_set_bank(machine, "bank1", 0);
}

MACHINE_RESET( ut88 )
{
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, ut88_reset);
	memory_set_bank(machine, "bank1", 1);
	ut88_keyboard_mask = 0;
}


READ8_DEVICE_HANDLER( ut88_keyboard_r )
{
	return i8255a_r(device, offset^0x03);
}


WRITE8_DEVICE_HANDLER( ut88_keyboard_w )
{
	i8255a_w(device, offset^0x03, data);
}

WRITE8_HANDLER( ut88_sound_w )
{
	const device_config *dac_device = devtag_get_device(space->machine, "dac");
	dac_data_w(dac_device, data); //beeper
	cassette_output(devtag_get_device(space->machine, "cassette"),data & 0x01 ? 1 : -1);
}


READ8_HANDLER( ut88_tape_r )
{
	double level = cassette_input(devtag_get_device(space->machine, "cassette"));
	if (level <  0) {
		 	return 0x00;
 	}
	return 0xff;
}

READ8_HANDLER( ut88mini_keyboard_r )
{
	// This is real keyboard implementation
	UINT8 *keyrom1 = memory_region(space->machine, "maincpu")+ 0x10000;
	UINT8 *keyrom2 = memory_region(space->machine, "maincpu")+ 0x10100;

	UINT8 key = keyrom2[input_port_read(space->machine, "LINE1")];
	// if keyboard 2nd part returned 0 on 4th bit output from
	// first part is used
	if ((key & 0x08) ==0x00) {
		key = keyrom1[input_port_read(space->machine, "LINE0")];
	}
	// for delete key there is special key producing code 0x80
	key = (input_port_read(space->machine, "LINE2") & 0x80)==0x80 ? key : 0x80;
	// If key 0 is pressed it value is 0x10 this is done by additional
	// discrete logic
	key = (input_port_read(space->machine, "LINE0") & 0x01)==0x01 ? key : 0x10;
	return key;
}

static int lcd_digit[6];

WRITE8_HANDLER( ut88mini_write_led )
{
		switch(offset) {
			case 0  : lcd_digit[4] = (data >> 4) & 0x0f;
								lcd_digit[5] = data & 0x0f;
								break;
			case 1  : lcd_digit[2] = (data >> 4) & 0x0f;
								lcd_digit[3] = data & 0x0f;
								break;
			case 2  : lcd_digit[0] = (data >> 4) & 0x0f;
								lcd_digit[1] = data & 0x0f;
								break;
		}
}

static const UINT8 hex_to_7seg[16] =
{
	 0x3F, 0x06, 0x5B, 0x4F,
	 0x66, 0x6D, 0x7D, 0x07,
	 0x7F, 0x6F, 0x77, 0x7c,
	 0x39, 0x5e, 0x79, 0x71
};

static TIMER_CALLBACK( update_display )
{
	int i;
	for (i=0;i<6;i++) {
		output_set_digit_value(i, hex_to_7seg[lcd_digit[i]]);
	}
}


DRIVER_INIT(ut88mini)
{

}
MACHINE_START( ut88mini )
{
	timer_pulse(machine, ATTOTIME_IN_HZ(60), NULL, 0, update_display);
}

MACHINE_RESET( ut88mini )
{
	lcd_digit[0] = lcd_digit[1] = lcd_digit[2] = 0;
	lcd_digit[3] = lcd_digit[4] = lcd_digit[5] = 0;
}

