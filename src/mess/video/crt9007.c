/**********************************************************************

    SMC CRT9007 CRT Video Processor and Controller (VPAC) emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- interrupts
	- status register
	- reset
	- DMA mode (currently forcing to non-DMA mode)
	- cursor/blank skew
	- sequential breaks
	- interlaced mode
	- smooth scroll
	- page blank
	- double height cursor
	- row attributes
	- pin configuration
	- operation modes 0,4,7
	- address modes 1,2,3
	- light pen
	- state saving

*/

#include "emu.h"
#include "crt9007.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

#define HAS_VALID_PARAMETERS \
	(crt9007->reg[0x00] && crt9007->reg[0x01] && crt9007->reg[0x07] && crt9007->reg[0x08] && crt9007->reg[0x09])

#define CHARACTERS_PER_HORIZONTAL_PERIOD \
	crt9007->reg[0x00]

#define CHARACTERS_PER_DATA_ROW \
	(crt9007->reg[0x01] + 1)

#define HORIZONTAL_DELAY \
	crt9007->reg[0x02]

#define HORIZONTAL_SYNC_WIDTH \
	crt9007->reg[0x03]

#define VERTICAL_SYNC_WIDTH \
	crt9007->reg[0x04]

#define VERTICAL_DELAY \
	(crt9007->reg[0x05] - 1)

#define PIN_CONFIGURATION \
	(crt9007->reg[0x06] >> 6)

#define CURSOR_SKEW \
	((crt9007->reg[0x06] >> 3) & 0x07)

#define BLANK_SKEW \
	(crt9007->reg[0x06] & 0x07)

#define VISIBLE_DATA_ROWS_PER_FRAME \
	(crt9007->reg[0x07] + 1)

#define SCAN_LINES_PER_DATA_ROW \
	((crt9007->reg[0x08] & 0x1f) + 1)

#define SCAN_LINES_PER_FRAME \
	(((crt9007->reg[0x08] << 3) & 0x0700) | crt9007->reg[0x09])

#define DMA_BURST_COUNT \
	(crt9007->reg[0x0a] & 0x0f)

#define DMA_BURST_DELAY \
	((crt9007->reg[0x0a] >> 4) & 0x07)

#define DMA_DISABLE \
	BIT(crt9007->reg[0x0a], 7)

#define SINGLE_HEIGHT_CURSOR \
	BIT(crt9007->reg[0x0b], 0)

#define OPERATION_MODE \
	((crt9007->reg[0x0b] >> 1) & 0x07)

#define INTERLACE_MODE \
	((crt9007->reg[0x0b] >> 4) & 0x03)

#define PAGE_BLANK \
	BIT(crt9007->reg[0x0b], 6)

#define TABLE_START \
	(((crt9007->reg[0x0d] << 8) & 0x3f00) | crt9007->reg[0x0c])

#define ADDRESS_MODE \
	((crt9007->reg[0x0d] >> 6) & 0x03)

#define AUXILIARY_ADDRESS_1 \
	(((crt9007->reg[0x0f] << 8) & 0x3f00) | crt9007->reg[0x0e])

#define ROW_ATTRIBUTES_1 \
	((crt9007->reg[0x0f] >> 6) & 0x03)

#define SEQUENTIAL_BREAK_1 \
	crt9007->reg[0x10]

#define SEQUENTIAL_BREAK_2 \
	crt9007->reg[0x12]

#define DATA_ROW_START \
	crt9007->reg[0x11]

#define DATA_ROW_END \
	crt9007->reg[0x12]

#define AUXILIARY_ADDRESS_2 \
	(((crt9007->reg[0x14] << 8) & 0x3f00) | crt9007->reg[0x13])

#define ROW_ATTRIBUTES_2 \
	((crt9007->reg[0x14] >> 6) & 0x03)

#define SMOOTH_SCROLL_OFFSET \
	((crt9007->reg[0x17] >> 1) & 0x3f)

#define SMOOTH_SCROLL_OFFSET_OVERFLOW \
	BIT(crt9007->reg[0x17], 7)

#define VERTICAL_CURSOR \
	crt9007->reg[0x18]

#define HORIZONTAL_CURSOR \
	crt9007->reg[0x19]

#define FRAME_TIMER \
	BIT(crt9007->reg[0x1a], 0)

