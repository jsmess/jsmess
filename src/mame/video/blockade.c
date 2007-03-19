#include "driver.h"
#include "includes/blockade.h"

static tilemap *bg_tilemap;

WRITE8_HANDLER( blockade_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}

	if (input_port_3_r(0) & 0x80)
	{
		logerror("blockade_videoram_w: scanline %d\n", cpu_getscanline());
		cpu_spinuntil_int();
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index];

	SET_TILE_INFO(0, code, 0, 0)
}

VIDEO_START( blockade )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	return 0;
}

VIDEO_UPDATE( blockade )
{
	tilemap_draw(bitmap, &Machine->screen[0].visarea, bg_tilemap, 0, 0);
	return 0;
}
