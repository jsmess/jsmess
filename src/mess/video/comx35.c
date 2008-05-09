#include "driver.h"
#include "includes/comx35.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1869.h"

#define COMX35_PAGERAM_SIZE 0x400
#define COMX35_CHARRAM_SIZE 0x800

#define COMX35_PAGERAM_MASK 0x3ff
#define COMX35_CHARRAM_MASK 0x7ff

static CDP1869_PAGE_RAM_READ( comx35_pageram_r )
{
	comx35_state *state = device->machine->driver_data;

	UINT16 addr = pma & COMX35_PAGERAM_MASK;

	return state->pageram[addr];
}

static CDP1869_PAGE_RAM_WRITE( comx35_pageram_w )
{
	comx35_state *state = device->machine->driver_data;

	UINT16 addr = pma & COMX35_PAGERAM_MASK;

	state->pageram[addr] = data;
}

static CDP1869_CHAR_RAM_READ( comx35_charram_r )
{
	comx35_state *state = device->machine->driver_data;

	UINT16 pageaddr = pma & COMX35_PAGERAM_MASK;
	UINT8 column = state->pageram[pageaddr] & 0x7f;
	UINT16 charaddr = (cma << 7) | column;

	return state->charram[charaddr];
}

static CDP1869_CHAR_RAM_WRITE( comx35_charram_w )
{
	comx35_state *state = device->machine->driver_data;

	UINT16 pageaddr = pma & COMX35_PAGERAM_MASK;
	UINT8 column = state->pageram[pageaddr] & 0x7f;
	UINT16 charaddr = (cma << 7) | column;

	state->charram[charaddr] = data;
}

static CDP1869_PCB_READ( comx35_pcb_r )
{
	comx35_state *state = device->machine->driver_data;

	UINT16 pageaddr = pma & COMX35_PAGERAM_MASK;

	return BIT(state->pageram[pageaddr], 7);
}

static CDP1869_ON_PRD_CHANGED( comx35_prd_w )
{
	comx35_state *state = device->machine->driver_data;

	if (!state->iden && !prd)
	{
		cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_INT, ASSERT_LINE);
	}

	state->cdp1869_prd = prd;
}

static CDP1869_INTERFACE( comx35p_cdp1869_intf )
{
	SCREEN_TAG,
	CDP1869_DOT_CLK_PAL,
	CDP1869_COLOR_CLK_PAL,
	CDP1869_PAL,
	comx35_pageram_r,
	comx35_pageram_w,
	comx35_pcb_r,
	comx35_charram_r,
	comx35_charram_w,
	comx35_prd_w
};

static CDP1869_INTERFACE( comx35n_cdp1869_intf )
{
	SCREEN_TAG,
	CDP1869_DOT_CLK_NTSC,
	CDP1869_COLOR_CLK_NTSC,
	CDP1869_NTSC,
	comx35_pageram_r,
	comx35_pageram_w,
	comx35_pcb_r,
	comx35_charram_r,
	comx35_charram_w,
	comx35_prd_w
};

static VIDEO_START( comx35 )
{
	comx35_state *state = machine->driver_data;

	// allocate memory

	state->pageram = auto_malloc(COMX35_PAGERAM_SIZE);
	state->charram = auto_malloc(COMX35_CHARRAM_SIZE);

	// register for save state

	state_save_register_global_pointer(state->pageram, COMX35_PAGERAM_SIZE);
	state_save_register_global_pointer(state->charram, COMX35_CHARRAM_SIZE);
}

static VIDEO_UPDATE( comx35 )
{
	const device_config *cdp1869 = device_list_find_by_tag(screen->machine->config->devicelist, CDP1869_VIDEO, CDP1869_TAG);

	cdp1869_update(cdp1869, bitmap, cliprect);

	return 0;
}

MACHINE_DRIVER_START( comx35p_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(CDP1869_DOT_CLK_PAL, CDP1869_SCREEN_WIDTH, CDP1869_HBLANK_END, CDP1869_HBLANK_START, CDP1869_TOTAL_SCANLINES_PAL, CDP1869_SCANLINE_VBLANK_END_PAL, CDP1869_SCANLINE_VBLANK_START_PAL)

	MDRV_PALETTE_LENGTH(8+64)
	MDRV_PALETTE_INIT(cdp1869)

	MDRV_VIDEO_START(comx35)
	MDRV_VIDEO_UPDATE(comx35)

	MDRV_DEVICE_ADD(CDP1869_TAG, CDP1869_VIDEO)
	MDRV_DEVICE_CONFIG(comx35p_cdp1869_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( comx35n_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(CDP1869_DOT_CLK_NTSC, CDP1869_SCREEN_WIDTH, CDP1869_HBLANK_END, CDP1869_HBLANK_START, CDP1869_TOTAL_SCANLINES_NTSC, CDP1869_SCANLINE_VBLANK_END_NTSC, CDP1869_SCANLINE_VBLANK_START_NTSC)

	MDRV_PALETTE_LENGTH(8+64)
	MDRV_PALETTE_INIT(cdp1869)

	MDRV_VIDEO_START(comx35)
	MDRV_VIDEO_UPDATE(comx35)

	MDRV_DEVICE_ADD(CDP1869_TAG, CDP1869_VIDEO)
	MDRV_DEVICE_CONFIG(comx35n_cdp1869_intf)
MACHINE_DRIVER_END
