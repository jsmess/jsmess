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
}

READ8_HANDLER (mikro80_8255_portb_r )
{	
	return readinputport(mikro80_keyboard_line);
}

READ8_HANDLER (mikro80_8255_portc_r )
{
	return readinputport(8) & 0x07;	
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

static const ppi8255_interface mikro80_ppi8255_interface =
{
	1,
	{NULL},
	{mikro80_8255_portb_r},
	{mikro80_8255_portc_r},
	{mikro80_8255_porta_w},
	{NULL},
	{NULL},
};

MACHINE_RESET( mikro80 )
{
	ppi8255_init(&mikro80_ppi8255_interface);
	mikro80_keyboard_line = 0;
}


READ8_HANDLER( mikro80_keyboard_r )
{
	return ppi8255_0_r(machine, offset^0x03);	
}

WRITE8_HANDLER( mikro80_keyboard_w )
{
	ppi8255_0_w(machine, offset^0x03, data);
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