#define LIGHT_PEN_INTERRUPT \
	BIT(crt9007->reg[0x1a], 5)

#define VERTICAL_RETRACE_INTERRUPT \
	BIT(crt9007->reg[0x1a], 6)

#define VERTICAL_LIGHT_PEN \
	crt9007->reg[0x3b]

#define HORIZONTAL_LIGHT_PEN \
	crt9007->reg[0x3c]

enum
{
	NON_INTERLACED = 0,
	ENHANCED_VIDEO_INTERFACE,
	NORMAL_VIDEO_INTERFACE,
};

enum
{
	OPERATION_MODE_REPETITIVE_MEMORY_ADDRESSING = 0,
	OPERATION_MODE_DOUBLE_ROW_BUFFER,
	OPERATION_MODE_SINGLE_ROW_BUFFER = 4,
	OPERATION_MODE_ATTRIBUTE_ASSEMBLE = 7
};

enum
{
	ADDRESS_MODE_SEQUENTIAL_ADDRESSING = 0,
	ADDRESS_MODE_SEQUENTIAL_ROLL_ADDRESSING,
	ADDRESS_MODE_CONTIGUOUS_ROW_TABLE,
	ADDRESS_MODE_LINKED_LIST_ROW_TABLE
};

#define STATUS_INTERRUPT_PENDING	0x80
#define STATUS_VERTICAL_RETRACE		0x40
#define STATUS_LIGHT_PEN_UPDATE		0x20
#define STATUS_ODD_EVEN				0x04
#define STATUS_FRAME_TIMER_OCCURRED	0x01	

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _crt9007_t crt9007_t;
struct _crt9007_t
{
	devcb_resolved_write_line	out_int_func;
	devcb_resolved_write_line	out_dmar_func;
	devcb_resolved_write_line	out_hs_func;
	devcb_resolved_write_line	out_vs_func;
	devcb_resolved_read8		in_vd_func;

	crt9007_draw_scanline_func	draw_scanline_func;

	running_device *screen;			/* screen */

	/* registers */
	UINT8 reg[0x3d];
	UINT8 status;

	int disp;
	int hpixels_per_column;

	/* runtime variables, do not state save */
	int vsync_start;
	int vsync_end;
	int vfp;
	int hsync_start;
	int hsync_end;
	int hfp;

