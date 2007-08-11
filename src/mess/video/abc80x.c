#include "abc80x.h"

static tilemap *tx_tilemap;
static tilemap *tx_tilemap_40;
static int abc802_columns;

WRITE8_HANDLER( abc800_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(tx_tilemap, offset);
}

PALETTE_INIT( abc800m )
{
	palette_set_color_rgb(machine,  0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine,  1, 0xff, 0xff, 0x00); // yellow (really white, but blue signal is disconnected from monitor)
}

PALETTE_INIT( abc800c )
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

static const crtc6845_interface crtc6845_intf =
{
	0,						/* screen we are acting on */
	ABC800_X01/6,			/* the clock (pin 21) of the chip */
	8,						/* number of pixels per video memory address */
	0,						/* before pixel update callback */
	0,						/* row update callback */
	0,						/* after pixel update callback */
	0						/* call back for display state changes */
};

static TILE_GET_INFO(abc800_get_tile_info)
{
	int attr = videoram[tile_index];
	int bank = 0;	// TODO: bank 1 is graphics mode, add a [40][25] array to support it, also to videoram_w
	int code = attr & 0x7f;
	int color = (attr & 0x80) ? 1 : 0;

	SET_TILE_INFO(bank, code, color, 0);
}

VIDEO_START( abc800m )
{
	tx_tilemap = tilemap_create(abc800_get_tile_info, tilemap_scan_rows, 
		TILEMAP_TYPE_PEN, 6, 10, 80, 24);
}

static TILE_GET_INFO(abc800c_get_tile_info)
{
	int code = videoram[tile_index];
	int color = 1;						// WRONG!

	SET_TILE_INFO(0, code, color, 0);
}

VIDEO_START( abc800c )
{
	tx_tilemap = tilemap_create(abc800c_get_tile_info, tilemap_scan_rows,
		TILEMAP_TYPE_PEN, 6, 10, 40, 24);
}

VIDEO_UPDATE( abc800 )
{
	tilemap_mark_all_tiles_dirty(tx_tilemap);
	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 0);

	return 0;
}

static TILE_GET_INFO(abc802_get_tile_info_40)
{
	int attr = videoram[tile_index * 2];
	int bank = 0;
	int code = attr & 0x7f;
	int color = (attr & 0x80) ? 1 : 0;

	SET_TILE_INFO(bank, code, color, 0);
}

static TILE_GET_INFO(abc802_get_tile_info_80)
{
	int attr = videoram[tile_index];
	int bank = 1;
	int code = attr & 0x7f;
	int color = (attr & 0x80) ? 1 : 0;

	SET_TILE_INFO(bank, code, color, 0);
}

VIDEO_START( abc802 )
{
	tx_tilemap_40 = tilemap_create(abc802_get_tile_info_40, tilemap_scan_rows, 
		TILEMAP_TYPE_PEN, 12, 10, 40, 24);

	tx_tilemap = tilemap_create(abc802_get_tile_info_80, tilemap_scan_rows, 
		TILEMAP_TYPE_PEN, 6, 10, 80, 24);

	abc802_columns = 80;
}

VIDEO_UPDATE( abc802 )
{
	if (abc802_columns == 40)
	{
		tilemap_mark_all_tiles_dirty(tx_tilemap_40);
		tilemap_draw(bitmap, cliprect, tx_tilemap_40, 0, 0);
	}
	else
	{
		tilemap_mark_all_tiles_dirty(tx_tilemap);
		tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 0);
	}

	return 0;
}

void abc802_set_columns(int columns)
{
	abc802_columns = columns;
}
