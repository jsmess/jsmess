/**********************************************************************

    NEC uPD3301 Programmable CRT Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - attributes
    - N interrupt
    - light pen
    - reset counters
    - proper DMA timing (now the whole screen is transferred at the end of the frame,
        accurate timing requires CCLK timer which kills performance)

*/

#include "emu.h"
#include "upd3301.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define UPD3301_COMMAND_MASK					0xe0
#define UPD3301_COMMAND_RESET					0x00
#define UPD3301_COMMAND_START_DISPLAY			0x20
#define UPD3301_COMMAND_SET_INTERRUPT_MASK		0x40
#define UPD3301_COMMAND_READ_LIGHT_PEN			0x60	/* not supported */
#define UPD3301_COMMAND_LOAD_CURSOR_POSITION	0x80
#define UPD3301_COMMAND_RESET_INTERRUPT			0xa0
#define UPD3301_COMMAND_RESET_COUNTERS			0xc0	/* not supported */

#define UPD3301_STATUS_VE						0x10
#define UPD3301_STATUS_U						0x08	/* not supported */
#define UPD3301_STATUS_N						0x04	/* not supported */
#define UPD3301_STATUS_E						0x02
#define UPD3301_STATUS_LP						0x01	/* not supported */

enum
{
	MODE_NONE,
	MODE_RESET,
	MODE_READ_LIGHT_PEN,
	MODE_LOAD_CURSOR_POSITION,
	MODE_RESET_COUNTERS
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd3301_t upd3301_t;
struct _upd3301_t
{
	/* callbacks */
	devcb_resolved_write_line		out_int_func;
	devcb_resolved_write_line		out_drq_func;
	devcb_resolved_write_line		out_hrtc_func;
	devcb_resolved_write_line		out_vrtc_func;
	upd3301_display_pixels_func		display_func;

	/* screen drawing */
	screen_device *screen;	/* screen */
	bitmap_t *bitmap;				/* bitmap */
	int y;							/* current scanline */
	int hrtc;						/* horizontal retrace */
	int vrtc;						/* vertical retrace */

	/* live state */
	int mode;						/* command mode */
	UINT8 status;					/* status register */
	int param_count;				/* parameter count */

	/* FIFOs */
	UINT8 data_fifo[80][2];			/* row data FIFO */
	UINT8 attr_fifo[40][2];			/* attribute FIFO */
	int data_fifo_pos;				/* row data FIFO position */
	int attr_fifo_pos;				/* attribute FIFO position */
	int input_fifo;					/* which FIFO is in input mode */

	/* interrupts */
	int mn;							/* disable special character interrupt */
	int me;							/* disable end of screen interrupt */
	int dma_mode;					/* DMA mode */

	/* screen geometry */
	int width;						/* character width */
	int h;							/* characters per line */
	int b;							/* cursor blink time */
	int l;							/* lines per screen */
	int s;							/* display every other line */
	int c;							/* cursor mode */
	int r;							/* lines per character */
	int v;							/* vertical blanking height */
	int z;							/* horizontal blanking width */

	/* attributes */
	int at1;						/*  */
	int at0;						/*  */
	int sc;							/*  */
	int attr;						/* attributes per row */
	int attr_blink;					/* attribute blink */
	int attr_frame;					/* attribute blink frame counter */

	/* cursor */
	int cm;							/* cursor visible */
	int cx;							/* cursor column */
	int cy;							/* cursor row */
	int cursor_blink;				/* cursor blink */
	int cursor_frame;				/* cursor blink frame counter */

