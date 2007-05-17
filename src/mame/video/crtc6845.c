/**********************************************************************

    Motorola 6845 CRT Controller interface and emulation

    This function emulates the functionality of a single
    crtc6845.

    This is just a storage shell for crtc6845 variables due
    to the fact that the hardware implementation makes a big
    difference and that is done by the specific video code.

**********************************************************************/

#include "driver.h"
#include "crtc6845.h"

crtc6845_state crtc6845;

void crtc6845_init(void)
{
	state_save_register_item("crtc6845", 0, crtc6845.address_latch);
	state_save_register_item("crtc6845", 0, crtc6845.horiz_total);
	state_save_register_item("crtc6845", 0, crtc6845.horiz_disp);
	state_save_register_item("crtc6845", 0, crtc6845.horiz_sync_pos);
	state_save_register_item("crtc6845", 0, crtc6845.sync_width);
	state_save_register_item("crtc6845", 0, crtc6845.vert_total);
	state_save_register_item("crtc6845", 0, crtc6845.vert_total_adj);
	state_save_register_item("crtc6845", 0, crtc6845.vert_disp);
	state_save_register_item("crtc6845", 0, crtc6845.vert_sync_pos);
	state_save_register_item("crtc6845", 0, crtc6845.intl_skew);
	state_save_register_item("crtc6845", 0, crtc6845.max_ras_addr);
	state_save_register_item("crtc6845", 0, crtc6845.cursor_start_ras);
	state_save_register_item("crtc6845", 0, crtc6845.cursor_end_ras);
	state_save_register_item("crtc6845", 0, crtc6845.start_addr);
	state_save_register_item("crtc6845", 0, crtc6845.cursor);
	state_save_register_item("crtc6845", 0, crtc6845.light_pen);
	state_save_register_item("crtc6845", 0, crtc6845.page_flip);
}


READ8_HANDLER( crtc6845_register_r )
{
	int retval = 0xff;

	switch (crtc6845.address_latch)
	{
		case 0:		/* write-only */
		case 1:		/* write-only */
		case 2:		/* write-only */
		case 3:		/* write-only */
		case 4:		/* write-only */
		case 5:		/* write-only */
		case 6:		/* write-only */
		case 7:		/* write-only */
		case 8:		/* write-only */
		case 9:		/* write-only */
		case 10:	/* write-only */
		case 11:	/* write-only */
		case 12:	/* write-only */
		case 13:	/* write-only */
			break;
		case 14:
			retval = (crtc6845.cursor & 0x3f) >> 8;
			break;
		case 15:
			retval = crtc6845.cursor & 0xff;
			break;
		case 16:
			retval = (crtc6845.light_pen & 0x3f) >> 8;
			break;
		case 17:
			retval = crtc6845.light_pen & 0xff;
			break;
		default:
			break;
	}
	return retval;
}


WRITE8_HANDLER( crtc6845_address_w )
{
	crtc6845.address_latch = data & 0x1f;
}


WRITE8_HANDLER( crtc6845_register_w )
{

//logerror("CRT #0 PC %04x: CRTC6845 WRITE reg 0x%02x data 0x%02x\n",activecpu_get_pc(),crtc6845.address_latch,data);

	switch (crtc6845.address_latch)
	{
		case 0:
			crtc6845.horiz_total = data;
			break;
		case 1:
			crtc6845.horiz_disp = data;
			break;
		case 2:
			crtc6845.horiz_sync_pos = data;
			break;
		case 3:
			crtc6845.sync_width = data & 0x0f;
			break;
		case 4:
			crtc6845.vert_total = data & 0x7f;
			break;
		case 5:
			crtc6845.vert_total_adj = data & 0x1f;
			break;
		case 6:
			crtc6845.vert_disp = data & 0x7f;
			break;
		case 7:
			crtc6845.vert_sync_pos = data & 0x7f;
			break;
		case 8:
			crtc6845.intl_skew = data & 0x03;
			break;
		case 9:
			crtc6845.max_ras_addr = data & 0x1f;
			break;
		case 10:
			crtc6845.cursor_start_ras = data & 0x7f;
			break;
		case 11:
			crtc6845.cursor_end_ras = data & 0x1f;
			break;
		case 12:
			crtc6845.start_addr &= 0x00ff;
			crtc6845.start_addr |= (data & 0x3f) << 8;
			crtc6845.page_flip = data & 0x40;
			break;
		case 13:
			crtc6845.start_addr &= 0xff00;
			crtc6845.start_addr |= data;
			break;
		case 14:
			crtc6845.cursor &= 0x00ff;
			crtc6845.cursor |= (data & 0x3f) << 8;
			break;
		case 15:
			crtc6845.cursor &= 0xff00;
			crtc6845.cursor |= data;
			break;
		case 16:	/* read-only */
			break;
		case 17:	/* read-only */
			break;
		default:
			break;
	}
}
