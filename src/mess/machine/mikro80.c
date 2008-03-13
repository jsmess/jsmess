/***************************************************************************

		Mikro-80 machine driver by Miodrag Milanovic

		10/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include <stdio.h>


static int mikro80_keyboard_line;


/* Driver initialization */
DRIVER_INIT(mikro80)
{
}

MACHINE_RESET( mikro80 )
{
	mikro80_keyboard_line = 0;
}


READ8_HANDLER( mikro80_keyboard_r )
{	
	if (offset==2) {		
  		switch (mikro80_keyboard_line) {
	  		case 0x01 : return readinputport(0);break;
	  		case 0x02 : return readinputport(1);break;
	  		case 0x04 : return readinputport(2);break;
	  		case 0x08 : return readinputport(3);break;
	  		case 0x10 : return readinputport(4);break;
	  		case 0x20 : return readinputport(5);break;
	  		case 0x40 : return readinputport(6);break;
	  		case 0x80 : return readinputport(7);break;
		}
	}
	if (offset==1) {
		return readinputport(8) & 0x07;
	}
	return 0xff;
}

WRITE8_HANDLER( mikro80_keyboard_w )
{
  	mikro80_keyboard_line = data ^ 0xff;
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
