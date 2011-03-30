/**********************************************************************

    IBM EGA CRT Controller emulation

    This controller is very loosely based on the mc6845.

**********************************************************************/

#include "emu.h"
#include "crtc_ega.h"


#define LOG		(0)


/* device types */
enum
{
	TYPE_CRTC_EGA=0,
	TYPE_CRTC_VGA,

	NUM_TYPES
};


/* tags for state saving */
static const char * const device_tags[NUM_TYPES] = { "crtc_ega", "crtc_vga" };


typedef struct _crtc_ega_t crtc_ega_t;
struct _crtc_ega_t
{
	int device_type;
	const crtc_ega_interface *intf;
	screen_device *screen;

	/* ega/vga register file */
	UINT8	horiz_char_total;	/* 0x00 */
	UINT8	horiz_disp;			/* 0x01 */
	UINT8	horiz_blank_start;	/* 0x02 */
	UINT8	horiz_blank_end;	/* 0x03/0x05 */
	UINT8	ena_vert_access;	/* 0x03 */
	UINT8	de_skew;			/* 0x03 */
	UINT8	horiz_retr_start;	/* 0x04 */
	UINT8	horiz_retr_end;		/* 0x05 */
	UINT8	horiz_retr_skew;	/* 0x05 */
	UINT16	vert_total;			/* 0x06/0x07 */
	UINT8	preset_row_scan;	/* 0x08 */
	UINT8	byte_panning;		/* 0x08 */
	UINT8	max_ras_addr;		/* 0x09 */
	UINT8	scan_doubling;		/* 0x09 */
	UINT8	cursor_start_ras;	/* 0x0a */
	UINT8	cursor_disable;		/* 0x0a */
	UINT8	cursor_end_ras;		/* 0x0b */
	UINT8	cursor_skew;		/* 0x0b */
	UINT16	disp_start_addr;	/* 0x0c/0x0d */
	UINT16	cursor_addr;		/* 0x0e/0x0f */
	UINT16	light_pen_addr;		/* 0x10/0x11 */
	UINT16	vert_retr_start;	/* 0x10/0x07 */
	UINT8	vert_retr_end;		/* 0x11 */
	UINT8	protect;			/* 0x11 */
	UINT8	bandwidth;			/* 0x11 */
	UINT16	vert_disp_end;		/* 0x12/0x07 */
	UINT8	offset;				/* 0x13 */
	UINT8	underline_loc;		/* 0x14 */
	UINT16	vert_blank_start;	/* 0x15/0x07/0x09 */
	UINT8	vert_blank_end;		/* 0x16 */
	UINT8	mode_control;		/* 0x17 */
	UINT16	line_compare;		/* 0x18/0x07/0x09 */

	/* other internal state */
	UINT64	clock;
	UINT8	register_address_latch;
	UINT8	hpixels_per_column;
	UINT8	cursor_state;	/* 0 = off, 1 = on */
	UINT8	cursor_blink_count;

	/* timers */
	emu_timer *de_changed_timer;
	emu_timer *hsync_on_timer;
	emu_timer *hsync_off_timer;
	emu_timer *vsync_on_timer;
	emu_timer *vsync_off_timer;
	emu_timer *vblank_on_timer;
	emu_timer *vblank_off_timer;
	emu_timer *light_pen_latch_timer;

	/* computed values - do NOT state save these! */
	UINT16	horiz_pix_total;
	UINT16	vert_pix_total;
	UINT16	max_visible_x;
	UINT16	max_visible_y;
	UINT16	hsync_on_pos;
	UINT16	hsync_off_pos;
	UINT16	vsync_on_pos;
	UINT16	vsync_off_pos;
	UINT16  current_disp_addr;	/* the display address currently drawn */
	UINT8	light_pen_latched;
	int		has_valid_parameters;
};

//static DEVICE_GET_INFO( crtc_vga );

static STATE_POSTLOAD( crtc_ega_state_save_postload );
static void recompute_parameters(crtc_ega_t *crtc_ega, int postload);
static void update_de_changed_timer(crtc_ega_t *crtc_ega);
static void update_hsync_changed_timers(crtc_ega_t *crtc_ega);
static void update_vsync_changed_timers(crtc_ega_t *crtc_ega);
static void update_vblank_changed_timers(crtc_ega_t *crtc_ega);


