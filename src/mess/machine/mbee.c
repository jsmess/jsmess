/***************************************************************************

	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

	"busnop" emulation by R. Belmont, May 2006.

	"busnop" is a trick by which all reads from 0x0000-0x7FFF are held at 0x00
	until A15 is asserted.  The Z80 NOP instruction is 0x00, so the effect is
	to cause reset to skip all memory up to 0x8000, where the actual handling
	code resides.

****************************************************************************/

#include "driver.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"
#include "includes/mbee.h"
#include "devices/cassette.h"
#include "cpu/z80/z80.h"


static UINT8 fdc_drv = 0;
static UINT8 fdc_head = 0;
static UINT8 fdc_den = 0;
static UINT8 fdc_status = 0;

UINT8 *mbee_workram;


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
}

static WD17XX_CALLBACK( mbee_fdc_callback )
{
	switch( state )
	{
	case WD17XX_IRQ_CLR:
//		cpu_set_input_line(machine->cpu[0],0,CLEAR_LINE);
		fdc_status &= ~0x40;
        break;
	case WD17XX_IRQ_SET:
//		cpu_set_input_line(machine->cpu[0],0,HOLD_LINE);
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
	device_config *fdc = (device_config*)device_list_find_by_tag( space->machine->config->devicelist, WD179X, "wd179x");
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

DEVICE_IMAGE_LOAD( mbee_cart )
{
	int size = image_length(image);
	UINT8 *mem = malloc(size);
	if (!mem)
		return INIT_FAIL;

	{
		if( image_fread(image, mem, size) == size )
		{
			memcpy(memory_region(image->machine, "main")+0x8000, mem, size);
		}
		free(mem);
	}
	return INIT_PASS;
}
