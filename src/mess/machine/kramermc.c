/***************************************************************************

        Kramer MC machine driver by Miodrag Milanovic

        13/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "machine/z80pio.h"
#include "includes/kramermc.h"

UINT8 kramermc_key_row;

READ8_HANDLER (kramermc_port_a_r)
{
	return 0xff;
}

READ8_HANDLER (kramermc_port_b_r)
{
	static const char *keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	return input_port_read(machine, keynames[kramermc_key_row]);
}

WRITE8_HANDLER (kramermc_port_a_w)
{
	kramermc_key_row = ((data >> 1) & 0x07);
}

static const z80pio_interface pio_intf =
{
	NULL,	/* callback when change interrupt status */
	kramermc_port_a_r,
	kramermc_port_b_r,
	kramermc_port_a_w,
	NULL,
	0,				/* portA ready active callback (do not support yet)*/
	0				/* portB ready active callback (do not support yet)*/
};

/* Driver initialization */
DRIVER_INIT(kramermc)
{
}

MACHINE_RESET( kramermc )
{
	kramermc_key_row = 0;
	z80pio_init(0, &pio_intf);
}
