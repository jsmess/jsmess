/*****************************************************************************
 *
 * video/abc800.c
 *
 ****************************************************************************/

/*

    TODO:

    - convert SAA5050 to C++ device and fix its memory interface
    - ABC800C video: http://www.qsl.net/zl1vfo/teletext.htm

*/

#include "includes/abc80x.h"



// these are needed because the MC6845 emulation does
// not position the active display area correctly
#define HORIZONTAL_PORCH_HACK	115
#define VERTICAL_PORCH_HACK		29



//**************************************************************************
//  HIGH RESOLUTION GRAPHICS
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
//  ABC 800 COLOR
//**************************************************************************

//-------------------------------------------------
//  abc800m_hr_update - color high resolution
//  screen update
//-------------------------------------------------

void abc800c_state::hr_update(bitmap_t *bitmap, const rectangle *cliprect)
{
	UINT16 addr = 0;
	int sx, y, dot;

	for (y = m_hrs; y < MIN(cliprect->max_y + 1, m_hrs + 240); y++)
	{
		int x = 0;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 data = m_video_ram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				UINT16 fgctl_addr = ((m_fgctl & 0x7f) << 2) | ((data >> 6) & 0x03);
				int color = m_fgctl_prom[fgctl_addr] & 0x07;

				*BITMAP_ADDR16(bitmap, y, x++) = color;

				data <<= 2;
			}
		}
	}
}


//-------------------------------------------------
//  VIDEO_START( abc800c )
//-------------------------------------------------

void abc800_state::video_start()
{
	// find memory regions
	m_char_rom = m_machine.region(MC6845_TAG)->base();
	m_fgctl_prom = m_machine.region("hru2")->base();

	// register for state saving
	state_save_register_global(m_machine, m_hrs);
	state_save_register_global(m_machine, m_fgctl);
}


//-------------------------------------------------
//  SCREEN_UPDATE( abc800c )
//-------------------------------------------------

bool abc800c_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	// clear screen
	bitmap_fill(&bitmap, &cliprect, get_black_pen(m_machine));

	// draw HR graphics
	hr_update(&bitmap, &cliprect);

	if (!BIT(m_fgctl, 7))
	{
		// draw text
		saa5050_update(m_trom, &bitmap, &cliprect);
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
	40, 24 - 1, 0x80,  // x, y, size
	0	// rev y order
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( abc800c_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( abc800c_video )
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(640, 400)
	MCFG_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MCFG_PALETTE_LENGTH(128)
	MCFG_PALETTE_INIT(saa5050)

	MCFG_GFXDECODE(saa5050)

	MCFG_SAA5050_ADD(SAA5052_TAG, trom_intf)
MACHINE_CONFIG_END



//**************************************************************************
//  ABC 800 MONOCHROME
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
//  screen update
//-------------------------------------------------

void abc800m_state::hr_update(bitmap_t *bitmap, const rectangle *cliprect)
{
	UINT16 addr = 0;
	int sx, y, dot;

	for (y = m_hrs + VERTICAL_PORCH_HACK; y < MIN(cliprect->max_y + 1, m_hrs + VERTICAL_PORCH_HACK + 240); y++)
	{
		int x = HORIZONTAL_PORCH_HACK;

		for (sx = 0; sx < 64; sx++)
		{
			UINT8 data = m_video_ram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				UINT16 fgctl_addr = ((m_fgctl & 0x7f) << 2) | ((data >> 6) & 0x03);
				int color = (m_fgctl_prom[fgctl_addr] & 0x07) ? 1 : 0;

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
	abc800m_state *state = device->machine().driver_data<abc800m_state>();

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
//  SCREEN_UPDATE( abc800m )
//-------------------------------------------------

bool abc800m_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	// HACK expand visible area to workaround MC6845
	screen.set_visible_area(0, 767, 0, 311);

	// clear screen
	bitmap_fill(&bitmap, &cliprect, get_black_pen(m_machine));

	// draw HR graphics
	hr_update(&bitmap, &cliprect);

	if (!BIT(m_fgctl, 7))
	{
		// draw text
		mc6845_update(m_crtc, &bitmap, &cliprect);
	}

	return 0;
}


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( abc800m_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( abc800m_video )
	MCFG_MC6845_ADD(MC6845_TAG, MC6845, ABC800_CCLK, crtc_intf)

	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(640, 400)
	MCFG_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(abc800m)
MACHINE_CONFIG_END
