/* Poke Champ */

#include "driver.h"

static tilemap *bg_tilemap;

WRITE8_HANDLER( pokechmp_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}

WRITE8_HANDLER( pokechmp_flipscreen_w )
{
	if (flip_screen != (data & 0x80))
	{
		flip_screen_set(data & 0x80);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index*2+1] + ((videoram[tile_index*2] & 0x3f) << 8);
	int color = videoram[tile_index*2] >> 6;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( pokechmp )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	return 0;
}

static void pokechmp_draw_sprites( mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] != 0xf8)
		{
			int sx,sy,flipx,flipy;


			sx = 240 - spriteram[offs+2];
			sy = 240 - spriteram[offs];

			flipx = spriteram[offs+1] & 0x04;
			flipy = spriteram[offs+1] & 0x02;
			if (flip_screen) {
				sx=240-sx;
				sy=240-sy;
				if (flipx) flipx=0; else flipx=1;
				if (flipy) flipy=0; else flipy=1;
			}

			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs+3] + ((spriteram[offs+1] & 1) << 8),
					(spriteram[offs+1] & 0x70) >> 4,
					flipx,flipy,
					sx,sy,
					&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);
		}
	}
}

VIDEO_UPDATE( pokechmp )
{
	tilemap_draw(bitmap, &Machine->screen[0].visarea, bg_tilemap, 0, 0);
	pokechmp_draw_sprites(bitmap);
	return 0;
}