	/* timers */
	emu_timer *vrtc_timer;			/* vertical sync timer */
	emu_timer *hrtc_timer;			/* horizontal sync timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd3301_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == UPD3301));
	return (upd3301_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const upd3301_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == UPD3301));
	return (const upd3301_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    set_interrupt - set interrupt line state
-------------------------------------------------*/

static void set_interrupt(device_t *device, int state)
{
	upd3301_t *upd3301 = get_safe_token(device);

	if (LOG) logerror("UPD3301 '%s' Interrupt: %u\n", device->tag(), state);

	devcb_call_write_line(&upd3301->out_int_func, state);

	if (!state)
	{
		upd3301->status &= ~(UPD3301_STATUS_N | UPD3301_STATUS_E);
	}
}

/*-------------------------------------------------
    set_drq - set data request line state
-------------------------------------------------*/

static void set_drq(device_t *device, int state)
{
	upd3301_t *upd3301 = get_safe_token(device);

	if (LOG) logerror("UPD3301 '%s' DRQ: %u\n", device->tag(), state);

	devcb_call_write_line(&upd3301->out_drq_func, state);
}

/*-------------------------------------------------
    set_display - set display state
-------------------------------------------------*/

static void set_display(device_t *device, int state)
{
	upd3301_t *upd3301 = get_safe_token(device);

	if (state)
	{
		upd3301->status |= UPD3301_STATUS_VE;
	}
	else
	{
		upd3301->status &= ~UPD3301_STATUS_VE;
	}
}

/*-------------------------------------------------
    reset_counters - reset screen counters
-------------------------------------------------*/

static void reset_counters(device_t *device)
{
	set_interrupt(device, 0);
	set_drq(device, 0);
}

/*-------------------------------------------------
    update_hrtc_timer - update horizontal retrace
    timer
-------------------------------------------------*/

static void update_hrtc_timer(upd3301_t *upd3301, int state)
{
	int y = upd3301->screen->vpos();

	int next_x = state ? upd3301->h : 0;
	int next_y = state ? y : ((y + 1) % ((upd3301->l + upd3301->v) * upd3301->width));

	attotime duration = upd3301->screen->time_until_pos(next_y, next_x);

	upd3301->hrtc_timer->adjust(duration, !state);
}

/*-------------------------------------------------
    update_vrtc_timer - update vertical retrace
    timer
-------------------------------------------------*/

static void update_vrtc_timer(upd3301_t *upd3301, int state)
{
	int next_y = state ? (upd3301->l * upd3301->r) : 0;

	attotime duration = upd3301->screen->time_until_pos(next_y, 0);

	upd3301->vrtc_timer->adjust(duration, !state);
}

/*-------------------------------------------------
    recompute_parameters - recompute screen
    geometry parameters
-------------------------------------------------*/

static void recompute_parameters(device_t *device)
{
	upd3301_t *upd3301 = get_safe_token(device);

	int horiz_pix_total = (upd3301->h + upd3301->z) * upd3301->width;
	int vert_pix_total = (upd3301->l + upd3301->v) * upd3301->r;

	attoseconds_t refresh = HZ_TO_ATTOSECONDS(device->clock()) * horiz_pix_total * vert_pix_total;

	rectangle visarea;

	visarea.min_x = 0;
	visarea.min_y = 0;
	visarea.max_x = (upd3301->h * upd3301->width) - 1;
	visarea.max_y = (upd3301->l * upd3301->r) - 1;

	if (LOG)
	{
		if (LOG) logerror("UPD3301 '%s' Screen: %u x %u @ %f Hz\n", device->tag(), horiz_pix_total, vert_pix_total, 1 / ATTOSECONDS_TO_DOUBLE(refresh));
		if (LOG) logerror("UPD3301 '%s' Visible Area: (%u, %u) - (%u, %u)\n", device->tag(), visarea.min_x, visarea.min_y, visarea.max_x, visarea.max_y);
	}

	upd3301->screen->configure(horiz_pix_total, vert_pix_total, visarea, refresh);

	update_hrtc_timer(upd3301, 0);
	update_vrtc_timer(upd3301, 0);
}

/*-------------------------------------------------
    TIMER_CALLBACK( vrtc_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( vrtc_tick )
{
	device_t *device = (device_t *)ptr;
	upd3301_t *upd3301 = get_safe_token(device);

	if (LOG) logerror("UPD3301 '%s' VRTC: %u\n", device->tag(), param);

	devcb_call_write_line(&upd3301->out_vrtc_func, param);
	upd3301->vrtc = param;

	if (param && !upd3301->me)
	{
		upd3301->status |= UPD3301_STATUS_E;
		set_interrupt(device, 1);
	}

	update_vrtc_timer(upd3301, param);
}

/*-------------------------------------------------
    TIMER_CALLBACK( hrtc_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( hrtc_tick )
{
	device_t *device = (device_t *)ptr;
	upd3301_t *upd3301 = get_safe_token(device);

	if (LOG) logerror("UPD3301 '%s' HRTC: %u\n", device->tag(), param);

	devcb_call_write_line(&upd3301->out_hrtc_func, param);
	upd3301->hrtc = param;

	update_hrtc_timer(upd3301, param);
}

/*-------------------------------------------------
    upd3301_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( upd3301_r )
{
	upd3301_t *upd3301 = get_safe_token(device);
	UINT8 data = 0;

	switch (offset & 0x01)
	{
	case 0: /* data */
		break;