	/* timers */
	emu_timer *vsync_timer;			/* vertical sync timer */
	emu_timer *hsync_timer;			/* horizontal sync timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE crt9007_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (crt9007_t *)device->token;
}

INLINE const crt9007_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert(device->type == CRT9007);
	return (const crt9007_interface *) device->baseconfig().static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    update_vsync_timer -
-------------------------------------------------*/

static void update_vsync_timer(crt9007_t *crt9007, int state)
{
	int next_y = state ? crt9007->vsync_start : crt9007->vsync_end;

	attotime duration = video_screen_get_time_until_pos(crt9007->screen, next_y, 0);

	timer_adjust_oneshot(crt9007->vsync_timer, duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( vsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( vsync_tick )
{
	running_device *device = (running_device *)ptr;
	crt9007_t *crt9007 = get_safe_token(device);

	devcb_call_write_line(&crt9007->out_vs_func, param);

	update_vsync_timer(crt9007, param);
}

/*-------------------------------------------------
    update_hsync_timer -
-------------------------------------------------*/

static void update_hsync_timer(crt9007_t *crt9007, int state)
{
	int y = video_screen_get_vpos(crt9007->screen);

	int next_x = state ? crt9007->hsync_start : crt9007->hsync_end;
	int next_y = (y + 1) % SCAN_LINES_PER_FRAME;

	attotime duration = video_screen_get_time_until_pos(crt9007->screen, next_y, next_x);

	timer_adjust_oneshot(crt9007->hsync_timer, duration, !state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( hsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( hsync_tick )
{
	running_device *device = (running_device *)ptr;
	crt9007_t *crt9007 = get_safe_token(device);

	devcb_call_write_line(&crt9007->out_hs_func, param);

	update_hsync_timer(crt9007, param);
}

/*-------------------------------------------------
    recompute_parameters -
-------------------------------------------------*/

static void recompute_parameters(running_device *device)
{
	crt9007_t *crt9007 = get_safe_token(device);
	
	/* check that necessary registers have been loaded */
	if (!HAS_VALID_PARAMETERS) return;

	int horiz_pix_total = CHARACTERS_PER_HORIZONTAL_PERIOD * crt9007->hpixels_per_column;
	int vert_pix_total = SCAN_LINES_PER_FRAME;

	attoseconds_t refresh = HZ_TO_ATTOSECONDS(device->clock) * horiz_pix_total * vert_pix_total;

	crt9007->hsync_start = (CHARACTERS_PER_HORIZONTAL_PERIOD - HORIZONTAL_SYNC_WIDTH) * crt9007->hpixels_per_column;
	crt9007->hsync_end = 0;
	crt9007->hfp = (HORIZONTAL_DELAY - HORIZONTAL_SYNC_WIDTH) * crt9007->hpixels_per_column;

	crt9007->vsync_start = SCAN_LINES_PER_FRAME - VERTICAL_SYNC_WIDTH;
	crt9007->vsync_end = 0;
	crt9007->vfp = VERTICAL_DELAY - VERTICAL_SYNC_WIDTH;

	rectangle visarea;

	visarea.min_x = crt9007->hsync_end;
	visarea.max_x = crt9007->hsync_start - 1;

	visarea.min_y = crt9007->vsync_end;
	visarea.max_y = crt9007->vsync_start - 1;

	if (LOG)
	{
		logerror("CRT9007 '%s' Screen: %u x %u @ %f Hz\n", device->tag(), horiz_pix_total, vert_pix_total, 1 / ATTOSECONDS_TO_DOUBLE(refresh));
		logerror("CRT9007 '%s' Visible Area: (%u, %u) - (%u, %u)\n", device->tag(), visarea.min_x, visarea.min_y, visarea.max_x, visarea.max_y);
	}

	video_screen_configure(crt9007->screen, horiz_pix_total, vert_pix_total, &visarea, refresh);

	update_hsync_timer(crt9007, 1);
	update_vsync_timer(crt9007, 1);
}

/*-------------------------------------------------
    crt9007_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( crt9007_r )
{
	crt9007_t *crt9007 = get_safe_token(device);

	UINT8 data = 0;

	switch (offset)
	{
	case 0x15:
		crt9007->disp = 1;
		if (LOG) logerror("CRT9007 '%s' Start\n", device->tag());
		break;

	case 0x16:
		crt9007->disp = 0;
		if (LOG) logerror("CRT9007 '%s' Reset\n", device->tag());
		break;

	case 0x38:
		data = VERTICAL_CURSOR;
		break;

	case 0x39:
		data = HORIZONTAL_CURSOR;
		break;

	case 0x3a:
		data = crt9007->status;
		break;

	case 0x3b:
		data = VERTICAL_LIGHT_PEN;
		break;

	case 0x3c:
		data = HORIZONTAL_LIGHT_PEN;
		break;

	default:
		logerror("CRT9007 '%s' Read from Invalid Register: %02x!\n", device->tag(), offset);
	}

	return data;
}

/*-------------------------------------------------
    crt9007_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( crt9007_w )
{
	crt9007_t *crt9007 = get_safe_token(device);

	crt9007->reg[offset] = data;

	switch (offset)
	{
	case 0x00:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Characters per Horizontal Period: %u\n", device->tag(), CHARACTERS_PER_HORIZONTAL_PERIOD);
		break;
	
	case 0x01:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Characters per Data Row: %u\n", device->tag(), CHARACTERS_PER_DATA_ROW);
		break;

	case 0x02:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Horizontal Delay: %u\n", device->tag(), HORIZONTAL_DELAY);
		break;

	case 0x03:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Horizontal Sync Width: %u\n", device->tag(), HORIZONTAL_SYNC_WIDTH);
		break;

	case 0x04:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Vertical Sync Width: %u\n", device->tag(), VERTICAL_SYNC_WIDTH);
		break;

	case 0x05:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Vertical Delay: %u\n", device->tag(), VERTICAL_DELAY);
		break;

	case 0x06:
		recompute_parameters(device);
		if (LOG) 
		{
			logerror("CRT9007 '%s' Pin Configuration: %u\n", device->tag(), PIN_CONFIGURATION);
			logerror("CRT9007 '%s' Cursor Skew: %u\n", device->tag(), CURSOR_SKEW);
			logerror("CRT9007 '%s' Blank Skew: %u\n", device->tag(), BLANK_SKEW);
		}
		break;

	case 0x07:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Visible Data Rows per Frame: %u\n", device->tag(), VISIBLE_DATA_ROWS_PER_FRAME);
		break;

	case 0x08:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Scan Lines per Data Row: %u\n", device->tag(), SCAN_LINES_PER_DATA_ROW);
		break;

	case 0x09:
		recompute_parameters(device);
		if (LOG) logerror("CRT9007 '%s' Scan Lines per Frame: %u\n", device->tag(), SCAN_LINES_PER_FRAME);
		break;

	case 0x0a:
		if (LOG)
		{
			logerror("CRT9007 '%s' DMA Burst Count: %u\n", device->tag(), DMA_BURST_COUNT);
			logerror("CRT9007 '%s' DMA Burst Delay: %u\n", device->tag(), DMA_BURST_DELAY);
			logerror("CRT9007 '%s' DMA Disable: %u\n", device->tag(), DMA_DISABLE);
		}
		break;

	case 0x0b:
		if (LOG)
		{
			logerror("CRT9007 '%s' %s Height Cursor\n", device->tag(), SINGLE_HEIGHT_CURSOR ? "Single" : "Double");
			logerror("CRT9007 '%s' Operation Mode: %u\n", device->tag(), OPERATION_MODE);
			logerror("CRT9007 '%s' Interlace Mode: %u\n", device->tag(), INTERLACE_MODE);
			logerror("CRT9007 '%s' %s Mechanism\n", device->tag(), PAGE_BLANK ? "Page Blank" : "Smooth Scroll");
		}
		break;

	case 0x0c:
		break;

	case 0x0d:
		if (LOG)
		{
			logerror("CRT9007 '%s' Table Start Register: %04x\n", device->tag(), TABLE_START);
			logerror("CRT9007 '%s' Address Mode: %u\n", device->tag(), ADDRESS_MODE);
		}
		break;

	case 0x0e:
		break;

	case 0x0f:
		if (LOG)
		{
			logerror("CRT9007 '%s' Auxialiary Address Register 1: %04x\n", device->tag(), AUXILIARY_ADDRESS_1);
			logerror("CRT9007 '%s' Row Attributes: %u\n", device->tag(), ROW_ATTRIBUTES_1);
		}
		break;

	case 0x10:
		if (LOG) logerror("CRT9007 '%s' Sequential Break Register 1: %u\n", device->tag(), SEQUENTIAL_BREAK_1);
		break;

	case 0x11:
		if (LOG) logerror("CRT9007 '%s' Data Row Start Register: %u\n", device->tag(), DATA_ROW_START);
		break;

	case 0x12:
		if (LOG) logerror("CRT9007 '%s' Data Row End/Sequential Break Register 2: %u\n", device->tag(), SEQUENTIAL_BREAK_2);
		break;

	case 0x13:
		break;

	case 0x14:
		if (LOG)
		{
			logerror("CRT9007 '%s' Auxiliary Address Register 2: %04x\n", device->tag(), AUXILIARY_ADDRESS_2);
			logerror("CRT9007 '%s' Row Attributes: %u\n", device->tag(), ROW_ATTRIBUTES_2);
		}
		break;

	case 0x15:
		crt9007->disp = 1;
		if (LOG) logerror("CRT9007 '%s' Start\n", device->tag());
		break;

	case 0x16:
		crt9007->disp = 0;
		if (LOG) logerror("CRT9007 '%s' Reset\n", device->tag());
		break;

	case 0x17:
		if (LOG)
		{
			logerror("CRT9007 '%s' Smooth Scroll Offset: %u\n", device->tag(), SMOOTH_SCROLL_OFFSET);
			logerror("CRT9007 '%s' Smooth Scroll Offset Overflow: %u\n", device->tag(), SMOOTH_SCROLL_OFFSET_OVERFLOW);
		}
		break;

	case 0x18:
		if (LOG) logerror("CRT9007 '%s' Vertical Cursor Register: %u\n", device->tag(), VERTICAL_CURSOR);
		break;

	case 0x19:
		if (LOG) logerror("CRT9007 '%s' Horizontal Cursor Register: %u\n", device->tag(), HORIZONTAL_CURSOR);
		break;

	case 0x1a:
		if (LOG)
		{
			logerror("CRT9007 '%s' Frame Timer: %u\n", device->tag(), FRAME_TIMER);
			logerror("CRT9007 '%s' Light Pen Interrupt: %u\n", device->tag(), LIGHT_PEN_INTERRUPT);
			logerror("CRT9007 '%s' Vertical Retrace Interrupt: %u\n", device->tag(), VERTICAL_RETRACE_INTERRUPT);
		}
		break;

	default:
		logerror("CRT9007 '%s' Write to Invalid Register: %02x!\n", device->tag(), offset);
	}
}

/*-------------------------------------------------
    crt9007_ack_w - DMA acknowledge
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( crt9007_ack_w )
{
}

/*-------------------------------------------------
    crt9007_lpstb_w - light pen strobe
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( crt9007_lpstb_w )
{
}

/*-------------------------------------------------
    crt9007_set_clock - set device clock
-------------------------------------------------*/

void crt9007_set_clock(running_device *device, UINT32 clock)
{
	device->set_clock(clock);
	recompute_parameters(device);
}

/*-------------------------------------------------
    crt9007_set_hpixels_per_column
-------------------------------------------------*/

void crt9007_set_hpixels_per_column(running_device *device, int hpixels_per_column)
{
	crt9007_t *crt9007 = get_safe_token(device);

	crt9007->hpixels_per_column = hpixels_per_column;
	recompute_parameters(device);
}

/*-------------------------------------------------
    crt9007_update - update screen
-------------------------------------------------*/

void crt9007_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	crt9007_t *crt9007 = get_safe_token(device);

	if (crt9007->disp && HAS_VALID_PARAMETERS)
	{
		int min_y = crt9007->vfp;
		int max_y = min_y + (VISIBLE_DATA_ROWS_PER_FRAME * SCAN_LINES_PER_DATA_ROW);

		for (int y = min_y; y < max_y; y++)
		{
			int sl = (y - min_y) % SCAN_LINES_PER_DATA_ROW;
			int sy = (y - min_y) / SCAN_LINES_PER_DATA_ROW;
			int cursor_x = (sy == VERTICAL_CURSOR) ? HORIZONTAL_CURSOR : -1;
			UINT16 addr = TABLE_START + (sy * CHARACTERS_PER_DATA_ROW);
//			UINT8 data = devcb_call_read8(&crt9007->in_vd_func, addr);
			
			crt9007->draw_scanline_func(device, bitmap, cliprect, addr, sl, 0, y, crt9007->hfp, CHARACTERS_PER_DATA_ROW, cursor_x);

			addr++;
		}
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(device->machine));
	}
}

/*-------------------------------------------------
    DEVICE_START( crt9007 )
-------------------------------------------------*/

static DEVICE_START( crt9007 )
{
	crt9007_t *crt9007 = (crt9007_t *)device->token;
	const crt9007_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&crt9007->out_int_func, &intf->out_int_func, device);
	devcb_resolve_write_line(&crt9007->out_dmar_func, &intf->out_dmar_func, device);
	devcb_resolve_write_line(&crt9007->out_hs_func, &intf->out_hs_func, device);
	devcb_resolve_write_line(&crt9007->out_vs_func, &intf->out_vs_func, device);
	devcb_resolve_read8(&crt9007->in_vd_func, &intf->in_vd_func, device);

	crt9007->draw_scanline_func = intf->draw_scanline_func;
	assert(crt9007->draw_scanline_func != NULL);

	/* get the screen device */
	crt9007->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(crt9007->screen != NULL);

	/* set horizontal pixels per column */
	crt9007->hpixels_per_column = intf->hpixels_per_column;

	/* create the timers */
	crt9007->vsync_timer = timer_alloc(device->machine, vsync_tick, (void *)device);
	crt9007->hsync_timer = timer_alloc(device->machine, hsync_tick, (void *)device);

	/* register for state saving */
//	state_save_register_device_item(device, 0, crt9007->);
}

/*-------------------------------------------------
    DEVICE_RESET( crt9007 )
-------------------------------------------------*/

static DEVICE_RESET( crt9007 )
{
//	crt9007_t *crt9007 = (crt9007_t *)device->token;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( crt9007 )
-------------------------------------------------*/

DEVICE_GET_INFO( crt9007 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(crt9007_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(crt9007);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(crt9007);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "SMC CRT9007");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "SMC CRT9007");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}
