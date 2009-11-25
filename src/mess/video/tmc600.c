#include "driver.h"
#include "sound/cdp1869.h"
#include "includes/tmc600.h"

WRITE8_HANDLER( tmc600_vismac_register_w )
{
	tmc600_state *state = space->machine->driver_data;

	state->vismac_reg_latch = data;
}

WRITE8_DEVICE_HANDLER( tmc600_vismac_data_w )
{
	tmc600_state *state = device->machine->driver_data;

	switch (state->vismac_reg_latch)
	{
	case 0x20:
		// set character color
		state->vismac_color_latch = data;
		break;

	case 0x30:
		// write cdp1869 command on the data bus
		state->vismac_bkg_latch = data & 0x07;
		cdp1869_out3_w(device, 0, data);
		break;

	case 0x40:
		cdp1869_out4_w(device, 0, data);
		break;

	case 0x50:
		cdp1869_out5_w(device, 0, data);
		break;

	case 0x60:
		cdp1869_out6_w(device, 0, data);
		break;

	case 0x70:
		cdp1869_out7_w(device, 0, data);
		break;
	}
}

static TIMER_DEVICE_CALLBACK( blink_tick )
{
	tmc600_state *state = timer->machine->driver_data;

	state->blink = !state->blink;
}

static UINT8 tmc600_get_color(running_machine *machine, UINT16 pma)
{
	tmc600_state *state = machine->driver_data;

	UINT16 pageaddr = pma & TMC600_PAGE_RAM_MASK;
	UINT8 color = state->color_ram[pageaddr];

	if (BIT(color, 3) && state->blink)
	{
		color = state->vismac_bkg_latch;
	}

	return color;
}

static READ8_DEVICE_HANDLER( tmc600_page_ram_r )
{
	tmc600_state *state = device->machine->driver_data;

	UINT16 addr = offset & TMC600_PAGE_RAM_MASK;

	return state->page_ram[addr];
}

static WRITE8_DEVICE_HANDLER( tmc600_page_ram_w )
{
	tmc600_state *state = device->machine->driver_data;

	UINT16 addr = offset & TMC600_PAGE_RAM_MASK;

	state->page_ram[addr] = data;
	state->color_ram[addr] = state->vismac_color_latch;
}

static CDP1869_CHAR_RAM_READ( tmc600_char_ram_r )
{
	tmc600_state *state = device->machine->driver_data;

	UINT16 pageaddr = pma & TMC600_PAGE_RAM_MASK;
	UINT8 column = state->page_ram[pageaddr];
	UINT8 color = tmc600_get_color(device->machine, pageaddr);
	UINT16 charaddr = ((cma & 0x08) << 8) | (column << 3) | (cma & 0x07);
	UINT8 cdb = state->char_rom[charaddr] & 0x3f;

	int ccb0 = BIT(color, 2);
	int ccb1 = BIT(color, 1);

	return (ccb1 << 7) | (ccb0 << 6) | cdb;
}

static CDP1869_PCB_READ( tmc600_pcb_r )
{
	UINT16 pageaddr = pma & TMC600_PAGE_RAM_MASK;
	UINT8 color = tmc600_get_color(device->machine, pageaddr);

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

static VIDEO_START( tmc600 )
{
	tmc600_state *state = machine->driver_data;

	/* allocate memory */

	state->page_ram = auto_alloc_array(machine, UINT8, TMC600_PAGE_RAM_SIZE);
	state->color_ram = auto_alloc_array(machine, UINT8, TMC600_PAGE_RAM_SIZE);

	/* find devices */

	state->cdp1869 = devtag_get_device(machine, CDP1869_TAG);

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");

	/* register for state saving */

	state_save_register_global_pointer(machine, state->page_ram, TMC600_PAGE_RAM_SIZE);
	state_save_register_global_pointer(machine, state->color_ram, TMC600_PAGE_RAM_SIZE);

	state_save_register_global(machine, state->vismac_reg_latch);
	state_save_register_global(machine, state->vismac_color_latch);
	state_save_register_global(machine, state->vismac_bkg_latch);
	state_save_register_global(machine, state->blink);
}

static VIDEO_UPDATE( tmc600 )
{
	tmc600_state *state = screen->machine->driver_data;

	cdp1869_update(state->cdp1869, bitmap, cliprect);

	return 0;
}

MACHINE_DRIVER_START( tmc600_video )
	MDRV_TIMER_ADD_PERIODIC("blink", blink_tick, HZ(2))

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(8+64)
	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(tmc600)
	MDRV_VIDEO_UPDATE(tmc600)
	MDRV_SCREEN_RAW_PARAMS(CDP1869_DOT_CLK_PAL, CDP1869_SCREEN_WIDTH, CDP1869_HBLANK_END, CDP1869_HBLANK_START, CDP1869_TOTAL_SCANLINES_PAL, CDP1869_SCANLINE_VBLANK_END_PAL, CDP1869_SCANLINE_VBLANK_START_PAL)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_CDP1869_ADD(CDP1869_TAG, CDP1869_DOT_CLK_PAL, tmc600_cdp1869_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END
