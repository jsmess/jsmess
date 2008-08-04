#include "driver.h"
#include "video/cdp1869.h"
#include "includes/tmc600.h"

#define SCREEN_TAG	"main"
#define CDP1869_TAG	"cdp1869"

#define TMC600_PAGERAM_SIZE	0x400
#define TMC600_PAGERAM_MASK	0x3ff

static int vismac_reg_latch;
static int vismac_color_latch;
static int vismac_bkg_latch;
static int vismac_blink;
static UINT8 *pageram, *vismac_colorram;

WRITE8_HANDLER( tmc600_vismac_register_w )
{
	vismac_reg_latch = data;
}

WRITE8_DEVICE_HANDLER( tmc600_vismac_data_w )
{
	switch (vismac_reg_latch)
	{
	case 0x20:
		// set character color
		vismac_color_latch = data;
		break;

	case 0x30:
		// write cdp1869 command on the data bus
		vismac_bkg_latch = data & 0x07;
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

static TIMER_CALLBACK( vismac_blink_int )
{
	vismac_blink = !vismac_blink;
}

static UINT8 tmc600_get_color(UINT16 pma)
{
	UINT8 color = vismac_colorram[pma];

	if (BIT(color, 3) && vismac_blink)
	{
		return vismac_bkg_latch;
	}
	else
	{
		return color;
	}
}

static CDP1869_PAGE_RAM_READ(tmc600_pageram_r)
{
	UINT16 addr = pma & TMC600_PAGERAM_MASK;

	return pageram[addr];
}

static CDP1869_PAGE_RAM_WRITE(tmc600_pageram_w)
{
	UINT16 addr = pma & TMC600_PAGERAM_MASK;

	pageram[addr] = data;
	vismac_colorram[addr] = vismac_color_latch;
}

static CDP1869_CHAR_RAM_READ(tmc600_charram_r)
{
	UINT16 pageaddr = pma & TMC600_PAGERAM_MASK;
	UINT8 column = pageram[pageaddr];
	UINT8 color = tmc600_get_color(pageaddr);
	UINT16 charaddr = ((cma & 0x08) << 8) | (column << 3) | (cma & 0x07);
	UINT8 *charrom = memory_region(device->machine, "chargen");
	UINT8 cdb = charrom[charaddr] & 0x3f;

	int ccb0 = BIT(color, 2);
	int ccb1 = BIT(color, 1);

	return (ccb1 << 7) | (ccb0 << 6) | cdb;
}

static CDP1869_PCB_READ(tmc600_pcb_r)
{
	UINT16 pageaddr = pma & TMC600_PAGERAM_MASK;
	UINT8 color = tmc600_get_color(pageaddr);

	int pcb = BIT(color, 0);

	return pcb;
}

static CDP1869_INTERFACE( tmc600_cdp1869_intf )
{
	SCREEN_TAG,
	CDP1869_DOT_CLK_PAL,
	CDP1869_COLOR_CLK_PAL,
	CDP1869_PAL,
	tmc600_pageram_r,
	tmc600_pageram_w,
	tmc600_pcb_r,
	tmc600_charram_r,
	NULL,
	NULL
};

static VIDEO_START( tmc600 )
{
	// allocate memory

	pageram = auto_malloc(TMC600_PAGERAM_SIZE);
	vismac_colorram = auto_malloc(TMC600_PAGERAM_SIZE);

	// register for save state

	state_save_register_global_pointer(pageram, TMC600_PAGERAM_SIZE);
	state_save_register_global_pointer(vismac_colorram, TMC600_PAGERAM_SIZE);

	state_save_register_global(vismac_reg_latch);
	state_save_register_global(vismac_color_latch);
	state_save_register_global(vismac_bkg_latch);
	state_save_register_global(vismac_blink);
}

static VIDEO_UPDATE( tmc600 )
{
	const device_config *cdp1869 = device_list_find_by_tag(screen->machine->config->devicelist, CDP1869_VIDEO, CDP1869_TAG);

	cdp1869_update(cdp1869, bitmap, cliprect);

	return 0;
}

MACHINE_DRIVER_START( tmc600_video )
	MDRV_TIMER_ADD_PERIODIC("blink", vismac_blink_int, HZ(2))

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(8+64)
	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(tmc600)
	MDRV_VIDEO_UPDATE(tmc600)
	MDRV_SCREEN_RAW_PARAMS(CDP1869_DOT_CLK_PAL, CDP1869_SCREEN_WIDTH, CDP1869_HBLANK_END, CDP1869_HBLANK_START, CDP1869_TOTAL_SCANLINES_PAL, CDP1869_SCANLINE_VBLANK_END_PAL, CDP1869_SCANLINE_VBLANK_START_PAL)

	MDRV_DEVICE_ADD(CDP1869_TAG, CDP1869_VIDEO)
	MDRV_DEVICE_CONFIG(tmc600_cdp1869_intf)
MACHINE_DRIVER_END
