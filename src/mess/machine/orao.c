/***************************************************************************

		Orao machine driver by Miodrag Milanovic

		23/02/2008 Sound support added.
		22/02/2008 Preliminary driver.
		     
****************************************************************************/

#include "driver.h"
#include "sound/dac.h"
 
/* Driver initialization */
DRIVER_INIT(orao)
{
}

DRIVER_INIT(orao103)
{
}
 
MACHINE_RESET( orao )
{
}

READ8_HANDLER( orao_keyboard_r )
{
	 switch(offset) {
	 	case 0x07FC : return readinputport(0); break;
	 	case 0x07FD : return readinputport(1); break;
	 	case 0x07FA : return readinputport(2); break;
	 	case 0x07FB : return readinputport(3); break;
	 	case 0x07F6 : return readinputport(4); break;
	 	case 0x07F7 : return readinputport(5); break;
	 	case 0x07EE : return readinputport(6); break;
	 	case 0x07EF : return readinputport(7); break;
	 	case 0x07DE : return readinputport(8); break;
	 	case 0x07DF : return readinputport(9); break;
	 	case 0x07BE : return readinputport(10); break;
	 	case 0x07BF : return readinputport(11); break;
	 	case 0x077E : return readinputport(12); break;
	 	case 0x077F : return readinputport(13); break;
	 	case 0x06FE : return readinputport(14); break;
	 	case 0x06FF : return readinputport(15); break;
	 	case 0x05FE : return readinputport(16); break;
	 	case 0x05FF : return readinputport(17); break;
	 	case 0x03FE : return readinputport(18); break;
	 	case 0x03FF : return readinputport(19); break;
	 }
	 return 0xff;	
}


WRITE8_HANDLER( orao_io_w )
{	 
	if (offset == 0x0800) {		
		DAC_data_w(0,data); //beeper
	}
}