/* makes sure that the passed in device is the right type */
INLINE crtc_ega_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == CRTC_EGA) ||
		   /*(device->type() == DEVICE_GET_INFO_NAME(crtc_vga))*/0 );

	return (crtc_ega_t *)downcast<legacy_device_base *>(device)->token();
}


static STATE_POSTLOAD( crtc_ega_state_save_postload )
{
	recompute_parameters((crtc_ega_t*)param, TRUE);
}


WRITE8_DEVICE_HANDLER( crtc_ega_address_w )
{
	crtc_ega_t *crtc_ega = get_safe_token(device);

	crtc_ega->register_address_latch = data & 0x1f;
}


READ8_DEVICE_HANDLER( crtc_ega_register_r )
{
	crtc_ega_t *crtc_ega = get_safe_token(device);
	UINT8 ret = 0;

	switch (crtc_ega->register_address_latch)
	{
		case 0x0c:  ret = (crtc_ega->disp_start_addr >> 8) & 0xff; break;
		case 0x0d:  ret = (crtc_ega->disp_start_addr >> 0) & 0xff; break;
		case 0x0e:  ret = (crtc_ega->cursor_addr    >> 8) & 0xff; break;
		case 0x0f:  ret = (crtc_ega->cursor_addr    >> 0) & 0xff; break;
		case 0x10:  ret = (crtc_ega->light_pen_addr >> 8) & 0xff; crtc_ega->light_pen_latched = FALSE; break;
		case 0x11:  ret = (crtc_ega->light_pen_addr >> 0) & 0xff; crtc_ega->light_pen_latched = FALSE; break;

		/* all other registers are write only and return 0 */
		default: break;
	}

	return ret;
}


