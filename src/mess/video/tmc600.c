#include "emu.h"
#include "sound/cdp1869.h"
#include "includes/tmc600.h"

WRITE8_MEMBER( tmc600_state::vismac_register_w )
{
	m_vismac_reg_latch = data;
}

WRITE8_MEMBER( tmc600_state::vismac_data_w )
{
	switch (m_vismac_reg_latch)
	{
	case 0x20:
		// set character color
		m_vismac_color_latch = data;
		break;

	case 0x30:
		// write cdp1869 command on the data bus
		m_vismac_bkg_latch = data & 0x07;
		cdp1869_out3_w(m_vis, 0, data);
		break;

	case 0x40:
		cdp1869_out4_w(m_vis, 0, data);
		break;

	case 0x50:
		cdp1869_out5_w(m_vis, 0, data);
		break;

	case 0x60:
		cdp1869_out6_w(m_vis, 0, data);
		break;

	case 0x70:
		cdp1869_out7_w(m_vis, 0, data);
		break;
	}
}

static TIMER_DEVICE_CALLBACK( blink_tick )
{
	tmc600_state *state = timer.machine->driver_data<tmc600_state>();

	state->m_blink = !state->m_blink;
}

UINT8 tmc600_state::get_color(UINT16 pma)
{
	UINT16 pageaddr = pma & TMC600_PAGE_RAM_MASK;
	UINT8 color = m_color_ram[pageaddr];

	if (BIT(color, 3) && m_blink)
	{
		color = m_vismac_bkg_latch;
	}

	return color;
}

static READ8_DEVICE_HANDLER( tmc600_page_ram_r )
{
	tmc600_state *state = device->machine->driver_data<tmc600_state>();

	UINT16 addr = offset & TMC600_PAGE_RAM_MASK;

	return state->m_page_ram[addr];
}

static WRITE8_DEVICE_HANDLER( tmc600_page_ram_w )
{
	tmc600_state *state = device->machine->driver_data<tmc600_state>();

	UINT16 addr = offset & TMC600_PAGE_RAM_MASK;

	state->m_page_ram[addr] = data;
	state->m_color_ram[addr] = state->m_vismac_color_latch;
}

static CDP1869_CHAR_RAM_READ( tmc600_char_ram_r )
{
	tmc600_state *state = device->machine->driver_data<tmc600_state>();

	UINT16 pageaddr = pma & TMC600_PAGE_RAM_MASK;
	UINT8 column = state->m_page_ram[pageaddr];
	UINT8 color = state->get_color(pageaddr);
	UINT16 charaddr = ((cma & 0x08) << 8) | (column << 3) | (cma & 0x07);
	UINT8 cdb = state->m_char_rom[charaddr] & 0x3f;

	int ccb0 = BIT(color, 2);
	int ccb1 = BIT(color, 1);

	return (ccb1 << 7) | (ccb0 << 6) | cdb;
}

static CDP1869_PCB_READ( tmc600_pcb_r )
{
	tmc600_state *state = device->machine->driver_data<tmc600_state>();

	UINT16 pageaddr = pma & TMC600_PAGE_RAM_MASK;
	UINT8 color = state->get_color(pageaddr);

	return BIT(color, 0);
}

static CDP1869_INTERFACE( tmc600_cdp1869_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1869_COLOR_CLK_PAL,
	CDP1869_PAL,
	DEVCB_HANDLER(tmc600_page_ram_r),
	DEVCB_HANDLER(tmc600_page_ram_w),
	tmc600_pcb_r,
	tmc600_char_ram_r,
	NULL,
	DEVCB_NULL
};

void tmc600_state::video_start()
{
	/* allocate memory */
	m_page_ram = auto_alloc_array(machine, UINT8, TMC600_PAGE_RAM_SIZE);
	m_color_ram = auto_alloc_array(machine, UINT8, TMC600_PAGE_RAM_SIZE);

	/* find memory regions */
	m_char_rom = memory_region(machine, "chargen");

	/* register for state saving */
	state_save_register_global_pointer(machine, m_page_ram, TMC600_PAGE_RAM_SIZE);
	state_save_register_global_pointer(machine, m_color_ram, TMC600_PAGE_RAM_SIZE);

	state_save_register_global(machine, m_vismac_reg_latch);
	state_save_register_global(machine, m_vismac_color_latch);
	state_save_register_global(machine, m_vismac_bkg_latch);
	state_save_register_global(machine, m_blink);
}

bool tmc600_state::video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	cdp1869_update(m_vis, &bitmap, &cliprect);

	return 0;
}

MACHINE_CONFIG_FRAGMENT( tmc600_video )
	MDRV_TIMER_ADD_PERIODIC("blink", blink_tick, HZ(2))

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(8+64)
	MDRV_PALETTE_INIT(cdp1869)
	MDRV_SCREEN_RAW_PARAMS(CDP1869_DOT_CLK_PAL, CDP1869_SCREEN_WIDTH, CDP1869_HBLANK_END, CDP1869_HBLANK_START, CDP1869_TOTAL_SCANLINES_PAL, CDP1869_SCANLINE_VBLANK_END_PAL, CDP1869_SCANLINE_VBLANK_START_PAL)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_CDP1869_ADD(CDP1869_TAG, CDP1869_DOT_CLK_PAL, tmc600_cdp1869_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END
