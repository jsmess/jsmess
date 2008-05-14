#include "driver.h"
#include "includes/comx35.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1869.h"
#include "video/mc6845.h"

#define COMX35_PAGERAM_SIZE 0x400
#define COMX35_CHARRAM_SIZE 0x800

#define COMX35_PAGERAM_MASK 0x3ff
#define COMX35_CHARRAM_MASK 0x7ff

/* CDP1869 */

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

/* MC6845 */

static MC6845_UPDATE_ROW( comx35_update_row )
{
	int column;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT8 *charrom = memory_region(REGION_GFX1);
		UINT8 code = memory_region(REGION_USER1)[((ma + column) & 0x7ff) + 0xd000];
		UINT8 addr = (code << 4) | ra;
		UINT8 data = charrom[addr & 0x7ff];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + (bit << 1);
			int color = BIT(code, 7);

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static MC6845_ON_VSYNC_CHANGED( comx35_vsync_changed )
{
	comx35_state *state = device->machine->driver_data;

	state->cdp1802_ef4 = vsync;
}

static const mc6845_interface comx35_mc6845_interface = {
	MC6845_SCREEN_TAG,
	XTAL_14_31818MHz,
	8,
	NULL,
	comx35_update_row,
	NULL,
	NULL,
	NULL,
	comx35_vsync_changed
};

static VIDEO_UPDATE( comx35_80 )
{
	const device_config *mc6845 = device_list_find_by_tag(screen->machine->config->devicelist, MC6845, MC6845_TAG);
	
	mc6845_update(mc6845, bitmap, cliprect);

	return 0;
}

/* Machine Drivers */

static MACHINE_DRIVER_START( comx35_80_video )
	MDRV_SCREEN_ADD(MC6845_SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 24*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 24*8-1)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_UPDATE(comx35_80)

	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(comx35_mc6845_interface)
MACHINE_DRIVER_END

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

	//MDRV_IMPORT_FROM(comx35_80_video)
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

	//MDRV_IMPORT_FROM(comx35_80_video)
MACHINE_DRIVER_END
