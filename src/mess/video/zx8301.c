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

#include "driver.h"
#include "zx8301.h"

#define LOG 0

typedef struct _zx8301_t zx8301_t;
struct _zx8301_t
{
	const zx8301_interface *intf;	/* interface */

	const device_config *screen;	/* screen */

	int dispoff;					/* display off */
	int mode8;						/* mode8 active */
	int base;						/* video ram base address */
	int flash;						/* flash */
	int vsync;						/* vertical sync */
	int vda;						/* valid data address */
	
	/* timers */
	emu_timer *flash_timer;			/* flash timer */
	emu_timer *vsync_timer;			/* vertical sync timer */
};

INLINE zx8301_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (zx8301_t *)device->token;
}

/* Timer Callbacks */

static TIMER_CALLBACK(zx8301_flash_tick)
{
	zx8301_t *zx8301 = get_safe_token(ptr);

	zx8301->flash = !zx8301->flash;
}

static TIMER_CALLBACK(zx8301_vsync_tick)
{
	zx8301_t *zx8301 = get_safe_token(ptr);

	//zx8301->vsync = !zx8301->vsync;

	zx8301->intf->on_vsync_changed(ptr, zx8301->vsync);
}

/* Palette Initialization */

static const int ZX8301_COLOR_MODE4[] = { 0, 2, 4, 7 };

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

/* Register Access */

WRITE8_DEVICE_HANDLER( zx8301_control_w )
{
	zx8301_t *zx8301 = get_safe_token(device);

	if (LOG) logerror("ZX8301 Control: %02x\n", data);

	zx8301->dispoff = BIT(data, 1);
	zx8301->mode8 = BIT(data, 3);
	zx8301->base = BIT(data, 7);
}

/* Video Memory Access */

READ8_DEVICE_HANDLER( zx8301_ram_r )
{
	zx8301_t *zx8301 = get_safe_token(device);

	if (LOG) logerror("ZX8301 RAM Read: %06x\n", offset);

	if (zx8301->vda)
	{
		cpu_spinuntil_time(device->machine->cpu[0], video_screen_get_time_until_pos(zx8301->screen, 256, 0));
	}

	return zx8301->intf->ram_r(device, offset);
}

WRITE8_DEVICE_HANDLER( zx8301_ram_w )
{
	zx8301_t *zx8301 = get_safe_token(device);

	if (LOG) logerror("ZX8301 RAM Write: %06x = %02x\n", offset, data);

	if (zx8301->vda)
	{
		cpu_spinuntil_time(device->machine->cpu[0], video_screen_get_time_until_pos(zx8301->screen, 256, 0));
	}

	zx8301->intf->ram_w(device, offset, data);
}

/* Screen Update */

static void zx8301_draw_line_mode4(const device_config *device, bitmap_t *bitmap, int y, UINT16 da)
{
	zx8301_t *zx8301 = get_safe_token(device);

	int word, pixel, x = 0;

	for (word = 0; word < 64; word++)
	{
		UINT8 byte_high = zx8301->intf->ram_r(device, da++);
		UINT8 byte_low = zx8301->intf->ram_r(device, da++);

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

static void zx8301_draw_line_mode8(const device_config *device, bitmap_t *bitmap, int y, UINT16 da)
{
	zx8301_t *zx8301 = get_safe_token(device);

	int word, pixel, x = 0;

	for (word = 0; word < 64; word++)
	{
		UINT8 byte_high = zx8301->intf->ram_r(device, da++);
		UINT8 byte_low = zx8301->intf->ram_r(device, da++);

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

static void zx8301_draw_screen(const device_config *device, bitmap_t *bitmap)
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

void zx8301_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
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

/* Device Interface */

static DEVICE_START( zx8301 )
{
	zx8301_t *zx8301 = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	zx8301->intf = device->static_config;

	assert(zx8301->intf != NULL);
	assert(zx8301->intf->clock > 0);
	assert(zx8301->intf->on_vsync_changed != NULL);
	assert(zx8301->intf->ram_r != NULL);
	assert(zx8301->intf->ram_w != NULL);

	/* set initial values */
	zx8301->vsync = 1;

	/* get the screen device */
	zx8301->screen = device_list_find_by_tag(device->machine->config->devicelist, VIDEO_SCREEN, zx8301->intf->screen_tag);
	assert(zx8301->screen != NULL);

	/* create the timers */
	zx8301->vsync_timer = timer_alloc(machine, zx8301_vsync_tick, (void *)device);
	timer_adjust_periodic(zx8301->vsync_timer, attotime_zero, 0, ATTOTIME_IN_HZ(50)); // HACK

	zx8301->flash_timer = timer_alloc(machine, zx8301_flash_tick, (void *)device);
	timer_adjust_periodic(zx8301->flash_timer, ATTOTIME_IN_HZ(2), 0, ATTOTIME_IN_HZ(2));

	/* register for state saving */
	state_save_register_item(machine, "zx8301", device->tag, 0, zx8301->dispoff);
	state_save_register_item(machine, "zx8301", device->tag, 0, zx8301->mode8);
	state_save_register_item(machine, "zx8301", device->tag, 0, zx8301->base);
	state_save_register_item(machine, "zx8301", device->tag, 0, zx8301->flash);
	state_save_register_item(machine, "zx8301", device->tag, 0, zx8301->vsync);
	state_save_register_item(machine, "zx8301", device->tag, 0, zx8301->vda);

	return DEVICE_START_OK;
}

static DEVICE_RESET( zx8301 )
{
//	zx8301_t *zx8301 = get_safe_token(device);
}

static DEVICE_SET_INFO( zx8301 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( zx8301 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(zx8301_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(zx8301); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(zx8301);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(zx8301);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Sinclair ZX8301";				break;
		case DEVINFO_STR_FAMILY:						info->s = "Sinclair ZX83";					break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright MESS Team";			break;
	}
}
