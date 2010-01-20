/*****************************************************************************
 *
 * video/abc80.c
 *
 ****************************************************************************/

#include "emu.h"
#include "includes/abc80.h"

/* Graphics Layout */

static const gfx_layout charlayout =
{
	6, 10,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8
};

/* Graphics Decode Information */

static GFXDECODE_START( abc80 )
	GFXDECODE_ENTRY( "chargen", 0,	   charlayout, 0, 2 ) // normal characters
	GFXDECODE_ENTRY( "chargen", 0x500, charlayout, 0, 2 ) // graphics characters
GFXDECODE_END

static PALETTE_INIT( abc80 )
{
	palette_set_color(machine, 0, RGB_BLACK);
	palette_set_color(machine, 1, RGB_WHITE);
	palette_set_color(machine, 2, RGB_WHITE);
	palette_set_color(machine, 3, RGB_BLACK);
}

static TILE_GET_INFO( abc80_get_tile_info )
{
	abc80_state *state = (abc80_state *)machine->driver_data;

	int attr = state->video_ram[tile_index];
	int code = attr & 0x7f;
	int color = (state->blink && (attr & 0x80)) ? 1 : 0;
	int r = (tile_index & 0x78) >> 3;
	int l = (tile_index & 0x380) >> 7;
	int row = l + ((r / 5) * 8);

	if (row != state->char_row)
	{
		state->char_bank = 0;
		state->char_row = row;
	}

	if (code == ABC80_MODE_TEXT)
	{
		state->char_bank = 0;
	}

	if (code == ABC80_MODE_GFX)
	{
		state->char_bank = 1;
	}

	SET_TILE_INFO(state->char_bank, code, color, 0);
}

static UINT32 abc80_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return ((row & 0x07) << 7) + (row >> 3) * num_cols + col;
}

static TIMER_DEVICE_CALLBACK(abc80_blink_tick)
{
	abc80_state *state = (abc80_state *)timer->machine->driver_data;

	state->blink = !state->blink;
}

#ifdef UNUSED_FUNCTION
static void abc80_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	abc80_state *state = (abc80_state *)machine->driver_data;
	UINT16 videoram_addr;
	int y, sx, c = 0, r = 0;

	for (y = 0; y < 312; y++)
	{
		UINT8 vsync_data = state->vsync_prom[y];
		UINT8 l = state->line_prom[y];
		int dv = (vsync_data & ABC80_K2_DV) ? 1 : 0;
		int mode = 1;

		if (vsync_data & ABC80_K2_FRAME_END)
			r = 0;

		if (vsync_data & ABC80_K2_FRAME_START)
			r++;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 hsync_data = state->hsync_prom[sx];
			int dh = (hsync_data & ABC80_K5_DH) ? 1 : 0;
			UINT8 attr_addr;
			UINT8 videoram_data, attr_data;
			UINT8 data = 0;
			int cursor, blank, tecken, graf, versal;
			int bit;

			if (hsync_data & ABC80_K5_LINE_END)
				c = 0;

			if (hsync_data & ABC80_K5_ROW_START)
				c++;

			/*

                Video RAM Addressing Scheme

                A9 A8 A7 A6 A5 A4 A3 A2 A1 A0
                R2 R1 R0 xx xx xx xx C2 C1 C0

                A6 A5 A4 A3 = 00 C5 C4 C3 + R4 R3 R4 R3

            */

			videoram_addr = ((r & 0x07) << 7) | ((((c >> 3) & 0x07) + ((r >> 3) | (r >> 1))) & 0x0f) | (c & 0x07);
			videoram_data = state->video_ram[videoram_addr];
			attr_addr = ((dh & dv) << 7) & (data & 0x7f);
			attr_data = state->attr_prom[attr_addr];

			blank = (attr_data & ABC80_J3_BLANK) ? 1 : 0;
			tecken = (attr_data & ABC80_J3_TEXT) ? 1 : 0;
			graf = (attr_data & ABC80_J3_GRAPHICS) ? 1 : 0;
			versal = (attr_data & ABC80_J3_VERSAL) ? 1 : 0;
			cursor = (videoram_data & ABC80_CHAR_CURSOR) ? 1 : 0;

			if (tecken) mode = 1;
			if (graf) mode = 0;

			if (mode || versal)
			{
				/* text mode */

				UINT16 chargen_addr = ((videoram_data & 0x7f) * 10) + l;

				data = state->char_rom[chargen_addr];
			}
			else
			{
				/* graphics mode */

				int c0, c1, c2, c3, c4, c5;
				int r0 = 1, r1 = 1, r2 = 1;

				switch (l)
				{
				case 0: case 1: case 2:
					r0 = 0;
					break;
				case 3: case 4: case 5: case 6:
					r1 = 0;
					break;
				case 7: case 8: case 9:
					r2 = 0;
					break;
				}

				c0 = BIT(videoram_data, 0) | r2;
				c1 = BIT(videoram_data, 2) | r2;
				c2 = BIT(videoram_data, 3) | r1;
				c3 = BIT(videoram_data, 4) | r1;
				c4 = BIT(videoram_data, 5) | r0;
				c5 = BIT(videoram_data, 6) | r0;

				if (c0 & c2 & c4) data |= 0x38;
				if (c1 & c3 & c5) data |= 0x07;
			}

			data <<= 2;

			/* shift out pixels */
			for (bit = 0; bit < 6; bit++)
			{
				int color = BIT(data, 7);
				int x = (sx * 6) + bit;

				color ^= (cursor & state->blink);
				color &= blank;

				*BITMAP_ADDR16(bitmap, y, x) = color;

				data <<= 1;
			}
		}
	}
}
#endif