WRITE8_DEVICE_HANDLER( crtc_ega_register_w )
{
	crtc_ega_t *crtc_ega = get_safe_token(device);

	if (LOG)  logerror("CRTC_EGA PC %04x: reg 0x%02x = 0x%02x\n", cpu_get_pc(device->machine().firstcpu), crtc_ega->register_address_latch, data);

	switch (crtc_ega->register_address_latch)
	{
		case 0x00:  crtc_ega->horiz_char_total  =   data & 0xff; break;
		case 0x01:  crtc_ega->horiz_disp        =   data & 0xff; break;
		case 0x02:  crtc_ega->horiz_blank_start =   data & 0xff; break;
		case 0x03:  crtc_ega->horiz_blank_end   =  ((data & 0x1f) << 0) | (crtc_ega->horiz_blank_end & 0x20);
					crtc_ega->de_skew           =  ((data & 0x60) >> 5);
					crtc_ega->ena_vert_access   =  data & 0x80;
					break;
		case 0x04:  crtc_ega->horiz_retr_start  =   data & 0xff; break;
		case 0x05:  crtc_ega->horiz_retr_end    =   data & 0x1f;
					crtc_ega->horiz_retr_skew   = ((data & 0x60) >> 5);
					crtc_ega->horiz_blank_end   = ((data & 0x80) >> 2) | (crtc_ega->horiz_blank_end & 0x1f);
					break;
		case 0x06:  crtc_ega->vert_total        = ((data & 0xff) << 0) | (crtc_ega->vert_total & 0x0300); break;
		case 0x07:  crtc_ega->vert_total        = ((data & 0x01) << 8) | (crtc_ega->vert_total & 0x02ff);
					crtc_ega->vert_disp_end     = ((data & 0x02) << 7) | (crtc_ega->vert_disp_end & 0x02ff);
					crtc_ega->vert_retr_start   = ((data & 0x04) << 6) | (crtc_ega->vert_retr_start & 0x02ff);
					crtc_ega->vert_blank_start  = ((data & 0x08) << 5) | (crtc_ega->vert_blank_start & 0x02ff);
					crtc_ega->line_compare      = ((data & 0x10) << 4) | (crtc_ega->line_compare & 0x02ff);
					crtc_ega->vert_total        = ((data & 0x20) << 4) | (crtc_ega->vert_total & 0x01ff);
					crtc_ega->vert_disp_end     = ((data & 0x40) << 3) | (crtc_ega->vert_disp_end & 0x1ff);
					crtc_ega->vert_retr_start   = ((data & 0x80) << 2) | (crtc_ega->vert_retr_start & 0x01ff);
					break;
		case 0x08:  crtc_ega->preset_row_scan   =   data & 0x1f;
					crtc_ega->byte_panning      = ((data & 0x60) >> 5);
					break;
		case 0x09:  crtc_ega->max_ras_addr      =   data & 0x1f;
					crtc_ega->vert_blank_start  = ((data & 0x20) << 4) | (crtc_ega->vert_blank_start & 0x01ff);
					crtc_ega->line_compare      = ((data & 0x40) << 3) | (crtc_ega->line_compare & 0x01ff);
					crtc_ega->scan_doubling     =   data & 0x80;
					break;
		case 0x0a:  crtc_ega->cursor_start_ras  =   data & 0x1f;
					crtc_ega->cursor_disable    =   data & 0x20;
					break;
		case 0x0b:  crtc_ega->cursor_end_ras    =   data & 0x1f;
					crtc_ega->cursor_skew       = ((data & 0x60) >> 5);
					break;
		case 0x0c:  crtc_ega->disp_start_addr   = ((data & 0xff) << 8) | (crtc_ega->disp_start_addr & 0x00ff); break;
		case 0x0d:  crtc_ega->disp_start_addr   = ((data & 0xff) << 0) | (crtc_ega->disp_start_addr & 0xff00); break;
		case 0x0e:  crtc_ega->cursor_addr       = ((data & 0xff) << 8) | (crtc_ega->cursor_addr & 0x00ff); break;
		case 0x0f:  crtc_ega->cursor_addr       = ((data & 0xff) << 0) | (crtc_ega->cursor_addr & 0xff00); break;
		case 0x10:	crtc_ega->vert_retr_start   = ((data & 0xff) << 0) | (crtc_ega->vert_retr_start & 0x03ff); break;
		case 0x11:	crtc_ega->vert_retr_end     =   data & 0x0f;
					crtc_ega->bandwidth         =   data & 0x40;
					crtc_ega->protect           =   data & 0x80;
					break;
		case 0x12:	crtc_ega->vert_disp_end     = ((data & 0xff) << 0) | (crtc_ega->vert_disp_end & 0x0300); break;
		case 0x13:	crtc_ega->offset            =  data & 0xff; break;
		case 0x14:	crtc_ega->underline_loc     =  data & 0x7f; break;
		case 0x15:	crtc_ega->vert_blank_start  = ((data & 0xff) << 0) | (crtc_ega->vert_blank_start & 0x0300); break;
		case 0x16:	crtc_ega->vert_blank_end    =   data & 0x7f; break;
		case 0x17:	crtc_ega->mode_control      =   data & 0xff; break;
		case 0x18:	crtc_ega->line_compare      = ((data & 0xff) << 0) | (crtc_ega->line_compare & 0x0300); break;
		default:	break;
	}

//  /* display message if the Mode Control register is not zero */
//  if ((crtc_ega->register_address_latch == 0x08) && (crtc_ega->mode_control != 0))
//      popmessage("Mode Control %02X is not supported!!!", crtc_ega->mode_control);

	recompute_parameters(crtc_ega, FALSE);
}