	case 1: /* status */
		data = upd3301->status;
		upd3301->status &= ~(UPD3301_STATUS_LP | UPD3301_STATUS_E |UPD3301_STATUS_N | UPD3301_STATUS_U);
		break;
	}

	return data;
}

/*-------------------------------------------------
    upd3301_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd3301_w )
{
	upd3301_t *upd3301 = get_safe_token(device);

	switch (offset & 0x01)
	{
	case 0: /* data */
		switch (upd3301->mode)
		{
		case MODE_RESET:
			switch (upd3301->param_count)
			{
			case 0:
				upd3301->dma_mode = BIT(data, 7);
				upd3301->h = (data & 0x7f) + 2;
				if (LOG) logerror("UPD3301 '%s' DMA Mode: %s\n", device->tag(), upd3301->dma_mode ? "character" : "burst");
				if (LOG) logerror("UPD3301 '%s' H: %u\n", device->tag(), upd3301->h);
				break;

			case 1:
				upd3301->b = ((data >> 6) + 1) * 16;
				upd3301->l = (data & 0x3f) + 1;
				if (LOG) logerror("UPD3301 '%s' B: %u\n", device->tag(), upd3301->b);
				if (LOG) logerror("UPD3301 '%s' L: %u\n", device->tag(), upd3301->l);
				break;

			case 2:
				upd3301->s = BIT(data, 7);
				upd3301->c = (data >> 4) & 0x03;
				upd3301->r = (data & 0x1f) + 1;
				if (LOG) logerror("UPD3301 '%s' S: %u\n", device->tag(), upd3301->s);
				if (LOG) logerror("UPD3301 '%s' C: %u\n", device->tag(), upd3301->c);
				if (LOG) logerror("UPD3301 '%s' R: %u\n", device->tag(), upd3301->r);
				break;

			case 3:
				upd3301->v = (data >> 5) + 1;
				upd3301->z = (data & 0x1f) + 2;
				if (LOG) logerror("UPD3301 '%s' V: %u\n", device->tag(), upd3301->v);
				if (LOG) logerror("UPD3301 '%s' Z: %u\n", device->tag(), upd3301->z);
				recompute_parameters(device);
				break;

			case 4:
				upd3301->at1 = BIT(data, 7);
				upd3301->at0 = BIT(data, 6);
				upd3301->sc = BIT(data, 5);
				upd3301->attr = (data & 0x1f) + 1;
				if (LOG) logerror("UPD3301 '%s' AT1: %u\n", device->tag(), upd3301->at1);
				if (LOG) logerror("UPD3301 '%s' AT0: %u\n", device->tag(), upd3301->at0);
				if (LOG) logerror("UPD3301 '%s' SC: %u\n", device->tag(), upd3301->sc);
				if (LOG) logerror("UPD3301 '%s' ATTR: %u\n", device->tag(), upd3301->attr);

				upd3301->mode = MODE_NONE;
				break;
			}

			upd3301->param_count++;
			break;

		case MODE_LOAD_CURSOR_POSITION:
			switch (upd3301->param_count)
			{
			case 0:
				upd3301->cx = data & 0x7f;
				if (LOG) logerror("UPD3301 '%s' CX: %u\n", device->tag(), upd3301->cx);
				break;

			case 1:
				upd3301->cy = data & 0x3f;
				if (LOG) logerror("UPD3301 '%s' CY: %u\n", device->tag(), upd3301->cy);

				upd3301->mode = MODE_NONE;
				break;
			}

			upd3301->param_count++;
			break;

		default:
			if (LOG) logerror("UPD3301 '%s' Invalid Parameter Byte %02x!\n", device->tag(), data);
		}
		break;

	case 1: /* command */
		upd3301->mode = MODE_NONE;
		upd3301->param_count = 0;

		switch (data & 0xe0)
		{
		case UPD3301_COMMAND_RESET:
			if (LOG) logerror("UPD3301 '%s' Reset\n", device->tag());
			upd3301->mode = MODE_RESET;
			set_display(device, 0);
			set_interrupt(device, 0);
			break;

		case UPD3301_COMMAND_START_DISPLAY:
			if (LOG) logerror("UPD3301 '%s' Start Display\n", device->tag());
			set_display(device, 1);
			reset_counters(device);
			break;

		case UPD3301_COMMAND_SET_INTERRUPT_MASK:
			if (LOG) logerror("UPD3301 '%s' Set Interrupt Mask\n", device->tag());
			upd3301->me = BIT(data, 0);
			upd3301->mn = BIT(data, 1);
			if (LOG) logerror("UPD3301 '%s' ME: %u\n", device->tag(), upd3301->me);
			if (LOG) logerror("UPD3301 '%s' MN: %u\n", device->tag(), upd3301->mn);
			break;

		case UPD3301_COMMAND_READ_LIGHT_PEN:
			if (LOG) logerror("UPD3301 '%s' Read Light Pen\n", device->tag());
			upd3301->mode = MODE_READ_LIGHT_PEN;
			break;

		case UPD3301_COMMAND_LOAD_CURSOR_POSITION:
			if (LOG) logerror("UPD3301 '%s' Load Cursor Position\n", device->tag());
			upd3301->mode = MODE_LOAD_CURSOR_POSITION;
			upd3301->cm = BIT(data, 0);
			if (LOG) logerror("UPD3301 '%s' CM: %u\n", device->tag(), upd3301->cm);
			break;

		case UPD3301_COMMAND_RESET_INTERRUPT:
			if (LOG) logerror("UPD3301 '%s' Reset Interrupt\n", device->tag());
			set_interrupt(device, 0);
			break;

		case UPD3301_COMMAND_RESET_COUNTERS:
			if (LOG) logerror("UPD3301 '%s' Reset Counters\n", device->tag());
			upd3301->mode = MODE_RESET_COUNTERS;
			reset_counters(device);
			break;
		}
		break;
	}
}

