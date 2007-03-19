 /****************************************************************************
 *                                                                           *
 * video.c                                                                 *
 *                                                                           *
 * Functions to emulate the video hardware of the machine.                   *
 *                                                                           *
 ****************************************************************************/

#include "driver.h"

UINT8 *speedbal_background_videoram;
UINT8 *speedbal_foreground_videoram;

static tilemap *bg_tilemap, *fg_tilemap;

static void get_tile_info_bg(int tile_index)
{
	int code = speedbal_background_videoram[tile_index*2] + ((speedbal_background_videoram[tile_index*2+1] & 0x30) << 4);
	int color = speedbal_background_videoram[tile_index*2+1] & 0x0f;

	SET_TILE_INFO(1, code, color, TILE_SPLIT(color == 8 ? 1 : 0))
}

static void get_tile_info_fg(int tile_index)
{
	int code = speedbal_foreground_videoram[tile_index*2] + ((speedbal_foreground_videoram[tile_index*2+1] & 0x30) << 4);
	int color = speedbal_foreground_videoram[tile_index*2+1] & 0x0f;

	SET_TILE_INFO(0, code, color, TILE_SPLIT(color == 9 ? 1 : 0))
}

/*************************************
 *                                   *
 *      Start-Stop                   *
 *                                   *
 *************************************/

VIDEO_START( speedbal )
{
	bg_tilemap = tilemap_create(get_tile_info_bg, tilemap_scan_cols_flip_x, TILEMAP_SPLIT, 16, 16, 16, 16);
	fg_tilemap = tilemap_create(get_tile_info_fg, tilemap_scan_cols_flip_x, TILEMAP_SPLIT,  8,  8, 32, 32);

	tilemap_set_transmask(bg_tilemap,0,0xffff,0x0000); /* split type 0 is totally transparent in front half */
	tilemap_set_transmask(bg_tilemap,1,0x00f7,0x0000); /* split type 1 has pen 0-2, 4-7 transparent in front half */

	tilemap_set_transmask(fg_tilemap,0,0xffff,0x0001); /* split type 0 is totally transparent in front half and has pen 0 transparent in back half */
	tilemap_set_transmask(fg_tilemap,1,0x0001,0x0001); /* split type 1 has pen 0 transparent in front and back half */

	return 0;

}



/*************************************
 *                                   *
 *  Foreground characters RAM        *
 *                                   *
 *************************************/

WRITE8_HANDLER( speedbal_foreground_videoram_w )
{
	if (speedbal_foreground_videoram[offset] != data)
	{
		speedbal_foreground_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset>>1);
	}
}

/*************************************
 *                                   *
 *  Background tiles RAM             *
 *                                   *
 *************************************/

WRITE8_HANDLER( speedbal_background_videoram_w )
{
	if (speedbal_background_videoram[offset] != data)
	{
		speedbal_background_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset>>1);
	}
}


/*************************************
 *                                   *
 *   Sprite drawing                  *
 *                                   *
 *************************************/

void speedbal_draw_sprites (mame_bitmap *bitmap, const rectangle *cliprect)
{
	int x,y,code,color,offset,flipx,flipy;

	/* Drawing sprites: 64 in total */

	for (offset = 0;offset < spriteram_size;offset += 4)
	{
		if(!(spriteram[offset + 2] & 0x80))
			continue;

		x = 243 - spriteram[offset + 3];
		y = 239 - spriteram[offset + 0];

		code = BITSWAP8(spriteram[offset + 1],0,1,2,3,4,5,6,7) | ((spriteram[offset + 2] & 0x40) << 2);

		color = spriteram[offset + 2] & 0x0f;

		flipx = flipy = 0;

		if(flip_screen)
		{
			x = 246 - x;
			y = 238 - y;
			flipx = flipy = 1;
		}

		drawgfx (bitmap,Machine->gfx[2],
				code,
				color,
				flipx,flipy,
				x,y,
				cliprect,TRANSPARENCY_PEN,0);
	}
}

/*************************************
 *                                   *
 *   Refresh screen                  *
 *                                   *
 *************************************/

VIDEO_UPDATE( speedbal )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, TILEMAP_BACK, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, TILEMAP_BACK, 0);
	speedbal_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, TILEMAP_FRONT, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, TILEMAP_FRONT, 0);
	return 0;
}
