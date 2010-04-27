#include "emu.h"
#include "includes/cgc7900.h"

static PALETTE_INIT( cgc7900 )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00 );
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff );
	palette_set_color_rgb(machine, 2, 0x00, 0xff, 0x00 );
	palette_set_color_rgb(machine, 3, 0x00, 0xff, 0xff );
	palette_set_color_rgb(machine, 4, 0xff, 0x00, 0x00 );
	palette_set_color_rgb(machine, 5, 0xff, 0x00, 0xff );
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00 );
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff );
}

static VIDEO_START( cgc7900 )
{
	cgc7900_state *state = (cgc7900_state *)machine->driver_data;

	/* find memory regions */
	state->char_rom = memory_region(machine, "gfx1");
}

static void update_clut(running_machine *machine)
{
	cgc7900_state *state = (cgc7900_state *)machine->driver_data;

	for (int i = 0; i < 256; i++)
	{
		UINT16 addr = i * 4;
		UINT32 data = (state->clut_ram[addr + 1] << 16) | state->clut_ram[addr];
		UINT8 b = data & 0xff;
		UINT8 g = (data >> 8) & 0xff;
		UINT8 r = (data >> 16) & 0xff;

//		logerror("CLUT %u : %08x\n", i, data);
		
		palette_set_color_rgb(machine, i + 8, r, g, b);
	}
}

#define OVERLAY_CUR		0x80000000	/* places a cursor in the cell if SET */
#define OVERLAY_BLK		0x40000000	/* blinks the foreground character in the cell if SET */
#define OVERLAY_VF		0x10000000	/* makes the foreground visible if SET (else transparent) */
#define OVERLAY_VB		0x08000000	/* makes the background visible if SET (else transparent) */
#define OVERLAY_PL		0x01000000	/* uses bits 0-7 as PLOT DOT descriptor if SET (else ASCII) */
#define OVERLAY_BR		0x00040000	/* turns on Red in background if SET */
#define OVERLAY_BG		0x00020000	/* turns on Green in background if SET */
#define OVERLAY_BB		0x00010000	/* turns on Blue in background if SET */
#define OVERLAY_FR		0x00000400	/* turns on Red in foreground if SET */
#define OVERLAY_FG		0x00000200	/* turns on Green in foreground if SET */
#define OVERLAY_FB		0x00000100	/* turns on Blue in background if SET */

static void draw_overlay(running_device *screen, bitmap_t *bitmap)
{
	cgc7900_state *state = (cgc7900_state *)screen->machine->driver_data;

	for (int y = 0; y < 768; y++)
	{
		int sy = y / 8;

		for (int sx = 0; sx < 85; sx++)
		{
			UINT16 addr = (sy * 85) + sx;
			UINT32 cell = (state->overlay_ram[addr + 1] << 16) | state->overlay_ram[addr];
			UINT8 data = state->char_rom[(cell & 0xff) << 3];
			int fg = (cell >> 8) & 0x07;
			int bg = (cell >> 16) & 0x07;

			for (int x = 0; x < 8; x++)
			{
				*BITMAP_ADDR16(bitmap, y, (sx * 8) + x) = BIT(data, 7) ? fg : bg;
				data <<= 1;
			}

			addr += 2;
		}
	}
}

static VIDEO_UPDATE( cgc7900 )
{
//	cgc7900_state *state = (cgc7900_state *)screen->machine->driver_data;

	update_clut(screen->machine);
	draw_overlay(screen, bitmap);

    return 0;
}

static const gfx_layout cgc7900_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ RGN_FRAC(0,1) },
	{ STEP8(7,-1) },
	{ STEP8(0,8) },
	8*8
};

static GFXDECODE_START( cgc7900 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, cgc7900_charlayout, 0, 1 )
GFXDECODE_END

MACHINE_DRIVER_START( cgc7900_video )
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(1024, 768)
    MDRV_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 768-1)
	
	MDRV_GFXDECODE(cgc7900)

	MDRV_PALETTE_LENGTH(8+256) /* 8 overlay colors + 256 bitmap colors */
	MDRV_PALETTE_INIT(cgc7900)

	MDRV_VIDEO_START(cgc7900)
    MDRV_VIDEO_UPDATE(cgc7900)
MACHINE_DRIVER_END
