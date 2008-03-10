/***************************************************************************

		Mikro-80 machine driver by Miodrag Milanovic

		10/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"

static int mikro80_keyboard_line;


/* Driver initialization */
DRIVER_INIT(mikro80)
{
}

static TIMER_CALLBACK(mikro80_bootstrap_callback)
{
	cpunum_set_reg(0, I8080_PC, 0xF800);
}

MACHINE_RESET( mikro80 )
{
	mikro80_keyboard_line = 0;
	timer_set(attotime_zero, NULL, 0, mikro80_bootstrap_callback);	
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

