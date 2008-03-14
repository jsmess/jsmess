/***************************************************************************

		UT88 machine driver by Miodrag Milanovic

		06/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"


static int ut88_keyboard_line;


/* Driver initialization */
DRIVER_INIT(ut88)
{

}


READ8_HANDLER (ut88_8255_portb_r )
{
	return readinputport(ut88_keyboard_line);
}

READ8_HANDLER (ut88_8255_portc_r )
{
	return readinputport(8);	
}

WRITE8_HANDLER (ut88_8255_porta_w )
{	
	switch (data ^ 0xff) {
	  	case 0x01 : ut88_keyboard_line = 0;break;
	  	case 0x02 : ut88_keyboard_line = 1;break;
	  	case 0x04 : ut88_keyboard_line = 2;break;
	  	case 0x08 : ut88_keyboard_line = 3;break;
	  	case 0x10 : ut88_keyboard_line = 4;break;
	  	case 0x20 : ut88_keyboard_line = 5;break;
	  	case 0x40 : ut88_keyboard_line = 6;break;
	  	case 0x80 : ut88_keyboard_line = 7;break;
	}	
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

MACHINE_RESET( ut88 )
{
	ppi8255_init(&ut88_ppi8255_interface);
	ut88_keyboard_line = 0;
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
