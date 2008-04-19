/***************************************************************************

		Mikro-80 machine driver by Miodrag Milanovic

		10/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"

static int mikro80_keyboard_line;

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
	return input_port_read_indexed(machine, mikro80_keyboard_line);
}

READ8_HANDLER (mikro80_8255_portc_r )
{
	return input_port_read_indexed(machine, 8);	
}

WRITE8_HANDLER (mikro80_8255_porta_w )
{	
	switch (data ^ 0xff) {
	  	case 0x01 : mikro80_keyboard_line = 0;break;
	  	case 0x02 : mikro80_keyboard_line = 1;break;
	  	case 0x04 : mikro80_keyboard_line = 2;break;
	  	case 0x08 : mikro80_keyboard_line = 3;break;
	  	case 0x10 : mikro80_keyboard_line = 4;break;
	  	case 0x20 : mikro80_keyboard_line = 5;break;
	  	case 0x40 : mikro80_keyboard_line = 6;break;
	  	case 0x80 : mikro80_keyboard_line = 7;break;
	}	
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
	mikro80_keyboard_line = 0;
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