static void recompute_parameters(crtc_ega_t *crtc_ega, int postload)
{
	if (crtc_ega->intf != NULL)
	{
		UINT16 hsync_on_pos, hsync_off_pos, vsync_on_pos, vsync_off_pos;

		/* compute the screen sizes */
		UINT16 horiz_pix_total = (crtc_ega->horiz_char_total + 5) * crtc_ega->hpixels_per_column;
		UINT16 vert_pix_total = crtc_ega->vert_total + 1;

		/* determine the visible area, avoid division by 0 */
		UINT16 max_visible_x = ( crtc_ega->horiz_disp + 1 ) * crtc_ega->hpixels_per_column - 1;
		UINT16 max_visible_y = crtc_ega->vert_disp_end;

		/* determine the syncing positions */
		int horiz_sync_char_width = ( crtc_ega->horiz_retr_end + 1 ) - ( crtc_ega->horiz_retr_start & 0x1f );
		int vert_sync_pix_width = crtc_ega->vert_retr_end - ( crtc_ega->vert_retr_start & 0x0f );

		if (horiz_sync_char_width <= 0)
			horiz_sync_char_width += 0x10;

		if (vert_sync_pix_width <= 0)
			vert_sync_pix_width += 0x10;

		hsync_on_pos = crtc_ega->horiz_retr_start * crtc_ega->hpixels_per_column;
		hsync_off_pos = hsync_on_pos + (horiz_sync_char_width * crtc_ega->hpixels_per_column);
		vsync_on_pos = crtc_ega->vert_retr_start;		/* + 1 ?? */
		vsync_off_pos = vsync_on_pos + vert_sync_pix_width;

		/* the Commodore PET computers program a horizontal synch pulse that extends
          past the scanline width.  I assume that the real device will clamp it */
		if (hsync_off_pos > horiz_pix_total)
			hsync_off_pos = horiz_pix_total;

		if (vsync_off_pos > vert_pix_total)
			vsync_off_pos = vert_pix_total;

		/* update only if screen parameters changed, unless we are coming here after loading the saved state */
		if (postload ||
		    (horiz_pix_total != crtc_ega->horiz_pix_total) || (vert_pix_total != crtc_ega->vert_pix_total) ||
			(max_visible_x != crtc_ega->max_visible_x) || (max_visible_y != crtc_ega->max_visible_y) ||
			(hsync_on_pos != crtc_ega->hsync_on_pos) || (vsync_on_pos != crtc_ega->vsync_on_pos) ||
			(hsync_off_pos != crtc_ega->hsync_off_pos) || (vsync_off_pos != crtc_ega->vsync_off_pos))
		{
			/* update the screen if we have valid data */
			if ((horiz_pix_total > 0) && (max_visible_x < horiz_pix_total) &&
				(vert_pix_total > 0) && (max_visible_y < vert_pix_total) &&
				(hsync_on_pos <= horiz_pix_total) && (vsync_on_pos <= vert_pix_total) &&
				(hsync_on_pos != hsync_off_pos))
			{
				rectangle visarea;

				attoseconds_t refresh = HZ_TO_ATTOSECONDS(crtc_ega->clock) * (crtc_ega->horiz_char_total + 1) * vert_pix_total;

				visarea.min_x = 0;
				visarea.min_y = 0;
				visarea.max_x = max_visible_x;
				visarea.max_y = max_visible_y;

				if (LOG) logerror("CRTC_EGA config screen: HTOTAL: 0x%x  VTOTAL: 0x%x  MAX_X: 0x%x  MAX_Y: 0x%x  HSYNC: 0x%x-0x%x  VSYNC: 0x%x-0x%x  Freq: %ffps\n",
								  horiz_pix_total, vert_pix_total, max_visible_x, max_visible_y, hsync_on_pos, hsync_off_pos - 1, vsync_on_pos, vsync_off_pos - 1, 1 / ATTOSECONDS_TO_DOUBLE(refresh));

				crtc_ega->screen->configure(horiz_pix_total, vert_pix_total, visarea, refresh);

				crtc_ega->has_valid_parameters = TRUE;
			}
			else
				crtc_ega->has_valid_parameters = FALSE;

			crtc_ega->horiz_pix_total = horiz_pix_total;
			crtc_ega->vert_pix_total = vert_pix_total;
			crtc_ega->max_visible_x = max_visible_x;
			crtc_ega->max_visible_y = max_visible_y;
			crtc_ega->hsync_on_pos = hsync_on_pos;
			crtc_ega->hsync_off_pos = hsync_off_pos;
			crtc_ega->vsync_on_pos = vsync_on_pos;
			crtc_ega->vsync_off_pos = vsync_off_pos;

			update_de_changed_timer(crtc_ega);
			update_hsync_changed_timers(crtc_ega);
			update_vsync_changed_timers(crtc_ega);
			update_vblank_changed_timers(crtc_ega);
		}
	}
}


