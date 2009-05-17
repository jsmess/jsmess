/***************************************************************************

	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000


****************************************************************************/

#include "driver.h"
#include "includes/mbee.h"
#include "cpu/z80/z80.h"


static UINT8 fdc_status = 0;
static const device_config *mbee_fdc;


/*
  On reset or power on, a circuit forces rom 8000-8FFF to appear at 0000-0FFF, while ram is disabled.
  It gets set back to normal on the first attempt to write to memory. (/WR line goes active).
*/


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( mbee_reset )
{
	memory_set_bank(machine, 1, 0);
}

MACHINE_RESET( mbee )
{
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
	memory_set_bank(machine, 1, 1);
	mbee_z80pio = devtag_get_device(machine, "z80pio");
	mbee_speaker = devtag_get_device(machine, "speaker");
	mbee_cassette = devtag_get_device(machine, "cassette");
	mbee_printer = devtag_get_device(machine, "centronics");
	mbee_fdc = devtag_get_device(machine, "wd179x");
}

static WD17XX_CALLBACK( mbee_fdc_callback )
{
	if (WD17XX_IRQ_SET || WD17XX_DRQ_SET)
		fdc_status |= 0x80;
	else
		fdc_status &= 0x7f;
}

const wd17xx_interface mbee_wd17xx_interface = { mbee_fdc_callback, NULL };

READ8_HANDLER ( mbee_fdc_status_r )
{
	return fdc_status;
}

WRITE8_HANDLER ( mbee_fdc_motor_w )
{
/*	d7..d4 not used
	d3 density (1=MFM)
	d2 side (1=side 1)
	d1..d0 drive select (0 to 3 - although no microbee ever had more than 2 drives) */

	wd17xx_set_drive(mbee_fdc, data & 3);
	wd17xx_set_side(mbee_fdc, (data & 4) ? 1 : 0);
	wd17xx_set_density(mbee_fdc, (data & 8) ? 1 : 0);
}

