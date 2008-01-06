#include "driver.h"

static tilemap *bg_tilemap;

WRITE8_HANDLER( sb2m600_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static TILE_GET_INFO(get_bg_tile_info)
{
	int code = videoram[tile_index];

	SET_TILE_INFO(0, code, 0, 0);
}

VIDEO_START( sb2m600 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_TYPE_PEN, 8, 8, 32, 32);
}

VIDEO_START( uk101 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_TYPE_PEN, 8, 16, 32, 32);
}

VIDEO_UPDATE( sb2m600 )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}
