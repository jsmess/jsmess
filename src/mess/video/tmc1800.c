#include "driver.h"
#include "includes/tmc1800.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1861.h"
#include "video/cdp1864.h"
#include "machine/rescap.h"

/* Telmac 1800 */

static CDP1861_ON_INT_CHANGED( tmc1800_int_w )
{
	cpu_set_input_line(device->machine->cpu[0], CDP1802_INPUT_LINE_INT, level);
}

static CDP1861_ON_DMAO_CHANGED( tmc1800_dmao_w )
{
	cpu_set_input_line(device->machine->cpu[0], CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1861_ON_EFX_CHANGED( tmc1800_efx_w )
{
	tmc1800_state *state = device->machine->driver_data;

	state->cdp1861_efx = level;
}

static CDP1861_INTERFACE( tmc1800_cdp1861_intf )
{
	SCREEN_TAG,
	tmc1800_int_w,
	tmc1800_dmao_w,
	tmc1800_efx_w
};

static VIDEO_UPDATE( tmc1800 )
{
	tmc1800_state *state = screen->machine->driver_data;

	cdp1861_update(state->cdp1861, bitmap, cliprect);

	return 0;
}

/* Telmac 2000 */

static CDP1864_ON_INT_CHANGED( tmc2000_int_w )
{
	cpu_set_input_line(device->machine->cpu[0], CDP1802_INPUT_LINE_INT, level);
}

static CDP1864_ON_DMAO_CHANGED( tmc2000_dmao_w )
{
	cpu_set_input_line(device->machine->cpu[0], CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1864_ON_EFX_CHANGED( tmc2000_efx_w )
{
	tmc2000_state *state = device->machine->driver_data;

	state->cdp1864_efx = level;
}

static CDP1864_INTERFACE( tmc2000_cdp1864_intf )
{
	SCREEN_TAG,
	CDP1864_INTERLACED,
	tmc2000_int_w,
	tmc2000_dmao_w,
	tmc2000_efx_w,
	RES_K(1.21),	// RL64
	RES_K(2.05),	// RL63
	RES_K(2.26),	// RL61
	RES_K(3.92)		// RL65 (also RH62 (2K pot) in series, but ignored here)
};

static VIDEO_UPDATE( tmc2000 )
{
	tmc2000_state *state = screen->machine->driver_data;

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

/* OSCOM Nano */

static CDP1864_ON_INT_CHANGED( oscnano_int_w )
{
	cpu_set_input_line(device->machine->cpu[0], CDP1802_INPUT_LINE_INT, level);
}

static CDP1864_ON_DMAO_CHANGED( oscnano_dmao_w )
{
	cpu_set_input_line(device->machine->cpu[0], CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1864_ON_EFX_CHANGED( oscnano_efx_w )
{
	oscnano_state *state = device->machine->driver_data;

	state->cdp1864_efx = level;
}

static CDP1864_INTERFACE( oscnano_cdp1864_intf )
{
	SCREEN_TAG,
	CDP1864_INTERLACED,
	oscnano_int_w,
	oscnano_dmao_w,
	oscnano_efx_w,
	RES_K(1.21), // R18 unconfirmed
	0, // not connected
	0, // not connected
	0  // not connected
};

static VIDEO_UPDATE( oscnano )
{
	oscnano_state *state = screen->machine->driver_data;

	cdp1864_update(state->cdp1864, bitmap, cliprect);

	return 0;
}

/* OSM-200 */

static VIDEO_START( osm200 )
{
}

static VIDEO_UPDATE( osm200 )
{
	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( tmc1800_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_1_75MHz, CDP1861_SCREEN_WIDTH, CDP1861_HBLANK_END, CDP1861_HBLANK_START, CDP1861_TOTAL_SCANLINES, CDP1861_SCANLINE_VBLANK_END, CDP1861_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(tmc1800)

	MDRV_CDP1861_ADD(CDP1861_TAG, XTAL_1_75MHz, tmc1800_cdp1861_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( osc1000b_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 199)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(osm200)
	MDRV_VIDEO_UPDATE(osm200)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( tmc2000_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_1_75MHz, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8+8)
	MDRV_VIDEO_UPDATE(tmc2000)

	MDRV_CDP1864_ADD(CDP1864_TAG, XTAL_1_75MHz, tmc2000_cdp1864_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( oscnano_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_1_75MHz, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8+8)
	MDRV_VIDEO_UPDATE(oscnano)

	MDRV_CDP1864_ADD(CDP1864_TAG, XTAL_1_75MHz, oscnano_cdp1864_intf)
MACHINE_DRIVER_END
