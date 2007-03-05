/***************************************************************************

Atari Sprint 4 video emulation

***************************************************************************/

#include "driver.h"

static tilemap* playfield;

static mame_bitmap* helper;

int sprint4_collision[4];


PALETTE_INIT( sprint4 )
{
	palette_set_color(machine, 0, 0x00, 0x00, 0x00); /* black  */
	palette_set_color(machine, 1, 0xfc, 0xdf, 0x80); /* peach  */
	palette_set_color(machine, 2, 0xf0, 0x00, 0xf0); /* violet */
	palette_set_color(machine, 3, 0x00, 0xf0, 0x0f); /* green  */
	palette_set_color(machine, 4, 0x30, 0x4f, 0xff); /* blue   */
	palette_set_color(machine, 5, 0xff, 0xff, 0xff); /* white  */

	colortable[0] = 0;
	colortable[2] = 0;
	colortable[4] = 0;
	colortable[6] = 0;
	colortable[8] = 0;

	colortable[1] = 1;
	colortable[3] = 2;
	colortable[5] = 3;
	colortable[7] = 4;
	colortable[9] = 5;
}


static void sprint4_tile_info(int tile_index)
{
	UINT8 code = videoram[tile_index];

	if ((code & 0x30) == 0x30)
	{
		SET_TILE_INFO(0, code & ~0x40, (code >> 6) ^ 3, 0)
	}
	else
	{
		SET_TILE_INFO(0, code, 4, 0)
	}
}


VIDEO_START( sprint4 )
{
	helper = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);

	playfield = tilemap_create(sprint4_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);

	return 0;
}


VIDEO_UPDATE( sprint4 )
{
	int i;

	tilemap_draw(bitmap, cliprect, playfield, 0, 0);

	for (i = 0; i < 4; i++)
	{
		int bank = 0;

		UINT8 horz = videoram[0x390 + 2 * i + 0];
		UINT8 attr = videoram[0x390 + 2 * i + 1];
		UINT8 vert = videoram[0x398 + 2 * i + 0];
		UINT8 code = videoram[0x398 + 2 * i + 1];

		if (i & 1)
		{
			bank = 32;
		}

		drawgfx(bitmap, Machine->gfx[1],
			(code >> 3) | bank,
			(attr & 0x80) ? 4 : i,
			0, 0,
			horz - 15,
			vert - 15,
			cliprect, TRANSPARENCY_PEN, 0);
	}
	return 0;
}


VIDEO_EOF( sprint4 )
{
	UINT16 BG = Machine->gfx[0]->colortable[0];

	int i;

	/* check for sprite-playfield collisions */

	for (i = 0; i < 4; i++)
	{
		rectangle rect;

		int x;
		int y;

		int bank = 0;

		UINT8 horz = videoram[0x390 + 2 * i + 0];
		UINT8 vert = videoram[0x398 + 2 * i + 0];
		UINT8 code = videoram[0x398 + 2 * i + 1];

		rect.min_x = horz - 15;
		rect.min_y = vert - 15;
		rect.max_x = horz - 15 + Machine->gfx[1]->width - 1;
		rect.max_y = vert - 15 + Machine->gfx[1]->height - 1;

		if (rect.min_x < Machine->screen[0].visarea.min_x)
			rect.min_x = Machine->screen[0].visarea.min_x;
		if (rect.min_y < Machine->screen[0].visarea.min_y)
			rect.min_y = Machine->screen[0].visarea.min_y;
		if (rect.max_x > Machine->screen[0].visarea.max_x)
			rect.max_x = Machine->screen[0].visarea.max_x;
		if (rect.max_y > Machine->screen[0].visarea.max_y)
			rect.max_y = Machine->screen[0].visarea.max_y;

		tilemap_draw(helper, &rect, playfield, 0, 0);

		if (i & 1)
		{
			bank = 32;
		}

		drawgfx(helper, Machine->gfx[1],
			(code >> 3) | bank,
			4,
			0, 0,
			horz - 15,
			vert - 15,
			&rect, TRANSPARENCY_PEN, 1);

		for (y = rect.min_y; y <= rect.max_y; y++)
		{
			for (x = rect.min_x; x <= rect.max_x; x++)
			{
				if (read_pixel(helper, x, y) != BG)
				{
					sprint4_collision[i] = 1;
				}
			}
		}
	}
}


WRITE8_HANDLER( sprint4_video_ram_w )
{
	if (data != videoram[offset])
	{
		tilemap_mark_tile_dirty(playfield, offset);
	}

	videoram[offset] = data;
}
