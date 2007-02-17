/***************************************************************************

  aquarius.c

  Functions to emulate the video hardware of the aquarius.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/aquarius.h"

static tilemap *aquarius_tilemap;

/**************************************************************************/

static void aquarius_gettileinfo(int memory_offset)
{
	SET_TILE_INFO(
		0,									/* gfx */
		videoram[memory_offset +    40],	/* character */
		videoram[memory_offset + 0x400],	/* color */
		0)									/* flags */
}

VIDEO_START( aquarius )
{
	aquarius_tilemap = tilemap_create(
		aquarius_gettileinfo,
		tilemap_scan_rows,
		TILEMAP_OPAQUE,
		8, 8,
		40, 24);

	return 0;
}

VIDEO_UPDATE( aquarius )
{
	tilemap_draw(bitmap, NULL, aquarius_tilemap, 0, 0);
	return 0;
}

WRITE8_HANDLER( aquarius_videoram_w )
{
	if (videoram[offset] != data)
	{
		if ((offset >= 40) && (offset < 40+960))
			tilemap_mark_tile_dirty(aquarius_tilemap, offset - 40);
		else if ((offset >= 0x400) && (offset < 0x400+960))
			tilemap_mark_tile_dirty(aquarius_tilemap, offset - 0x400);
		videoram[offset] = data;
	}
}
