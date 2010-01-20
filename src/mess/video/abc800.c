/*****************************************************************************
 *
 * video/abc800.c
 *
 ****************************************************************************/

/*

    TODO:

    - abc800c

*/

#include "emu.h"
#include "includes/abc80x.h"
#include "machine/z80dart.h"
#include "video/mc6845.h"

/* Palette Initialization */

static PALETTE_INIT( abc800m )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine, 1, 0xff, 0xff, 0x00); // yellow
}

static PALETTE_INIT( abc800c )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff); // blue
	palette_set_color_rgb(machine, 2, 0xff, 0x00, 0x00); // red
	palette_set_color_rgb(machine, 3, 0xff, 0x00, 0xff); // magenta
	palette_set_color_rgb(machine, 4, 0x00, 0xff, 0x00); // green
	palette_set_color_rgb(machine, 5, 0x00, 0xff, 0xff); // cyan
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00); // yellow
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff); // white
}

/* External Interface */

WRITE8_HANDLER( abc800_hrs_w )
{
	abc800_state *state = (abc800_state *)space->machine->driver_data;

	state->hrs = data;
}

WRITE8_HANDLER( abc800_hrc_w )
{
	abc800_state *state = (abc800_state *)space->machine->driver_data;

	state->fgctl = data;
}

/* MC6845 Row Update */

static MC6845_UPDATE_ROW( abc800m_update_row )
{
	abc800_state *state = (abc800_state *)device->machine->driver_data;

	int column;

	/* prevent wraparound */
	if (y >= 240) return;

	y += 29;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT16 address = (state->charram[(ma + column) & 0x7ff] << 4) | (ra & 0x0f);
		UINT8 data = (state->char_rom[address & 0x7ff] & 0x3f);

		if (column == cursor_x)
		{
			data = 0x3f;
		}

		data <<= 2;

		for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
		{
			int x = 115 + (column * ABC800_CHAR_WIDTH) + bit;

			if (BIT(data, 7))
			{
				*BITMAP_ADDR16(bitmap, y, x) = 1;
			}

			data <<= 1;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( abc800_vsync_changed )
{
	abc800_state *driver_state = (abc800_state *)device->machine->driver_data;

	z80dart_rib_w(driver_state->z80dart, state);
}

/* MC6845 Interfaces */

static const mc6845_interface abc800m_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CHAR_WIDTH,
	NULL,
	abc800m_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(abc800_vsync_changed),
	NULL
};

/* ABC 800 C */

static void abc800c_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
}

/* HR */

static void abc800m_hr_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	abc800_state *state = (abc800_state *)machine->driver_data;

	UINT16 addr = 0;
	int sx, y, dot;

	for (y = state->hrs + 29; y < MIN(cliprect->max_y + 1, state->hrs + 29 + 240); y++)
	{
		int x = 115;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 data = state->videoram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				UINT16 fgctl_addr = ((state->fgctl & 0x7f) << 2) | ((data >> 6) & 0x03);
				int color = (state->fgctl_prom[fgctl_addr] & 0x07) ? 1 : 0;

				*BITMAP_ADDR16(bitmap, y, x++) = color;
				*BITMAP_ADDR16(bitmap, y, x++) = color;

				data <<= 2;
			}
		}
	}
}

static void abc800c_hr_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	abc800_state *state = (abc800_state *)machine->driver_data;

	UINT16 addr = 0;
	int sx, y, dot;

	for (y = state->hrs; y < MIN(cliprect->max_y + 1, state->hrs + 240); y++)
	{
		int x = 0;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 data = state->videoram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				UINT16 fgctl_addr = ((state->fgctl & 0x7f) << 2) | ((data >> 6) & 0x03);
				int color = state->fgctl_prom[fgctl_addr] & 0x07;

				*BITMAP_ADDR16(bitmap, y, x++) = color;

				data <<= 2;
			}
		}
	}
}

/* Video Start */

static VIDEO_START( abc800m )
{
	abc800_state *state = (abc800_state *)machine->driver_data;

	/* find devices */
	state->mc6845 = devtag_get_device(machine, MC6845_TAG);

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");
	state->fgctl_prom = memory_region(machine, "fgctl");

	/* register for state saving */
	state_save_register_global(machine, state->hrs);
	state_save_register_global(machine, state->fgctl);
}

static VIDEO_START( abc800c )
{
	abc800_state *state = (abc800_state *)machine->driver_data;

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");
	state->fgctl_prom = memory_region(machine, "fgctl");

	/* register for state saving */
	state_save_register_global(machine, state->hrs);
	state_save_register_global(machine, state->fgctl);
}

/* Video Update */

static VIDEO_UPDATE( abc800m )
{
	abc800_state *state = (abc800_state *)screen->machine->driver_data;

	/* expand visible area to workaround MC6845 */
	video_screen_set_visarea(screen, 0, 767, 0, 311);

	/* clear screen */
	bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine));

	/* draw HR graphics */
	abc800m_hr_update(screen->machine, bitmap, cliprect);

	if (!BIT(state->fgctl, 7))
	{
		/* draw text */
		mc6845_update(state->mc6845, bitmap, cliprect);
	}

	return 0;
}

static VIDEO_UPDATE( abc800c )
{
	abc800_state *state = (abc800_state *)screen->machine->driver_data;

	/* clear screen */
	bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine));

	/* draw HR graphics */
	abc800c_hr_update(screen->machine, bitmap, cliprect);

	if (!BIT(state->fgctl, 7))
	{
		/* draw text */
		abc800c_update(screen->machine, bitmap, cliprect);
	}

	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( abc800m_video )
	MDRV_MC6845_ADD(MC6845_TAG, MC6845, ABC800_CCLK, abc800m_mc6845_interface)

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc800m)
	MDRV_VIDEO_UPDATE(abc800m)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc800c_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(abc800c)
	MDRV_VIDEO_START(abc800c)
	MDRV_VIDEO_UPDATE(abc800c)
MACHINE_DRIVER_END
