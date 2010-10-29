/*****************************************************************************
 *
 * video/abc800.c
 *
 ****************************************************************************/

/*

    TODO:

	- check compatibility with new MC6845
	- ABC800C video: http://www.qsl.net/zl1vfo/teletext.htm

*/

#include "emu.h"
#include "includes/abc80x.h"
#include "machine/z80dart.h"
#include "video/mc6845.h"
#include "video/saa5050.h"



// these are needed because the MC6845 emulation does
// not position the active display area correctly
#define HORIZONTAL_PORCH_HACK	115
#define VERTICAL_PORCH_HACK		29



//**************************************************************************
//	HIGH RESOLUTION GRAPHICS
//**************************************************************************

//-------------------------------------------------
//  abc800_hrs_w - high resolution scanline write
//-------------------------------------------------

WRITE8_MEMBER( abc800_state::hrs_w )
{
	m_hrs = data;
}


//-------------------------------------------------
//  abc800_hrc_w - high resolution color write
//-------------------------------------------------

WRITE8_MEMBER( abc800_state::hrc_w )
{
	m_fgctl = data;
}



//**************************************************************************
//	ABC 800 COLOR
//**************************************************************************

//-------------------------------------------------
//  PALETTE_INIT( abc800c )
//-------------------------------------------------

static PALETTE_INIT( abc800c )
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


//-------------------------------------------------
//  abc800m_hr_update - color high resolution
//	screen update
//-------------------------------------------------

static void abc800c_hr_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	abc800_state *state = machine->driver_data<abc800_state>();

	UINT16 addr = 0;
	int sx, y, dot;

	for (y = state->m_hrs; y < MIN(cliprect->max_y + 1, state->m_hrs + 240); y++)
	{
		int x = 0;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 data = state->m_video_ram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				UINT16 fgctl_addr = ((state->m_fgctl & 0x7f) << 2) | ((data >> 6) & 0x03);
				int color = state->fgctl_prom[fgctl_addr] & 0x07;

				*BITMAP_ADDR16(bitmap, y, x++) = color;

				data <<= 2;
			}
		}
	}
}


//-------------------------------------------------
//  VIDEO_START( abc800c )
//-------------------------------------------------

static VIDEO_START( abc800c )
{
	abc800_state *state = machine->driver_data<abc800_state>();

	// find memory regions
	state->m_char_rom = memory_region(machine, MC6845_TAG);
	state->fgctl_prom = memory_region(machine, "hru2");

	// register for state saving
	state_save_register_global(machine, state->m_hrs);
	state_save_register_global(machine, state->m_fgctl);
}


//-------------------------------------------------
//  VIDEO_UPDATE( abc800c )
//-------------------------------------------------

static VIDEO_UPDATE( abc800c )
{
	abc800_state *state = screen->machine->driver_data<abc800_state>();

	// clear screen
	bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine));

	// draw HR graphics
	abc800c_hr_update(screen->machine, bitmap, cliprect);

	if (!BIT(state->m_fgctl, 7))
	{
		// draw text
		saa5050_update(state->m_trom, bitmap, cliprect);
	}

	return 0;
}


//-------------------------------------------------
//  saa5050_interface trom_intf
//-------------------------------------------------

static const saa5050_interface trom_intf =
{
	SCREEN_TAG,
	0,	// starting gfxnum
	40, 24 - 1, 40,  // x, y, size
	0 	// rev y order
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( abc800c_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( abc800c_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(abc800c)

	MDRV_VIDEO_START(abc800c)
	MDRV_VIDEO_UPDATE(abc800c)

	MDRV_GFXDECODE(saa5050)

	MDRV_SAA5050_ADD(SAA5052_TAG, trom_intf)
MACHINE_CONFIG_END



//**************************************************************************
//	ABC 800 MONOCHROME
//**************************************************************************

//-------------------------------------------------
//  PALETTE_INIT( abc800m )
//-------------------------------------------------

static PALETTE_INIT( abc800m )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine, 1, 0xff, 0xff, 0x00); // yellow
}


//-------------------------------------------------
//  abc800m_hr_update - monochrome high resolution
//	screen update
//-------------------------------------------------

static void abc800m_hr_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	abc800_state *state = machine->driver_data<abc800_state>();

	UINT16 addr = 0;
	int sx, y, dot;

	for (y = state->m_hrs + VERTICAL_PORCH_HACK; y < MIN(cliprect->max_y + 1, state->m_hrs + VERTICAL_PORCH_HACK + 240); y++)
	{
		int x = HORIZONTAL_PORCH_HACK;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 data = state->m_video_ram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				UINT16 fgctl_addr = ((state->m_fgctl & 0x7f) << 2) | ((data >> 6) & 0x03);
				int color = (state->fgctl_prom[fgctl_addr] & 0x07) ? 1 : 0;

				*BITMAP_ADDR16(bitmap, y, x++) = color;
				*BITMAP_ADDR16(bitmap, y, x++) = color;

				data <<= 2;
			}
		}
	}
}


//-------------------------------------------------
//  MC6845_UPDATE_ROW( abc800m_update_row )
//-------------------------------------------------

static MC6845_UPDATE_ROW( abc800m_update_row )
{
	abc800_state *state = device->machine->driver_data<abc800_state>();

	int column;

	// prevent wraparound
	if (y >= 240) return;

	y += VERTICAL_PORCH_HACK;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT16 address = (state->m_char_ram[(ma + column) & 0x7ff] << 4) | (ra & 0x0f);
		UINT8 data = (state->m_char_rom[address & 0x7ff] & 0x3f);

		if (column == cursor_x)
		{
			data = 0x3f;
		}

		data <<= 2;

		for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
		{
			int x = HORIZONTAL_PORCH_HACK + (column * ABC800_CHAR_WIDTH) + bit;

			if (BIT(data, 7))
			{
				*BITMAP_ADDR16(bitmap, y, x) = 1;
			}

			data <<= 1;
		}
	}
}


//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	ABC800_CHAR_WIDTH,
	NULL,
	abc800m_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z80DART_TAG, z80dart_rib_w),
	NULL
};


//-------------------------------------------------
//  VIDEO_UPDATE( abc800m )
//-------------------------------------------------

static VIDEO_UPDATE( abc800m )
{
	abc800_state *state = screen->machine->driver_data<abc800_state>();

	// HACK expand visible area to workaround MC6845
	screen->set_visible_area(0, 767, 0, 311);

	// clear screen
	bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine));

	// draw HR graphics
	abc800m_hr_update(screen->machine, bitmap, cliprect);

	if (!BIT(state->m_fgctl, 7))
	{
		// draw text
		mc6845_update(state->m_crtc, bitmap, cliprect);
	}

	return 0;
}


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( abc800m_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( abc800m_video )
	MDRV_MC6845_ADD(MC6845_TAG, MC6845, ABC800_CCLK, crtc_intf)

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(abc800m)

	MDRV_VIDEO_START(abc800c)
	MDRV_VIDEO_UPDATE(abc800m)
MACHINE_CONFIG_END
