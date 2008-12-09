#include "driver.h"
#include "includes/osi.h"

/* Graphics Layouts */

static const gfx_layout osi600_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static const gfx_layout uk101_charlayout =
{
	8, 16,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8,
	  4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8 },
	8 * 8
};

/* Graphics Decode Information */

static GFXDECODE_START( osi600 )
	GFXDECODE_ENTRY( "chargen", 0, osi600_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( uk101 )
	GFXDECODE_ENTRY( "chargen", 0, uk101_charlayout, 0, 1 )
GFXDECODE_END

/* Tilemap */

static TILE_GET_INFO( get_bg_tile_info )
{
	osi_state *state = machine->driver_data;

	UINT8 code = state->video_ram[tile_index];

	SET_TILE_INFO(0, code, 0, 0);
}

/* Palette Initialization */

static PALETTE_INIT( osi630 )
{
}

/* Video Start */

static VIDEO_START( osi600 )
{
	osi_state *state = machine->driver_data;

	state->bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 8, 8, 32, 32);
}

static VIDEO_START( uk101 )
{
	osi_state *state = machine->driver_data;
	UINT16 addr;

	state->bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 8, 16, 64, 16);

	/* randomize video memory contents */
	for (addr = 0; addr < OSI600_VIDEORAM_SIZE; addr++)
	{
		state->video_ram[addr] = mame_rand(machine) & 0xff;
	}
}

/* Video Update */

static VIDEO_UPDATE( osi600 )
{
	osi_state *state = screen->machine->driver_data;

	tilemap_mark_all_tiles_dirty(state->bg_tilemap);
	tilemap_draw(bitmap, cliprect, state->bg_tilemap, 0, 0);

	return 0;
}

static VIDEO_UPDATE( uk101 )
{
	osi_state *state = screen->machine->driver_data;
/*
	tilemap_mark_all_tiles_dirty(state->bg_tilemap);
	tilemap_draw(bitmap, cliprect, state->bg_tilemap, 0, 0);
*/
	int y, bit, sx;

	for (y = 0; y < 256; y++)
	{
		UINT16 videoram_addr = (y >> 4) * 64;
		int line = (y >> 1) & 0x07;
		int x = 0;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 videoram_data = state->video_ram[videoram_addr++];
			UINT16 charrom_addr = ((videoram_data << 3) | line) & 0x7ff;
			UINT8 charrom_data = memory_region(screen->machine, "chargen")[charrom_addr];

			for (bit = 0; bit < 8; bit++)
			{
				*BITMAP_ADDR16(bitmap, y, x) = BIT(charrom_data, 7);
				x++;
				charrom_data <<= 1;
			}
		}
	}

	return 0;
}

/*	int y, bit, sx;

	for (y = 0; y < 256; y++)
	{
		UINT16 videoram_addr = (y >> 4) * 64;
		int line = (y >> 1) & 0x07;
		int x = 0;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 videoram_data = state->video_ram[videoram_addr++];
			UINT16 charrom_addr = ((videoram_data << 3) | line) & 0x7ff;
			UINT8 charrom_data = memory_region(screen->machine, "chargen")[charrom_addr];

			for (bit = 0; bit < 8; bit++)
			{
				*BITMAP_ADDR16(bitmap, y, x) = BIT(charrom_data, 7);
				x++;
				charrom_data <<= 1;
			}
		}
	}
*/

/* Machine Drivers */

MACHINE_DRIVER_START( osi600_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(X1/256/256) // 60 Hz
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(5*8, 29*8-1, 0, 28*8-1)

	MDRV_GFXDECODE(osi600)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(osi600)
	MDRV_VIDEO_UPDATE(osi600)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( uk101_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 16*16)
	MDRV_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 16*16-1)

	MDRV_GFXDECODE(uk101)
	
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(uk101)
	MDRV_VIDEO_UPDATE(uk101)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( osi630_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(X1/256/256) // 60 Hz
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(5*8, 29*8-1, 0, 28*8-1)

	MDRV_GFXDECODE(osi600)

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(osi630)

	MDRV_VIDEO_START(osi600)
	MDRV_VIDEO_UPDATE(osi600)
MACHINE_DRIVER_END
