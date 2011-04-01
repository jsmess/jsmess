/***************************************************************************

        Orao machine driver by Miodrag Milanovic

        23/02/2008 Sound support added.
        22/02/2008 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "sound/dac.h"
#include "imagedev/cassette.h"
#include "includes/orao.h"


/* Driver initialization */
DRIVER_INIT(orao)
{
	orao_state *state = machine.driver_data<orao_state>();
	memset(state->m_memory,0xff,0x6000);
}

DRIVER_INIT(orao103)
{
	orao_state *state = machine.driver_data<orao_state>();
	memset(state->m_memory,0xff,0x6000);
}

MACHINE_RESET( orao )
{
}

READ8_HANDLER( orao_io_r )
{
	double level;

	 switch(offset) {
		/* Keyboard*/
		case 0x07FC : return input_port_read(space->machine(), "LINE0");
		case 0x07FD : return input_port_read(space->machine(), "LINE1");
		case 0x07FA : return input_port_read(space->machine(), "LINE2");
		case 0x07FB : return input_port_read(space->machine(), "LINE3");
		case 0x07F6 : return input_port_read(space->machine(), "LINE4");
		case 0x07F7 : return input_port_read(space->machine(), "LINE5");
		case 0x07EE : return input_port_read(space->machine(), "LINE6");
		case 0x07EF : return input_port_read(space->machine(), "LINE7");
		case 0x07DE : return input_port_read(space->machine(), "LINE8");
		case 0x07DF : return input_port_read(space->machine(), "LINE9");
		case 0x07BE : return input_port_read(space->machine(), "LINE10");
		case 0x07BF : return input_port_read(space->machine(), "LINE11");
		case 0x077E : return input_port_read(space->machine(), "LINE12");
		case 0x077F : return input_port_read(space->machine(), "LINE13");
		case 0x06FE : return input_port_read(space->machine(), "LINE14");
		case 0x06FF : return input_port_read(space->machine(), "LINE15");
		case 0x05FE : return input_port_read(space->machine(), "LINE16");
		case 0x05FF : return input_port_read(space->machine(), "LINE17");
		case 0x03FE : return input_port_read(space->machine(), "LINE18");
		case 0x03FF : return input_port_read(space->machine(), "LINE19");
		/* Tape */
		case 0x07FF :
					level = cassette_input(space->machine().device("cassette"));
					if (level <  0) {
						return 0x00;
					}
					return 0xff;
	 }


	 return 0xff;
}


WRITE8_HANDLER( orao_io_w )
{
	if (offset == 0x0800)
	{
		device_t *dac_device = space->machine().device("dac");
		dac_data_w(dac_device, data); //beeper
	}
}