INLINE int is_display_enabled(crtc_ega_t *crtc_ega)
{
	return !crtc_ega->screen->vblank() && !crtc_ega->screen->hblank();
}


static void update_de_changed_timer(crtc_ega_t *crtc_ega)
{
	if (crtc_ega->has_valid_parameters && (crtc_ega->de_changed_timer != NULL))
	{
		INT16 next_y;
		UINT16 next_x;
		attotime duration;

		/* we are in a display region, get the location of the next blanking start */
		if (is_display_enabled(crtc_ega))
		{
			/* normally, it's at end the current raster line */
			next_y = crtc_ega->screen->vpos();
			next_x = crtc_ega->max_visible_x + 1;

			/* but if visible width = horiz_pix_total, then we need
               to go to the beginning of VBLANK */
			if (next_x == crtc_ega->horiz_pix_total)
			{
				next_y = crtc_ega->max_visible_y + 1;
				next_x = 0;

				/* abnormal case, no vertical blanking, either */
				if (next_y == crtc_ega->vert_pix_total)
					next_y = -1;
			}
		}

		/* we are in a blanking region, get the location of the next display start */
		else
		{
			next_x = 0;
			next_y = (crtc_ega->screen->vpos() + 1) % crtc_ega->vert_pix_total;

			/* if we would now fall in the vertical blanking, we need
               to go to the top of the screen */
			if (next_y > crtc_ega->max_visible_y)
				next_y = 0;
		}

		if (next_y != -1)
			duration = crtc_ega->screen->time_until_pos(next_y, next_x);
		else
			duration = attotime::never;

		crtc_ega->de_changed_timer->adjust(duration);
	}
}


static void update_hsync_changed_timers(crtc_ega_t *crtc_ega)
{
	if (crtc_ega->has_valid_parameters && (crtc_ega->hsync_on_timer != NULL))
	{
		UINT16 next_y;

		/* we are before the HSYNC position, we trigger on the current line */
		if (crtc_ega->screen->hpos() < crtc_ega->hsync_on_pos)
			next_y = crtc_ega->screen->vpos();

		/* trigger on the next line */
		else
			next_y = (crtc_ega->screen->vpos() + 1) % crtc_ega->vert_pix_total;

		/* if the next line is not in the visible region, go to the beginning of the screen */
		if (next_y > crtc_ega->max_visible_y)
			next_y = 0;

		crtc_ega->hsync_on_timer->adjust(crtc_ega->screen->time_until_pos(next_y, crtc_ega->hsync_on_pos) );
		crtc_ega->hsync_off_timer->adjust(crtc_ega->screen->time_until_pos(next_y, crtc_ega->hsync_off_pos));
	}
}


static void update_vsync_changed_timers(crtc_ega_t *crtc_ega)
{
	if (crtc_ega->has_valid_parameters && (crtc_ega->vsync_on_timer != NULL))
	{
		crtc_ega->vsync_on_timer->adjust(crtc_ega->screen->time_until_pos(crtc_ega->vsync_on_pos,  0));
		crtc_ega->vsync_off_timer->adjust(crtc_ega->screen->time_until_pos(crtc_ega->vsync_off_pos, 0));
	}
}


static void update_vblank_changed_timers(crtc_ega_t *crtc_ega)
{
	if (crtc_ega->has_valid_parameters && (crtc_ega->vblank_on_timer != NULL))
	{
		crtc_ega->vblank_on_timer->adjust(crtc_ega->screen->time_until_pos(crtc_ega->vert_disp_end,  crtc_ega->hsync_on_pos));
		crtc_ega->vblank_off_timer->adjust(crtc_ega->screen->time_until_pos(0, crtc_ega->hsync_off_pos));
	}
}


static TIMER_CALLBACK( de_changed_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* call the callback function -- we know it exists */
	crtc_ega->intf->on_de_changed(device, is_display_enabled(crtc_ega));

	update_de_changed_timer(crtc_ega);
}


static TIMER_CALLBACK( vsync_on_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* call the callback function -- we know it exists */
	crtc_ega->intf->on_vsync_changed(device, TRUE);
}