/*-------------------------------------------------
    upd3301_lpen_w - light pen write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd3301_lpen_w )
{
}

/*-------------------------------------------------
    upd3301_hrtc_r - horizontal retrace read
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( upd3301_hrtc_r )
{
	upd3301_t *upd3301 = get_safe_token(device);

	return upd3301->hrtc;
}

/*-------------------------------------------------
    upd3301_vrtc_r - vertical retrace read
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( upd3301_vrtc_r )
{
	upd3301_t *upd3301 = get_safe_token(device);

	return upd3301->vrtc;
}

/*-------------------------------------------------
    draw_row - draw character row
-------------------------------------------------*/

static void draw_row(device_t *device, bitmap_t *bitmap)
{
	upd3301_t *upd3301 = get_safe_token(device);
	int sx, lc;

	for (lc = 0; lc < upd3301->r; lc++)
	{
		for (sx = 0; sx < upd3301->h; sx++)
		{
			int y = upd3301->y + lc;
			UINT8 cc = upd3301->data_fifo[sx][!upd3301->input_fifo];
			int hlgt = 0;
			int rvv = 0;
			int vsp = 0;
			int sl0 = 0;
			int sl12 = 0;
			int csr = upd3301->cm && upd3301->cursor_blink && ((y / upd3301->r) == upd3301->cy) && (sx == upd3301->cx);
			int gpa = 0;

			upd3301->display_func(device, bitmap, y, sx, cc, lc, hlgt, rvv, vsp, sl0, sl12, csr, gpa);
		}
	}

	upd3301->y += upd3301->r;
}

