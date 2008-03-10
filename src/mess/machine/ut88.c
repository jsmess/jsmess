/***************************************************************************

		UT88 machine driver by Miodrag Milanovic

		06/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"

static int ut88_keyboard_line;


/* Driver initialization */
DRIVER_INIT(ut88)
{
}

MACHINE_RESET( ut88 )
{
	ut88_keyboard_line = 0;
}


READ8_HANDLER( ut88_keyboard_r )
{
	if (offset==2) {
		return ut88_keyboard_line;
	}
	if (offset==1) {
		return readinputport(8) & 0x07;
	}
	 return 0xff;
}

WRITE8_HANDLER( ut88_keyboard_w )
{
  switch (data ^ 0xff) {
	  	case 0x01 : ut88_keyboard_line = readinputport(0) & 0x7f;break;
	  	case 0x02 : ut88_keyboard_line = readinputport(1) & 0x7f;break;
	  	case 0x04 : ut88_keyboard_line = readinputport(2) & 0x7f;break;
	  	case 0x08 : ut88_keyboard_line = readinputport(3) & 0x7f;break;
	  	case 0x10 : ut88_keyboard_line = readinputport(4) & 0x7f;break;
	  	case 0x20 : ut88_keyboard_line = readinputport(5) & 0x7f;break;
	  	case 0x40 : ut88_keyboard_line = readinputport(6) & 0x7f;break;
	  	case 0x80 : ut88_keyboard_line = readinputport(7) & 0x7f;break;
	}
}

WRITE8_HANDLER( ut88_sound_w )
{
	DAC_data_w(0,data); //beeper
}


