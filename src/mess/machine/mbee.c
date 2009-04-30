/***************************************************************************

	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000


****************************************************************************/

#include "driver.h"
#include "machine/wd17xx.h"
#include "includes/mbee.h"
#include "cpu/z80/z80.h"


static UINT8 fdc_drv = 0;
static UINT8 fdc_head = 0;
static UINT8 fdc_den = 0;
static UINT8 fdc_status = 0;


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
}

static WD17XX_CALLBACK( mbee_fdc_callback )
{
	switch( state )
	{
	case WD17XX_IRQ_CLR:
//		cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
		fdc_status &= ~0x40;
        break;
	case WD17XX_IRQ_SET:
//		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
		fdc_status |= 0x40;
        break;
	case WD17XX_DRQ_CLR:
		fdc_status &= ~0x80;
		break;
	case WD17XX_DRQ_SET:
		fdc_status |= 0x80;
        break;
    }
}

const wd17xx_interface mbee_wd17xx_interface = { mbee_fdc_callback, NULL };

 READ8_HANDLER ( mbee_fdc_status_r )
{
	logerror("mbee fdc_motor_r $%02X\n", fdc_status);
	return fdc_status;
}

WRITE8_HANDLER ( mbee_fdc_motor_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	logerror("mbee fdc_motor_w $%02X\n", data);
	/* Controller latch bits
	 * 0-1	driver select
	 * 2	head select
	 * 3	density (0: FM, 1:MFM)
	 * 4-7	unused
	 */
	fdc_drv = data & 3;
	fdc_head = (data >> 2) & 1;
	fdc_den = (data >> 3) & 1;
	wd17xx_set_drive(fdc,fdc_drv);
	wd17xx_set_side(fdc,fdc_head);
	if (data & (1<<3))
	{
	   wd17xx_set_density(fdc,DEN_FM_HI);
	}
	else
	{
	   wd17xx_set_density(fdc,DEN_MFM_LO);
	}

}