/*-------------------------------------------------
    upd3301_dack_w - DMA acknowledge write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd3301_dack_w )
{
	upd3301_t *upd3301 = get_safe_token(device);

	if (upd3301->y >= (upd3301->l * upd3301->r))
	{
		return;
	}

	if (upd3301->data_fifo_pos < upd3301->h)
	{
		upd3301->data_fifo[upd3301->data_fifo_pos][upd3301->input_fifo] = data;
		upd3301->data_fifo_pos++;
	}
	else
	{
		upd3301->attr_fifo[upd3301->attr_fifo_pos][upd3301->input_fifo] = data;
		upd3301->attr_fifo_pos++;
	}

	if ((upd3301->data_fifo_pos == upd3301->h) && (upd3301->attr_fifo_pos == (upd3301->attr << 1)))
	{
		upd3301->input_fifo = !upd3301->input_fifo;
		upd3301->data_fifo_pos = 0;
		upd3301->attr_fifo_pos = 0;
		draw_row(device, upd3301->bitmap);

		if (upd3301->y == (upd3301->l * upd3301->r))
		{
			/* end DMA transfer */
			set_drq(device, 0);
		}
	}
}

/*-------------------------------------------------
    upd3301_update - screen update
-------------------------------------------------*/

void upd3301_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	upd3301_t *upd3301 = get_safe_token(device);

	if (upd3301->status & UPD3301_STATUS_VE)
	{
		upd3301->y = 0;
		upd3301->bitmap = bitmap;
		upd3301->data_fifo_pos = 0;
		upd3301->attr_fifo_pos = 0;

		upd3301->cursor_frame++;

		if (upd3301->cursor_frame == upd3301->b)
		{
			upd3301->cursor_frame = 0;
			upd3301->cursor_blink = !upd3301->cursor_blink;
		}

		upd3301->attr_frame++;

		if (upd3301->attr_frame == (upd3301->b << 1))
		{
			upd3301->attr_frame = 0;
			upd3301->attr_blink = !upd3301->attr_blink;
		}

		/* start DMA transfer */
		set_drq(device, 1);
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(device->machine()));
	}
}

/*-------------------------------------------------
    DEVICE_START( upd3301 )
-------------------------------------------------*/

static DEVICE_START( upd3301 )
{
	upd3301_t *upd3301 = get_safe_token(device);
	const upd3301_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&upd3301->out_int_func, &intf->out_int_func, device);
	devcb_resolve_write_line(&upd3301->out_drq_func, &intf->out_drq_func, device);
	devcb_resolve_write_line(&upd3301->out_hrtc_func, &intf->out_hrtc_func, device);
	devcb_resolve_write_line(&upd3301->out_vrtc_func, &intf->out_vrtc_func, device);

	/* get the screen device */
	upd3301->screen = downcast<screen_device *>(device->machine().device(intf->screen_tag));
	assert(upd3301->screen != NULL);

	/* get character width */
	upd3301->display_func = intf->display_func;
	upd3301->width = intf->width;

	/* create the timers */
	upd3301->vrtc_timer = device->machine().scheduler().timer_alloc(FUNC(vrtc_tick), (void *)device);
	upd3301->hrtc_timer = device->machine().scheduler().timer_alloc(FUNC(hrtc_tick), (void *)device);

	/* register for state saving */
//  device->save_item(NAME(upd3301->));
}

/*-------------------------------------------------
    DEVICE_RESET( upd3301 )
-------------------------------------------------*/

static DEVICE_RESET( upd3301 )
{
	upd3301_t *upd3301 = get_safe_token(device);

	/* set some initial values so PC8001 will boot */
	upd3301->h = 80;
	upd3301->l = 20;
	upd3301->r = 10;
	upd3301->v = 6;
	upd3301->z = 32;
	upd3301->status = 0;

	set_interrupt(device, 0);
	set_drq(device, 0);

	recompute_parameters(device);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( upd3301 )
-------------------------------------------------*/

DEVICE_GET_INFO( upd3301 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd3301_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd3301);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(upd3301);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC uPD3301");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "NEC uPD3301");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(UPD3301, upd3301);