static TIMER_CALLBACK( vsync_off_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* call the callback function -- we know it exists */
	crtc_ega->intf->on_vsync_changed(device, FALSE);

	update_vsync_changed_timers(crtc_ega);
}


static TIMER_CALLBACK( hsync_on_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* call the callback function -- we know it exists */
	crtc_ega->intf->on_hsync_changed(device, TRUE);
}


static TIMER_CALLBACK( hsync_off_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* call the callback function -- we know it exists */
	crtc_ega->intf->on_hsync_changed(device, FALSE);

	update_hsync_changed_timers(crtc_ega);
}


static TIMER_CALLBACK( vblank_on_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* call the callback function -- we know it exists */
	crtc_ega->intf->on_vblank_changed(device, TRUE);

	update_vblank_changed_timers(crtc_ega);
}


static TIMER_CALLBACK( vblank_off_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* call the callback function -- we know it exists */
	crtc_ega->intf->on_vblank_changed(device, FALSE);

	update_vblank_changed_timers(crtc_ega);
}



UINT16 crtc_ega_get_ma(device_t *device)
{
	UINT16 ret;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	if (crtc_ega->has_valid_parameters)
	{
		/* clamp Y/X to the visible region */
		int y = crtc_ega->screen->vpos();
		int x = crtc_ega->screen->hpos();

		/* since the MA counter stops in the blanking regions, if we are in a
           VBLANK, both X and Y are at their max */
		if ((y > crtc_ega->max_visible_y) || (x > crtc_ega->max_visible_x))
			x = crtc_ega->max_visible_x;

		if (y > crtc_ega->max_visible_y)
			y = crtc_ega->max_visible_y;

		ret = (crtc_ega->disp_start_addr +
			  (y / (crtc_ega->max_ras_addr + 1)) * ( crtc_ega->horiz_disp + 1 ) +
			  (x / crtc_ega->hpixels_per_column)) & 0x3fff;
	}
	else
		ret = 0;

	return ret;
}


UINT8 crtc_ega_get_ra(device_t *device)
{
	UINT8 ret;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	if (crtc_ega->has_valid_parameters)
	{
		/* get the current vertical raster position and clamp it to the visible region */
		int y = crtc_ega->screen->vpos();

		if (y > crtc_ega->max_visible_y)
			y = crtc_ega->max_visible_y;

		ret = y % (crtc_ega->max_ras_addr + 1);
	}
	else
		ret = 0;

	return ret;
}


static TIMER_CALLBACK( light_pen_latch_timer_cb )
{
	device_t *device = (device_t *)ptr;
	crtc_ega_t *crtc_ega = get_safe_token(device);

	crtc_ega->light_pen_addr = crtc_ega_get_ma(device);
	crtc_ega->light_pen_latched = TRUE;
}


void crtc_ega_assert_light_pen_input(device_t *device)
{
	int y, x;
	int char_x;

	crtc_ega_t *crtc_ega = get_safe_token(device);

	if (crtc_ega->has_valid_parameters)
	{
		/* get the current pixel coordinates */
		y = crtc_ega->screen->vpos();
		x = crtc_ega->screen->hpos();

		/* compute the pixel coordinate of the NEXT character -- this is when the light pen latches */
		char_x = x / crtc_ega->hpixels_per_column;
		x = (char_x + 1) * crtc_ega->hpixels_per_column;

		/* adjust if we are passed the boundaries of the screen */
		if (x == crtc_ega->horiz_pix_total)
		{
			y = y + 1;
			x = 0;

			if (y == crtc_ega->vert_pix_total)
				y = 0;
		}

		/* set the timer that will latch the display address into the light pen registers */
		crtc_ega->light_pen_latch_timer->adjust(crtc_ega->screen->time_until_pos(y, x));
	}
}


void crtc_ega_set_clock(device_t *device, int clock)
{
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* validate arguments */
	assert(clock > 0);

	if (clock != crtc_ega->clock)
	{
		crtc_ega->clock = clock;
		recompute_parameters(crtc_ega, FALSE);
	}
}