static VIDEO_START( abc80 )
{
	abc80_state *state = (abc80_state *)machine->driver_data;

	/* create tx_tilemap */

	state->tx_tilemap = tilemap_create(machine, abc80_get_tile_info, abc80_tilemap_scan, 6, 10, 40, 24);

	tilemap_set_scrolldx(state->tx_tilemap, ABC80_HDSTART, ABC80_HDSTART);
	tilemap_set_scrolldy(state->tx_tilemap, ABC80_VDSTART, ABC80_VDSTART);

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");
	state->hsync_prom = memory_region(machine, "hsync");
	state->vsync_prom = memory_region(machine, "vsync");
	state->line_prom = memory_region(machine, "line");
	state->attr_prom = memory_region(machine, "attr");

	/* register for state saving */

	state_save_register_global(machine, state->blink);
	state_save_register_global(machine, state->char_bank);
	state_save_register_global(machine, state->char_row);
}

static VIDEO_UPDATE( abc80 )
{
	abc80_state *state = (abc80_state *)screen->machine->driver_data;
	rectangle rect;

	rect.min_x = ABC80_HDSTART;
	rect.max_x = ABC80_HDSTART + 40 * 6 - 1;
	rect.min_y = ABC80_VDSTART;
	rect.max_y = ABC80_VDSTART + 240 - 1;

	bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine));

	tilemap_mark_all_tiles_dirty(state->tx_tilemap);
	tilemap_draw(bitmap, &rect, state->tx_tilemap, 0, 0);

	//abc80_update(screen->machine, bitmap, cliprect);

	return 0;
}

MACHINE_DRIVER_START( abc80_video )
	MDRV_TIMER_ADD_PERIODIC("blink", abc80_blink_tick, HZ(ABC80_XTAL/2/6/64/312/16))

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_GFXDECODE(abc80)
	MDRV_PALETTE_LENGTH(4)

	MDRV_PALETTE_INIT(abc80)
	MDRV_VIDEO_START(abc80)
	MDRV_VIDEO_UPDATE(abc80)

	MDRV_SCREEN_RAW_PARAMS(ABC80_XTAL/2, ABC80_HTOTAL, ABC80_HBEND, ABC80_HBSTART, ABC80_VTOTAL, ABC80_VBEND, ABC80_VBSTART)
MACHINE_DRIVER_END
