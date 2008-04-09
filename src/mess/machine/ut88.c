/***************************************************************************

		UT88 machine driver by Miodrag Milanovic

		06/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"

static int ut88_8255_porta;

/* Driver initialization */
DRIVER_INIT(ut88)
{
	/* set initialy ROM to be visible on first bank */
	UINT8 *RAM = memory_region(REGION_CPU1);	
	memset(RAM,0x0000,0x0800); // make frist page empty by default
  memory_configure_bank(1, 1, 2, RAM, 0x0000);
	memory_configure_bank(1, 0, 2, RAM, 0xf800);
}

READ8_HANDLER (ut88_8255_portb_r )
{
	switch (ut88_8255_porta ^ 0xff) {
	  	case 0x01 : return input_port_read_indexed(machine, 0);break;
	  	case 0x02 : return input_port_read_indexed(machine, 1);break;
	  	case 0x04 : return input_port_read_indexed(machine, 2);break;
	  	case 0x08 : return input_port_read_indexed(machine, 3);break;
	  	case 0x10 : return input_port_read_indexed(machine, 4);break;
	  	case 0x20 : return input_port_read_indexed(machine, 5);break;
	  	case 0x40 : return input_port_read_indexed(machine, 6);break;
	  	case 0x80 : return input_port_read_indexed(machine, 7);break;
	}	
	return 0xff;
}

READ8_HANDLER (ut88_8255_portc_r )
{
	return input_port_read_indexed(machine, 8);	
}

WRITE8_HANDLER (ut88_8255_porta_w )
{
	ut88_8255_porta = data;	
}

static const ppi8255_interface ut88_ppi8255_interface =
{
	1,
	{NULL},
	{ut88_8255_portb_r},
	{ut88_8255_portc_r},
	{ut88_8255_porta_w},
	{NULL},
	{NULL},
};

static TIMER_CALLBACK( ut88_reset )
{
	memory_set_bank(1, 0);
}

MACHINE_RESET( ut88 )
{
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, ut88_reset);
	memory_set_bank(1, 1);	
	ppi8255_init(&ut88_ppi8255_interface);
	ut88_8255_porta = 0;
}


READ8_HANDLER( ut88_keyboard_r )
{
	return ppi8255_0_r(machine, offset^0x03);	
}


WRITE8_HANDLER( ut88_keyboard_w )
{
	ppi8255_0_w(machine, offset^0x03, data);
}

WRITE8_HANDLER( ut88_sound_w )
{
	DAC_data_w(0,data); //beeper
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0),data & 0x01 ? 1 : -1);	
}


READ8_HANDLER( ut88_tape_r )
{
	double level = cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));	 									 					
	if (level <  0) { 
		 	return 0x00; 
 	}
	return 0xff;	
}

READ8_HANDLER( ut88mini_keyboard_r )
{
	return input_port_read_indexed(machine, 0);	
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

static int hex_to_7seg[16] = 
	{0x3F, 0x06, 0x5B, 0x4F, 
	 0x66, 0x6D, 0x7D, 0x07, 
	 0x7F, 0x6F, 0x77, 0x7c, 
	 0x39, 0x5e, 0x79, 0x71 };

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
	timer_pulse(ATTOTIME_IN_HZ(60), NULL, 0, update_display);
}

MACHINE_RESET( ut88mini )
{
	lcd_digit[0] = lcd_digit[1] = lcd_digit[2] = 0;
	lcd_digit[3] = lcd_digit[4] = lcd_digit[5] = 0;
}

