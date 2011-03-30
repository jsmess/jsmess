#include "emu.h"
#include "rendlay.h"
#include "includes/comx35.h"
#include "cpu/cosmac/cosmac.h"
#include "sound/cdp1869.h"
#include "sound/wave.h"
#include "video/mc6845.h"

WRITE8_MEMBER( comx35_state::cdp1869_w )
{
	UINT16 ma = m_maincpu->get_memory_address();

	switch (offset + 3)
	{
	case 3:
		m_vis->out3_w(space, ma, data);
		break;

	case 4:
		m_vis->out4_w(space, ma, data);
		break;

	case 5:
		m_vis->out5_w(space, ma, data);
		break;

	case 6:
		m_vis->out6_w(space, ma, data);
		break;

	case 7:
		m_vis->out7_w(space, ma, data);
		break;
	}
}

/* CDP1869 */

static ADDRESS_MAP_START( cdp1869_page_ram, AS_0, 8 )
	AM_RANGE(0x000, 0x7ff) AM_RAM
ADDRESS_MAP_END

static CDP1869_CHAR_RAM_READ( comx35_charram_r )
{
	comx35_state *state = device->machine().driver_data<comx35_state>();

	UINT8 column = pmd & 0x7f;
	UINT16 charaddr = (column << 4) | cma;

	return state->m_charram[charaddr];
}

static CDP1869_CHAR_RAM_WRITE( comx35_charram_w )
{
	comx35_state *state = device->machine().driver_data<comx35_state>();

	UINT8 column = pmd & 0x7f;
	UINT16 charaddr = (column << 4) | cma;

	state->m_charram[charaddr] = data;
}

static CDP1869_PCB_READ( comx35_pcb_r )
{
	return BIT(pmd, 7);
}

static WRITE_LINE_DEVICE_HANDLER( comx35_prd_w )
{
	comx35_state *driver_state = device->machine().driver_data<comx35_state>();

	if (!driver_state->m_iden && !state)
	{
		device_set_input_line(driver_state->m_maincpu, COSMAC_INPUT_LINE_INT, ASSERT_LINE);
	}

	device_set_input_line(driver_state->m_maincpu, COSMAC_INPUT_LINE_EF1, state);
}

static CDP1869_INTERFACE( pal_cdp1869_intf )
{
	SCREEN_TAG,
	CDP1869_COLOR_CLK_PAL,
	CDP1869_PAL,
	comx35_pcb_r,
	comx35_charram_r,
	comx35_charram_w,
	DEVCB_LINE(comx35_prd_w)
};

static CDP1869_INTERFACE( ntsc_cdp1869_intf )
{
	SCREEN_TAG,
	CDP1869_COLOR_CLK_NTSC,
	CDP1869_NTSC,
	comx35_pcb_r,
	comx35_charram_r,
	comx35_charram_w,
	DEVCB_LINE(comx35_prd_w)
};

void comx35_state::video_start()
{
	// allocate memory
	m_charram = auto_alloc_array(m_machine, UINT8, COMX35_CHARRAM_SIZE);
	m_videoram = auto_alloc_array(m_machine, UINT8, COMX35_VIDEORAM_SIZE);

	// register for save state
	state_save_register_global_pointer(m_machine, m_charram, COMX35_CHARRAM_SIZE);
	state_save_register_global_pointer(m_machine, m_videoram, COMX35_VIDEORAM_SIZE);
}

bool comx35_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	if (screen.width() == CDP1869_SCREEN_WIDTH)
	{
		m_vis->update_screen(&bitmap, &cliprect);
	}
	else
	{
		mc6845_update(m_crtc, &bitmap, &cliprect);
	}

	return false;
}

/* MC6845 */

static MC6845_UPDATE_ROW( comx35_update_row )
{
	comx35_state *state = device->machine().driver_data<comx35_state>();

	UINT8 *charrom = device->machine().region("chargen")->base();

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT8 code = state->m_videoram[((ma + column) & 0x7ff)];
		UINT16 addr = (code << 3) | (ra & 0x07);
		UINT8 data = charrom[addr & 0x7ff];

		if (BIT(ra, 3) && column == cursor_x)
		{
			data = 0xff;
		}

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7) ? 7 : 0;

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( comx35_hsync_changed )
{
	comx35_state *driver_state = device->machine().driver_data<comx35_state>();

	driver_state->m_cdp1802_ef4 = state;
}

static const mc6845_interface comx35_mc6845_interface =
{
	MC6845_SCREEN_TAG,
	8,
	NULL,
	comx35_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(comx35_hsync_changed),
	DEVCB_NULL,
	NULL
};

/* F4 Character Displayer */
static const gfx_layout comx35_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( comx35 )
	GFXDECODE_ENTRY( "chargen", 0x0000, comx35_charlayout, 0, 36 )
GFXDECODE_END

/* Machine Drivers */

static MACHINE_CONFIG_FRAGMENT( comx35_80_video )
	MCFG_SCREEN_ADD(MC6845_SCREEN_TAG, RASTER)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(80*8, 24*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 24*8-1)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_REFRESH_RATE(50)

	MCFG_MC6845_ADD(MC6845_TAG, MC6845, XTAL_14_31818MHz, comx35_mc6845_interface)
MACHINE_CONFIG_END

MACHINE_CONFIG_FRAGMENT( comx35_pal_video )
	MCFG_DEFAULT_LAYOUT(layout_dualhsxs)

	MCFG_CDP1869_SCREEN_PAL_ADD(SCREEN_TAG, CDP1869_DOT_CLK_PAL)

	MCFG_GFXDECODE(comx35)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_CDP1869_ADD(CDP1869_TAG, CDP1869_DOT_CLK_PAL, pal_cdp1869_intf, cdp1869_page_ram)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_FRAGMENT_ADD(comx35_80_video)
MACHINE_CONFIG_END

MACHINE_CONFIG_FRAGMENT( comx35_ntsc_video )
	MCFG_DEFAULT_LAYOUT(layout_dualhsxs)

	MCFG_CDP1869_SCREEN_NTSC_ADD(SCREEN_TAG, CDP1869_DOT_CLK_NTSC)

	MCFG_GFXDECODE(comx35)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_CDP1869_ADD(CDP1869_TAG, CDP1869_DOT_CLK_NTSC, ntsc_cdp1869_intf, cdp1869_page_ram)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_FRAGMENT_ADD(comx35_80_video)
MACHINE_CONFIG_END
