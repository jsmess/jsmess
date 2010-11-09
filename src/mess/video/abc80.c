/*****************************************************************************
 *
 * video/abc80.c
 *
 ****************************************************************************/

#include "emu.h"
#include "includes/abc80.h"



//-------------------------------------------------
//  gfx_layout charlayout
//-------------------------------------------------

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


//-------------------------------------------------
//  GFXDECODE( abc80 )
//-------------------------------------------------

static GFXDECODE_START( abc80 )
	GFXDECODE_ENTRY( "chargen", 0,	   charlayout, 0, 2 ) // normal characters
	GFXDECODE_ENTRY( "chargen", 0x500, charlayout, 0, 2 ) // graphics characters
GFXDECODE_END


//-------------------------------------------------
//  PALETTE_INIT( abc80 )
//-------------------------------------------------

static PALETTE_INIT( abc80 )
{
	palette_set_color(machine, 0, RGB_BLACK);
	palette_set_color(machine, 1, RGB_WHITE);
	palette_set_color(machine, 2, RGB_WHITE);
	palette_set_color(machine, 3, RGB_BLACK);
}


//-------------------------------------------------
//  TILE_GET_INFO( abc80_get_tile_info )
//-------------------------------------------------

static TILE_GET_INFO( abc80_get_tile_info )
{
	abc80_state *state = machine->driver_data<abc80_state>();

	int attr = state->m_video_ram[tile_index];
	int code = attr & 0x7f;
	int color = (state->m_blink && (attr & 0x80)) ? 1 : 0;
	int r = (tile_index & 0x78) >> 3;
	int l = (tile_index & 0x380) >> 7;
	int row = l + ((r / 5) * 8);

	if (row != state->m_char_row)
	{
		state->m_char_bank = 0;
		state->m_char_row = row;
	}

	if (code == ABC80_MODE_TEXT)
	{
		state->m_char_bank = 0;
	}

	if (code == ABC80_MODE_GFX)
	{
		state->m_char_bank = 1;
	}

	SET_TILE_INFO(state->m_char_bank, code, color, 0);
}


//-------------------------------------------------
//  abc80_tilemap_scan - tilemap scanning
//-------------------------------------------------

static UINT32 abc80_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return ((row & 0x07) << 7) + (row >> 3) * num_cols + col;
}


//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( blink_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( blink_tick )
{
	abc80_state *state = timer.machine->driver_data<abc80_state>();

	state->m_blink = !state->m_blink;
}


//-------------------------------------------------
//  update_screen - 
//-------------------------------------------------

void abc80_state::update_screen(bitmap_t *bitmap, const rectangle *cliprect)
{
	UINT16 videoram_addr;
	int y, sx, c = 0, r = 0;

	for (y = 0; y < 312; y++)
	{
		UINT8 vsync_data = m_vsync_prom[y];
		UINT8 l = m_line_prom[y];
		int dv = (vsync_data & ABC80_K2_DV) ? 1 : 0;
		int mode = 1;

		if (vsync_data & ABC80_K2_FRAME_END)
			r = 0;

		if (vsync_data & ABC80_K2_FRAME_START)
			r++;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 hsync_data = m_hsync_prom[sx];
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
			videoram_data = m_video_ram[videoram_addr];
			attr_addr = ((dh & dv) << 7) & (data & 0x7f);
			attr_data = m_attr_prom[attr_addr];

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

				data = m_char_rom[chargen_addr];
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

				color ^= (cursor & m_blink);
				color &= blank;

				*BITMAP_ADDR16(bitmap, y, x) = color;

				data <<= 1;
			}
		}
	}
}


//-------------------------------------------------
//  VIDEO_START( abc80 )
//-------------------------------------------------

void abc80_state::video_start()
{
	/* create tx_tilemap */
	m_tx_tilemap = tilemap_create(machine, abc80_get_tile_info, abc80_tilemap_scan, 6, 10, 40, 24);

	tilemap_set_scrolldx(m_tx_tilemap, ABC80_HDSTART, ABC80_HDSTART);
	tilemap_set_scrolldy(m_tx_tilemap, ABC80_VDSTART, ABC80_VDSTART);

	/* find memory regions */
	m_char_rom = memory_region(machine, "chargen");
	m_hsync_prom = memory_region(machine, "hsync");
	m_vsync_prom = memory_region(machine, "vsync");
	m_line_prom = memory_region(machine, "line");
	m_attr_prom = memory_region(machine, "attr");

	/* register for state saving */
	state_save_register_global(machine, m_blink);
	state_save_register_global(machine, m_char_bank);
	state_save_register_global(machine, m_char_row);
}


//-------------------------------------------------
//  VIDEO_UPDATE( abc80 )
//-------------------------------------------------

bool abc80_state::video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	rectangle rect;

	rect.min_x = ABC80_HDSTART;
	rect.max_x = ABC80_HDSTART + 40 * 6 - 1;
	rect.min_y = ABC80_VDSTART;
	rect.max_y = ABC80_VDSTART + 240 - 1;

	bitmap_fill(&bitmap, &cliprect, get_black_pen(machine));

	tilemap_mark_all_tiles_dirty(m_tx_tilemap);
	tilemap_draw(&bitmap, &rect, m_tx_tilemap, 0, 0);

	//abc80_update(screen->machine, bitmap, cliprect);

	return 0;
}


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( abc80_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( abc80_video )
	MDRV_TIMER_ADD_PERIODIC("blink", blink_tick, HZ(ABC80_XTAL/2/6/64/312/16))

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_GFXDECODE(abc80)
	
	MDRV_PALETTE_LENGTH(4)
	MDRV_PALETTE_INIT(abc80)

	MDRV_SCREEN_RAW_PARAMS(ABC80_XTAL/2, ABC80_HTOTAL, ABC80_HBEND, ABC80_HBSTART, ABC80_VTOTAL, ABC80_VBEND, ABC80_VBSTART)
MACHINE_CONFIG_END
