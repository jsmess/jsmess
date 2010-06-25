/**********************************************************************

    Sinclair ZX8301 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - use resnet for colors
    - proper video timing

*/

#include "emu.h"
#include "zx8301.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

static const int ZX8301_COLOR_MODE4[] = { 0, 2, 4, 7 };

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _zx8301_t zx8301_t;
struct _zx8301_t
{
	devcb_resolved_read8		in_ram_func;
	devcb_resolved_write8		out_ram_func;
	devcb_resolved_write_line	out_vsync_func;

	screen_device *screen;	/* screen */

	int dispoff;					/* display off */
	int mode8;						/* mode8 active */
	int base;						/* video ram base address */
	int flash;						/* flash */
	int vsync;						/* vertical sync */
	int vda;						/* valid data address */

	/* timers */
	emu_timer *flash_timer;			/* flash timer */
	emu_timer *vsync_timer;			/* vertical sync timer */

	running_device *cpu;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE zx8301_t *get_safe_token(running_device *device)
{
	assert(device != NULL);

	return (zx8301_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const zx8301_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert((device->type() == ZX8301));
	return (const zx8301_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( zx8301_flash_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( zx8301_flash_tick )
{
	zx8301_t *zx8301 = get_safe_token((running_device *)ptr);

	zx8301->flash = !zx8301->flash;
}

/*-------------------------------------------------
    TIMER_CALLBACK( zx8301_vsync_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( zx8301_vsync_tick )
{
	zx8301_t *zx8301 = get_safe_token((running_device *)ptr);

	//zx8301->vsync = !zx8301->vsync;

	devcb_call_write_line(&zx8301->out_vsync_func, zx8301->vsync);
}

/*-------------------------------------------------
    PALETTE_INIT( zx8301 )
-------------------------------------------------*/

PALETTE_INIT( zx8301 )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00 ); // black
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff ); // blue
	palette_set_color_rgb(machine, 2, 0xff, 0x00, 0x00 ); // red
	palette_set_color_rgb(machine, 3, 0xff, 0x00, 0xff ); // magenta
	palette_set_color_rgb(machine, 4, 0x00, 0xff, 0x00 ); // green
	palette_set_color_rgb(machine, 5, 0x00, 0xff, 0xff ); // cyan
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00 ); // yellow
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff ); // white
}

/*-------------------------------------------------
    zx8301_control_w - display control register
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8301_control_w )
{
	zx8301_t *zx8301 = get_safe_token(device);

	if (LOG) logerror("ZX8301 Control: %02x\n", data);

	zx8301->dispoff = BIT(data, 1);
	zx8301->mode8 = BIT(data, 3);
	zx8301->base = BIT(data, 7);
}

/*-------------------------------------------------
    zx8301_ram_r - RAM access
-------------------------------------------------*/

READ8_DEVICE_HANDLER( zx8301_ram_r )
{
	zx8301_t *zx8301 = get_safe_token(device);

	if (LOG) logerror("ZX8301 RAM Read: %06x\n", offset);

	if (zx8301->vda)
	{
		cpu_spinuntil_time(zx8301->cpu, zx8301->screen->time_until_pos(256, 0));
	}

	return devcb_call_read8(&zx8301->in_ram_func, offset);
}

/*-------------------------------------------------
    zx8301_ram_w - RAM access
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( zx8301_ram_w )
{
	zx8301_t *zx8301 = get_safe_token(device);

	if (LOG) logerror("ZX8301 RAM Write: %06x = %02x\n", offset, data);

	if (zx8301->vda)
	{
		cpu_spinuntil_time(zx8301->cpu, zx8301->screen->time_until_pos(256, 0));
	}

	devcb_call_write8(&zx8301->out_ram_func, offset, data);
}

/*-------------------------------------------------
    zx8301_draw_line_mode4 - high resolution
    line drawing routine
-------------------------------------------------*/

static void zx8301_draw_line_mode4(running_device *device, bitmap_t *bitmap, int y, UINT16 da)
{
	zx8301_t *zx8301 = get_safe_token(device);

	int word, pixel, x = 0;

	for (word = 0; word < 64; word++)
	{
		UINT8 byte_high = devcb_call_read8(&zx8301->in_ram_func, da++);
		UINT8 byte_low = devcb_call_read8(&zx8301->in_ram_func, da++);

		for (pixel = 0; pixel < 8; pixel++)
		{
			int red = BIT(byte_low, 7);
			int green = BIT(byte_high, 7);
			int color = (green << 1) | red;

			*BITMAP_ADDR16(bitmap, y, x++) = ZX8301_COLOR_MODE4[color];

			byte_high <<= 1;
			byte_low <<= 1;
		}
	}
}

