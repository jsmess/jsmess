#include "emu.h"
#include "rendlay.h"
#include "includes/comx35.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/cdp1869.h"
#include "sound/wave.h"
#include "video/mc6845.h"

/* CDP1869 */

static READ8_DEVICE_HANDLER( comx35_pageram_r )
{
	comx35_state *state = (comx35_state *)device->machine->driver_data;

	UINT16 addr = offset & COMX35_PAGERAM_MASK;

	return state->pageram[addr];
}

static WRITE8_DEVICE_HANDLER( comx35_pageram_w )
{
	comx35_state *state = (comx35_state *)device->machine->driver_data;

	UINT16 addr = offset & COMX35_PAGERAM_MASK;

	state->pageram[addr] = data;
}

static CDP1869_CHAR_RAM_READ( comx35_charram_r )
{
	comx35_state *state = (comx35_state *)device->machine->driver_data;

	UINT16 pageaddr = pma & COMX35_PAGERAM_MASK;
	UINT8 column = state->pageram[pageaddr] & 0x7f;
	UINT16 charaddr = (column << 4) | cma;

	return state->charram[charaddr];
}

static CDP1869_CHAR_RAM_WRITE( comx35_charram_w )
{
	comx35_state *state = (comx35_state *)device->machine->driver_data;

	UINT16 pageaddr = pma & COMX35_PAGERAM_MASK;
	UINT8 column = state->pageram[pageaddr] & 0x7f;
	UINT16 charaddr = (column << 4) | cma;

	state->charram[charaddr] = data;
}

static CDP1869_PCB_READ( comx35_pcb_r )
{
	comx35_state *state = (comx35_state *)device->machine->driver_data;

	UINT16 pageaddr = pma & COMX35_PAGERAM_MASK;

	return BIT(state->pageram[pageaddr], 7);
}

static WRITE_LINE_DEVICE_HANDLER( comx35_prd_w )
{
	comx35_state *driver_state = (comx35_state *)device->machine->driver_data;

	if (!driver_state->iden && !state)
	{
		cputag_set_input_line(device->machine, CDP1802_TAG, CDP1802_INPUT_LINE_INT, ASSERT_LINE);
	}

	driver_state->cdp1869_prd = state;
}

static CDP1869_INTERFACE( pal_cdp1869_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1869_COLOR_CLK_PAL,
	CDP1869_PAL,
	DEVCB_HANDLER(comx35_pageram_r),
	DEVCB_HANDLER(comx35_pageram_w),
	comx35_pcb_r,
	comx35_charram_r,
	comx35_charram_w,
	DEVCB_LINE(comx35_prd_w)
};

static CDP1869_INTERFACE( ntsc_cdp1869_intf )
{
	CDP1802_TAG,
	SCREEN_TAG,
	CDP1869_COLOR_CLK_NTSC,
	CDP1869_NTSC,
	DEVCB_HANDLER(comx35_pageram_r),
	DEVCB_HANDLER(comx35_pageram_w),
	comx35_pcb_r,
	comx35_charram_r,
	comx35_charram_w,
	DEVCB_LINE(comx35_prd_w)
};

static VIDEO_START( comx35 )
{
	comx35_state *state = (comx35_state *)machine->driver_data;

	// allocate memory

	state->pageram = auto_alloc_array(machine, UINT8, COMX35_PAGERAM_SIZE);
	state->charram = auto_alloc_array(machine, UINT8, COMX35_CHARRAM_SIZE);
	state->videoram = auto_alloc_array(machine, UINT8, COMX35_VIDEORAM_SIZE);

	// register for save state

	state_save_register_global(machine, state->pal_ntsc);
	state_save_register_global(machine, state->cdp1869_prd);

	state_save_register_global_pointer(machine, state->pageram, COMX35_PAGERAM_SIZE);
	state_save_register_global_pointer(machine, state->charram, COMX35_CHARRAM_SIZE);
	state_save_register_global_pointer(machine, state->videoram, COMX35_VIDEORAM_SIZE);
}

static VIDEO_UPDATE( comx35 )
{
	screen_device *screen_40 = downcast<screen_device *>(devtag_get_device(screen->machine, SCREEN_TAG));

	if (screen == screen_40)
	{
		running_device *cdp1869 = devtag_get_device(screen->machine, CDP1869_TAG);

		cdp1869_update(cdp1869, bitmap, cliprect);
	}
	else
	{
		running_device *mc6845 = devtag_get_device(screen->machine, MC6845_TAG);

		mc6845_update(mc6845, bitmap, cliprect);
	}

	return 0;
}

/* MC6845 */

READ8_HANDLER( comx35_videoram_r )
{
	comx35_state *state = (comx35_state *)space->machine->driver_data;

	return state->videoram[offset];
}

WRITE8_HANDLER( comx35_videoram_w )
{
	comx35_state *state = (comx35_state *)space->machine->driver_data;

	state->videoram[offset] = data;
}

static MC6845_UPDATE_ROW( comx35_update_row )
{
	comx35_state *state = (comx35_state *)device->machine->driver_data;

	UINT8 *charrom = memory_region(device->machine, "chargen");

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT8 code = state->videoram[((ma + column) & 0x7ff)];
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
	comx35_state *driver_state = (comx35_state *)device->machine->driver_data;

	driver_state->cdp1802_ef4 = state;
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

/* Machine Drivers */

static MACHINE_DRIVER_START( comx35_80_video )
	MDRV_SCREEN_ADD(MC6845_SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 24*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 24*8-1)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_MC6845_ADD(MC6845_TAG, MC6845, XTAL_14_31818MHz, comx35_mc6845_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( comx35_pal_video )
	MDRV_DEFAULT_LAYOUT(layout_dualhsxs)

	MDRV_CDP1869_SCREEN_PAL_ADD(SCREEN_TAG, CDP1869_DOT_CLK_PAL)

	MDRV_VIDEO_START(comx35)
	MDRV_VIDEO_UPDATE(comx35)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_CDP1869_ADD(CDP1869_TAG, CDP1869_DOT_CLK_PAL, pal_cdp1869_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_IMPORT_FROM(comx35_80_video)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( comx35_ntsc_video )
	MDRV_DEFAULT_LAYOUT(layout_dualhsxs)

	MDRV_CDP1869_SCREEN_NTSC_ADD(SCREEN_TAG, CDP1869_DOT_CLK_NTSC)

	MDRV_VIDEO_START(comx35)
	MDRV_VIDEO_UPDATE(comx35)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_CDP1869_ADD(CDP1869_TAG, CDP1869_DOT_CLK_NTSC, ntsc_cdp1869_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_IMPORT_FROM(comx35_80_video)
MACHINE_DRIVER_END