void crtc_ega_set_hpixels_per_column(device_t *device, int hpixels_per_column)
{
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* validate arguments */
	assert(hpixels_per_column > 0);

	if (hpixels_per_column != crtc_ega->hpixels_per_column)
	{
		crtc_ega->hpixels_per_column = hpixels_per_column;
		recompute_parameters(crtc_ega, FALSE);
	}
}


static void update_cursor_state(crtc_ega_t *crtc_ega)
{
	/* save and increment cursor counter */
	UINT8 last_cursor_blink_count = crtc_ega->cursor_blink_count;
	crtc_ega->cursor_blink_count = crtc_ega->cursor_blink_count + 1;

	/* switch on cursor blinking mode */
	switch (crtc_ega->cursor_start_ras & 0x60)
	{
		/* always on */
		case 0x00: crtc_ega->cursor_state = TRUE; break;

		/* always off */
		default:
		case 0x20: crtc_ega->cursor_state = FALSE; break;

		/* fast blink */
		case 0x40:
			if ((last_cursor_blink_count & 0x10) != (crtc_ega->cursor_blink_count & 0x10))
				crtc_ega->cursor_state = !crtc_ega->cursor_state;
			break;

		/* slow blink */
		case 0x60:
			if ((last_cursor_blink_count & 0x20) != (crtc_ega->cursor_blink_count & 0x20))
				crtc_ega->cursor_state = !crtc_ega->cursor_state;
			break;
	}
}


void crtc_ega_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	crtc_ega_t *crtc_ega = get_safe_token(device);
	assert(bitmap != NULL);
	assert(cliprect != NULL);

	if (crtc_ega->has_valid_parameters)
	{
		UINT16 y;

		void *param = NULL;

		assert(crtc_ega->intf != NULL);
		assert(crtc_ega->intf->update_row != NULL);

		/* call the set up function if any */
		if (crtc_ega->intf->begin_update != NULL)
			param = crtc_ega->intf->begin_update(device, bitmap, cliprect);

		if (cliprect->min_y == 0)
		{
			/* read the start address at the beginning of the frame */
			crtc_ega->current_disp_addr = crtc_ega->disp_start_addr;

			/* also update the cursor state now */
			update_cursor_state(crtc_ega);
		}

		/* for each row in the visible region */
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			/* compute the current raster line */
			UINT8 ra = y % (crtc_ega->max_ras_addr + 1);

			/* check if the cursor is visible and is on this scanline */
			int cursor_visible = crtc_ega->cursor_state &&
								(ra >= (crtc_ega->cursor_start_ras & 0x1f)) &&
								(ra <= crtc_ega->cursor_end_ras) &&
								(crtc_ega->cursor_addr >= crtc_ega->current_disp_addr) &&
								(crtc_ega->cursor_addr < (crtc_ega->current_disp_addr + ( crtc_ega->horiz_disp + 1 )));

			/* compute the cursor X position, or -1 if not visible */
			INT8 cursor_x = cursor_visible ? (crtc_ega->cursor_addr - crtc_ega->current_disp_addr) : -1;

			/* call the external system to draw it */
			crtc_ega->intf->update_row(device, bitmap, cliprect, crtc_ega->current_disp_addr, ra, y, crtc_ega->horiz_disp + 1, cursor_x, param);

			/* update MA if the last raster address */
			if (ra == crtc_ega->max_ras_addr)
				crtc_ega->current_disp_addr = (crtc_ega->current_disp_addr + crtc_ega->horiz_disp + 1) & 0xffff;
		}

		/* call the tear down function if any */
		if (crtc_ega->intf->end_update != NULL)
			crtc_ega->intf->end_update(device, bitmap, cliprect, param);
	}
	else
		popmessage("Invalid crtc_ega screen parameters - display disabled!!!");
}


