#include "abc80.h"

static tilemap *tx_tilemap;
static emu_timer *abc80_blink_timer;
static int abc80_blink;
static int abc80_bank;
static int abc80_row;

static const unsigned short abc80_colortable[] =
{
	0x00, 0x01,
	0x01, 0x00,
};

PALETTE_INIT( abc80 )
{
	palette_set_color(machine, 0, RGB_BLACK);
	palette_set_color(machine, 1, RGB_WHITE);

	memcpy(colortable, abc80_colortable, sizeof(abc80_colortable));
}

static TILE_GET_INFO(abc80_get_tile_info)
{
	int attr = videoram[tile_index];
	int code = attr & 0x7f;
	int color = (abc80_blink && (attr & 0x80)) ? 1 : 0;
	int r = (tile_index & 0x78) >> 3;
	int l = (tile_index & 0x380) >> 7;
	int row = l + ((r / 5) * 8);

	if (row != abc80_row)
	{
		abc80_bank = 0;
		abc80_row = row;
	}
	
	if (code == ABC80_MODE_TEXT)
	{
		abc80_bank = 0;
	}

	if (code == ABC80_MODE_GFX)
	{
		abc80_bank = 1;
	}

	SET_TILE_INFO(abc80_bank, code, color, 0);
}

static UINT32 abc80_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return ((row & 0x07) << 7) + (row >> 3) * num_cols + col;
}

static TIMER_CALLBACK(abc80_blink_tick)
{
	abc80_blink = !abc80_blink;
}

VIDEO_START( abc80 )
{
	abc80_blink_timer = timer_alloc(abc80_blink_tick, NULL);
	timer_adjust(abc80_blink_timer, attotime_zero, 0, ATTOTIME_IN_HZ(ABC80_XTAL/2/6/64/312/16));

	tx_tilemap = tilemap_create(abc80_get_tile_info, abc80_tilemap_scan, 
		TILEMAP_TYPE_PEN, 6, 10, 40, 24);

	tilemap_set_scrolldx(tx_tilemap, ABC80_HDSTART, ABC80_HDSTART);
	tilemap_set_scrolldy(tx_tilemap, ABC80_VDSTART, ABC80_VDSTART);

	state_save_register_global(abc80_blink);
	state_save_register_global(abc80_bank);
	state_save_register_global(abc80_row);
}

VIDEO_UPDATE( abc80 )
{
	rectangle rect;

	rect.min_x = ABC80_HDSTART;
	rect.max_x = ABC80_HDSTART + 40 * 6 - 1;
	rect.min_y = ABC80_VDSTART;
	rect.max_y = ABC80_VDSTART + 240 - 1;

	fillbitmap(bitmap, get_black_pen(machine), cliprect);

	tilemap_mark_all_tiles_dirty(tx_tilemap);
	tilemap_draw(bitmap, &rect, tx_tilemap, 0, 0);

	return 0;
}