/*-------------------------------------------------
    zx8301_draw_line_mode4 - low resolution
    line drawing routine
-------------------------------------------------*/

static void zx8301_draw_line_mode8(running_device *device, bitmap_t *bitmap, int y, UINT16 da)
{
	zx8301_t *zx8301 = get_safe_token(device);

	int word, pixel, x = 0;

	for (word = 0; word < 64; word++)
	{
		UINT8 byte_high = devcb_call_read8(&zx8301->in_ram_func, da++);
		UINT8 byte_low = devcb_call_read8(&zx8301->in_ram_func, da++);

		for (pixel = 0; pixel < 4; pixel++)
		{
			int red = BIT(byte_low, 7);
			int green = BIT(byte_high, 7);
			int blue = BIT(byte_low, 6);
			int flash = BIT(byte_high, 6);

			int color = (green << 2) | (red << 1) | blue;

			if (flash && zx8301->flash)
			{
				color = 0;
			}

			*BITMAP_ADDR16(bitmap, y, x++) = color;
			*BITMAP_ADDR16(bitmap, y, x++) = color;

			byte_high <<= 2;
			byte_low <<= 2;
		}
	}
}

/*-------------------------------------------------
    zx8301_draw_screen - draw screen
-------------------------------------------------*/

static void zx8301_draw_screen(running_device *device, bitmap_t *bitmap)
{
	zx8301_t *zx8301 = get_safe_token(device);

	UINT32 da = zx8301->base << 15;
	int y;

	zx8301->vda = 1;

	for (y = 0; y < 256; y++)
	{
		if (zx8301->mode8)
		{
			zx8301_draw_line_mode8(device, bitmap, y, da);
		}
		else
		{
			zx8301_draw_line_mode4(device, bitmap, y, da);
		}

		da += 128;
	}

	zx8301->vda = 0;
}

/*-------------------------------------------------
    zx8301_draw_screen - screen update
-------------------------------------------------*/

void zx8301_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	zx8301_t *zx8301 = get_safe_token(device);

	if (!zx8301->dispoff)
	{
		zx8301_draw_screen(device, bitmap);
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(zx8301->screen->machine));
	}
}

/*-------------------------------------------------
    DEVICE_START( zx8301 )
-------------------------------------------------*/

static DEVICE_START( zx8301 )
{
	zx8301_t *zx8301 = get_safe_token(device);
	const zx8301_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&zx8301->in_ram_func, &intf->in_ram_func, device);
	devcb_resolve_write8(&zx8301->out_ram_func, &intf->out_ram_func, device);
	devcb_resolve_write_line(&zx8301->out_vsync_func, &intf->out_vsync_func, device);

	/* set initial values */
	zx8301->vsync = 1;

	/* get the cpu */
	zx8301->cpu = devtag_get_device(device->machine, intf->cpu_tag);

	/* get the screen device */
	zx8301->screen = device->machine->device<screen_device>(intf->screen_tag);
	assert(zx8301->screen != NULL);

	/* create the timers */
	zx8301->vsync_timer = timer_alloc(device->machine, zx8301_vsync_tick, (void *)device);
	timer_adjust_periodic(zx8301->vsync_timer, attotime_zero, 0, ATTOTIME_IN_HZ(50)); // HACK

	zx8301->flash_timer = timer_alloc(device->machine, zx8301_flash_tick, (void *)device);
	timer_adjust_periodic(zx8301->flash_timer, ATTOTIME_IN_HZ(2), 0, ATTOTIME_IN_HZ(2));

	/* register for state saving */
	state_save_register_item(device->machine, "zx8301", device->tag(), 0, zx8301->dispoff);
	state_save_register_item(device->machine, "zx8301", device->tag(), 0, zx8301->mode8);
	state_save_register_item(device->machine, "zx8301", device->tag(), 0, zx8301->base);
	state_save_register_item(device->machine, "zx8301", device->tag(), 0, zx8301->flash);
	state_save_register_item(device->machine, "zx8301", device->tag(), 0, zx8301->vsync);
	state_save_register_item(device->machine, "zx8301", device->tag(), 0, zx8301->vda);
}

/*-------------------------------------------------
    DEVICE_RESET( zx8301 )
-------------------------------------------------*/

static DEVICE_RESET( zx8301 )
{
//  zx8301_t *zx8301 = get_safe_token(device);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( zx8301 )
-------------------------------------------------*/

DEVICE_GET_INFO( zx8301 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(zx8301_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;									break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(zx8301);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(zx8301);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Sinclair ZX8301");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Sinclair QL");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}

DEFINE_LEGACY_DEVICE(ZX8301, zx8301);