/* device interface */
static void common_start(device_t *device, int device_type)
{
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag() != NULL);
	assert(device->clock() > 0);

	crtc_ega->intf = (const crtc_ega_interface*)device->baseconfig().static_config();
	crtc_ega->device_type = device_type;

	if (crtc_ega->intf != NULL)
	{
		assert(crtc_ega->intf->hpixels_per_column > 0);

		/* copy the initial parameters */
		crtc_ega->clock = device->clock();
		crtc_ega->hpixels_per_column = crtc_ega->intf->hpixels_per_column;

		/* get the screen device */
		crtc_ega->screen = downcast<screen_device *>(device->machine().device(crtc_ega->intf->screen_tag));
		assert(crtc_ega->screen != NULL);

		/* create the timers */
		if (crtc_ega->intf->on_de_changed != NULL)
			crtc_ega->de_changed_timer = device->machine().scheduler().timer_alloc(FUNC(de_changed_timer_cb), (void *)device);

		if (crtc_ega->intf->on_hsync_changed != NULL)
		{
			crtc_ega->hsync_on_timer = device->machine().scheduler().timer_alloc(FUNC(hsync_on_timer_cb), (void *)device);
			crtc_ega->hsync_off_timer = device->machine().scheduler().timer_alloc(FUNC(hsync_off_timer_cb), (void *)device);
		}

		if (crtc_ega->intf->on_vsync_changed != NULL)
		{
			crtc_ega->vsync_on_timer = device->machine().scheduler().timer_alloc(FUNC(vsync_on_timer_cb), (void *)device);
			crtc_ega->vsync_off_timer = device->machine().scheduler().timer_alloc(FUNC(vsync_off_timer_cb), (void *)device);
		}

		if (crtc_ega->intf->on_vblank_changed != NULL)
		{
			crtc_ega->vblank_on_timer = device->machine().scheduler().timer_alloc(FUNC(vblank_on_timer_cb), (void *)device);
			crtc_ega->vblank_off_timer = device->machine().scheduler().timer_alloc(FUNC(vblank_off_timer_cb), (void *)device);
		}
	}

	crtc_ega->light_pen_latch_timer = device->machine().scheduler().timer_alloc(FUNC(light_pen_latch_timer_cb), (void *)device);

	/* register for state saving */

	device->machine().state().register_postload(crtc_ega_state_save_postload, crtc_ega);

	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->clock);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->hpixels_per_column);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->register_address_latch);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->horiz_char_total);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->horiz_disp);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->horiz_blank_start);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->mode_control);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->cursor_start_ras);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->cursor_end_ras);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->disp_start_addr);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->cursor_addr);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->light_pen_addr);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->light_pen_latched);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->cursor_state);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->cursor_blink_count);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->horiz_blank_end);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->ena_vert_access);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->de_skew);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->horiz_retr_start);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->horiz_retr_end);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->horiz_retr_skew);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->vert_total);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->preset_row_scan);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->byte_panning);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->max_ras_addr);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->scan_doubling);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->cursor_disable);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->cursor_skew);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->vert_retr_start);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->vert_retr_end);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->protect);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->bandwidth);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->vert_disp_end);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->offset);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->underline_loc);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->vert_blank_start);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->vert_blank_end);
	state_save_register_item(device->machine(), device->tag(), NULL, 0, crtc_ega->line_compare);
}


static DEVICE_START( crtc_ega )
{
	common_start(device, TYPE_CRTC_EGA);
}


static DEVICE_RESET( crtc_ega )
{
	crtc_ega_t *crtc_ega = get_safe_token(device);

	/* internal registers other than status remain unchanged, all outputs go low */
	if (crtc_ega->intf != NULL)
	{
		if (crtc_ega->intf->on_de_changed != NULL)
			crtc_ega->intf->on_de_changed(device, FALSE);

		if (crtc_ega->intf->on_hsync_changed != NULL)
			crtc_ega->intf->on_hsync_changed(device, FALSE);

		if (crtc_ega->intf->on_vsync_changed != NULL)
			crtc_ega->intf->on_vsync_changed(device, FALSE);
	}

	crtc_ega->light_pen_latched = FALSE;
}


DEVICE_GET_INFO( crtc_ega )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(crtc_ega_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(crtc_ega);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(crtc_ega);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "IBM EGA CRTC");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "EGA CRTC");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.00");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); break;
	}
}

#if 0
DEVICE_GET_INFO( crtc_vga )
{
	switch (state)
	{
		default:										DEVICE_GET_INFO_CALL( crtc_ega );			break;
	}
}
#endif

DEFINE_LEGACY_DEVICE(CRTC_EGA, crtc_ega);
