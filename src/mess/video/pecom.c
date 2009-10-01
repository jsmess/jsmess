/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "sound/cdp1869.h"
#include "sound/wave.h"
#include "cpu/cdp1802/cdp1802.h"
#include "includes/pecom.h"

static CDP1869_PAGE_RAM_READ( pecom_page_ram_r )
{
	pecom_state *state = device->machine->driver_data;

	UINT16 addr = pma & PECOM_PAGE_RAM_MASK;

	return state->page_ram[addr];
}

static CDP1869_PAGE_RAM_WRITE( pecom_page_ram_w )
{
	pecom_state *state = device->machine->driver_data;

	UINT16 addr = pma & PECOM_PAGE_RAM_MASK;

	state->page_ram[addr] = data;
}


static CDP1869_CHAR_RAM_READ( pecom_char_ram_r )
{
	pecom_state *state = device->machine->driver_data;

	UINT16 pageaddr = pma & PECOM_PAGE_RAM_MASK;
	UINT8 column = state->page_ram[pageaddr] & 0x7f;
	UINT16 charaddr = (column << 4) | cma;

	return state->charram[charaddr];
}

static CDP1869_CHAR_RAM_WRITE( pecom_char_ram_w )
{
	pecom_state *state = device->machine->driver_data;

	UINT16 pageaddr = pma & PECOM_PAGE_RAM_MASK;
	UINT8 column = state->page_ram[pageaddr] & 0x7f;
	UINT16 charaddr = (column << 4) | cma;

	state->charram[charaddr] = data;
}

static CDP1869_PCB_READ( pecom_pcb_r )
{
	pecom_state *state = device->machine->driver_data;

	UINT16 pageaddr = pma & PECOM_PAGE_RAM_MASK;

	return BIT(state->page_ram[pageaddr], 7);
}


static WRITE_LINE_DEVICE_HANDLER( pecom_prd_w )
{
	pecom_state *driver_state = device->machine->driver_data;
	// every other PRD triggers a DMAOUT request
	if (driver_state->dma)
	{
		driver_state->dma = 0;

		cputag_set_input_line(device->machine, "maincpu", CDP1802_INPUT_LINE_DMAOUT, HOLD_LINE);
	}
	else
	{
		driver_state->dma = 1;
	}
}

static CDP1869_INTERFACE( pecom_cdp1869_intf )
{
	"maincpu",
	SCREEN_TAG,
	CDP1869_COLOR_CLK_PAL,
	CDP1869_PAL,
	pecom_page_ram_r,
	pecom_page_ram_w,
	pecom_pcb_r,
	pecom_char_ram_r,
	pecom_char_ram_w,
	DEVCB_LINE(pecom_prd_w)
};



static VIDEO_START( pecom )
{
	pecom_state *state = machine->driver_data;

	/* allocate memory */

	state->page_ram = auto_alloc_array(machine, UINT8, PECOM_PAGE_RAM_SIZE);
	state->charram = auto_alloc_array(machine, UINT8, 0x800);

	/* find devices */

	state->cdp1869 = devtag_get_device(machine, CDP1869_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->cdp1802_mode);
	state_save_register_global(machine, state->dma);
	state_save_register_global_pointer(machine, state->page_ram, PECOM_PAGE_RAM_SIZE);
}

static VIDEO_UPDATE( pecom )
{
	pecom_state *state = screen->machine->driver_data;

	cdp1869_update(state->cdp1869, bitmap, cliprect);

	return 0;
}

MACHINE_DRIVER_START( pecom_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(8+64)
	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(pecom)
	MDRV_VIDEO_UPDATE(pecom)
	MDRV_SCREEN_RAW_PARAMS(CDP1869_DOT_CLK_PAL, CDP1869_SCREEN_WIDTH, CDP1869_HBLANK_END, CDP1869_HBLANK_START, CDP1869_TOTAL_SCANLINES_PAL, CDP1869_SCANLINE_VBLANK_END_PAL, CDP1869_SCANLINE_VBLANK_START_PAL)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_CDP1869_ADD(CDP1869_TAG, CDP1869_DOT_CLK_PAL, pecom_cdp1869_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END
