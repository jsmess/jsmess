/***************************************************************************

		Galeb machine driver by Miodrag Milanovic

		23/02/2008 Sound support added.
		22/02/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "sound/dac.h"
  
  
/* Driver initialization */
DRIVER_INIT(galeb)
{
}

MACHINE_RESET( galeb )
{
}

READ8_HANDLER( galeb_keyboard_r )
{
	char port[6];
	sprintf(port,"LINE%d",offset);
	return input_port_read(machine, port);
}

WRITE8_HANDLER( galeb_speaker_w )
{	 
	DAC_data_w(0,data);
}
