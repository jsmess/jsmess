#include "video/zx8301.h"

typedef struct
{
	int dispoff;
	int mode8;
	int base;
	int flash;
} ZX8301_VIDEO_CONFIG;

static ZX8301_VIDEO_CONFIG zx8301;

static const int ZX8301_COLOR_MODE4[] = { 0, 2, 4, 7 };

static emu_timer *zx8301_flash_timer;

static TIMER_CALLBACK(zx8301_flash_tick)
{
	zx8301.flash = zx8301.flash ? 0 : 1;
}

PALETTE_INIT( zx8301 )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00 ); // black
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff ); // blue
	palette_set_color_rgb(machine, 2, 0xff, 0x00, 0x00 ); // red
	palette_set_color_rgb(machine, 3, 0xff, 0x00, 0xff ); // magenta
	palette_set_color_rgb(machine, 4, 0x00, 0xff, 0x00 ); // green
	palette_set_color_rgb(machine, 5, 0x00, 0xff, 0xff ); // cyan
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00 ); // yellow
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff ); // whit
}

WRITE8_HANDLER( zx8301_control_w )
{
	zx8301.dispoff = BIT(data, 1);
	zx8301.mode8 = BIT(data, 3);
	zx8301.base = BIT(data, 7);
}

static void zx8301_draw_screen(mame_bitmap *bitmap)
{
	UINT32 addr = zx8301.base << 15;
	int y, word, pixel;

	if (zx8301.mode8)
	{
		// mode 8/256 GFGFGFGF RBRBRBRB

		for (y = 0; y < 256; y++)
		{
			int x = 0;

			for (word = 0; word < 64; word++)
			{
				UINT8 byte_high = videoram[addr++];
				UINT8 byte_low = videoram[addr++];

				for (pixel = 0; pixel < 4; pixel++)
				{
					int red = BIT(byte_low, 7);
					int green = BIT(byte_high, 7);
					int blue = BIT(byte_low, 6);
					int flash = BIT(byte_high, 6);

					int color = (green << 2) | (red << 1) | blue;

					if (flash && zx8301.flash)
					{
						color = 0;
					}

					*BITMAP_ADDR16(bitmap, y, x++) = Machine->pens[color];
					*BITMAP_ADDR16(bitmap, y, x++) = Machine->pens[color];

					byte_high <<= 2;
					byte_low <<= 2;
				}
			}
		}
	}
	else
	{
		// mode 4/512 GGGGGGGG RRRRRRRR

		for (y = 0; y < 256; y++)
		{
			int x = 0;

			for (word = 0; word < 64; word++)
			{
				UINT8 byte_high = videoram[addr++];
				UINT8 byte_low = videoram[addr++];

				for (pixel = 0; pixel < 8; pixel++)
				{
					int red = BIT(byte_low, 7);
					int green = BIT(byte_high, 7);
					int color = (green << 1) | red;

					*BITMAP_ADDR16(bitmap, y, x++) = Machine->pens[ZX8301_COLOR_MODE4[color]];

					byte_high <<= 1;
					byte_low <<= 1;
				}
			}
		}
	}
}

VIDEO_START( zx8301 )
{
	zx8301_flash_timer = timer_alloc(zx8301_flash_tick);
	timer_adjust(zx8301_flash_timer, ATTOTIME_IN_HZ(2), 0, ATTOTIME_IN_HZ(2));

	state_save_register_global(zx8301.dispoff);
	state_save_register_global(zx8301.mode8);
	state_save_register_global(zx8301.base);
	state_save_register_global(zx8301.flash);
}

VIDEO_UPDATE( zx8301 )
{
	if (!zx8301.dispoff)
	{
		zx8301_draw_screen(bitmap);
	}
	else
	{
		fillbitmap(bitmap, get_black_pen(Machine), cliprect);
	}

	return 0;
}
