/*****************************************************************************
 *
 * video/abc802.c
 *
 ****************************************************************************/

#include "emu.h"
#include "includes/abc80x.h"
#include "machine/z80dart.h"
#include "video/mc6845.h"

/* Character Memory */

READ8_HANDLER( abc802_charram_r )
{
	abc802_state *state = (abc802_state *) space->machine->driver_data;

	return state->charram[offset];
}

WRITE8_HANDLER( abc802_charram_w )
{
	abc802_state *state = (abc802_state *) space->machine->driver_data;

	state->charram[offset] = data;
}

/* MC6845 Row Update */

static MC6845_UPDATE_ROW( abc802_update_row )
{
	/*

        PAL16R4 equation:

        IF (VCC)    *OS   = FC + RF / RC
                    *RG:  = HS / *RG + *ATE / *RG + ATD / *RG + LL /
                            *RG + AT1 / *RG + AT0 / ATE + *ATD + *LL +
                            *AT1 + *AT0
                    *RI:  = *RI + *INV / *RI + LL / *INV + *LL
                    *RF:  = HS / *RF + *ATE / *RF + ATD / *RF + LL /
                            *RF + AT1 / *RF + AT0 / ATE + *ATD + *LL +
                            *AT1 + AT0
                    *RC:  = HS / *RC + *ATE / *RC + *ATD / *RC + LL /
                            *RC + *ATI / *RC + AT0 / ATE + *LL + *AT1 +
                            *AT0
        IF (VCC)    *O0   = *CUR + *AT0 / *CUR + ATE
                    *O1   = *CUR + *AT1 / *CUR + ATE


        + = AND
        / = OR
        * = Inverted

        ATD     Attribute data
        ATE     Attribute enable
        AT0,AT1 Attribute address
        CUR     Cursor
        FC      FLSH clock
        HS      Horizontal sync
        INV     Inverted signal input
        LL      Load when Low
        OEL     Output Enable when Low
        RC      Row clear
        RF      Row flash
        RG      Row graphic
        RI      Row inverted

    */

	abc802_state *state = (abc802_state *) device->machine->driver_data;

	int column;
	int rf = 0, rc = 0, rg = 0;

	/* prevent wraparound */
	if (y >= 240) return;

	y += 29;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT8 code = state->charram[(ma + column) & 0x7ff];
		UINT16 address = code << 4;
		UINT8 ra_latch = ra;
		UINT8 data;

		int ri = (code & ABC802_INV) ? 1 : 0;

		if (column == cursor_x)
		{
			ra_latch = 0x0f;
		}

		if ((state->flshclk && rf) || rc)
		{
			ra_latch = 0x0e;
		}

		if (rg)
		{
			address |= 0x800;
		}

		data = state->char_rom[(address + ra_latch) & 0x1fff];

		if (data & ABC802_ATE)
		{
			int attr = data & 0x03;
			int value = (data & ABC802_ATD) ? 1 : 0;

			switch (attr)
			{
			case 0x00:
				/* Row Graphic */
				rg = value;
				break;

			case 0x01:
				/* Row Flash */
				rf = value;
				break;

			case 0x02:
				/* Row Clear */
				rc = value;
				break;

			case 0x03:
				/* undefined */
				break;
			}
		}
		else
		{
			data <<= 2;

			if (state->mux80_40)
			{
				for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
				{
					int x = 121 + ((column + 3) * ABC800_CHAR_WIDTH) + bit;
					int color = BIT(data, 7) ^ ri;

					*BITMAP_ADDR16(bitmap, y, x) = color;

					data <<= 1;
				}
			}
			else
			{
				for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
				{
					int x = 121 + ((column + 3) * ABC800_CHAR_WIDTH) + (bit << 1);
					int color = BIT(data, 7) ^ ri;

					*BITMAP_ADDR16(bitmap, y, x) = color;
					*BITMAP_ADDR16(bitmap, y, x + 1) = color;

					data <<= 1;
				}

				column++;
			}
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( abc802_vsync_changed )
{
	abc802_state *driver_state = (abc802_state *)device->machine->driver_data;

	if (!state)
	{
		/* flash clock */
		if (driver_state->flshclk_ctr & 0x20)
		{
			driver_state->flshclk = !driver_state->flshclk;
			driver_state->flshclk_ctr = 0;
		}
		else
		{
			driver_state->flshclk_ctr++;
		}
	}

	/* signal _DEW to DART */
	z80dart_rib_w(driver_state->z80dart, !state);
}

/* MC6845 Interfaces */

static const mc6845_interface abc802_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CHAR_WIDTH,
	NULL,
	abc802_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(abc802_vsync_changed),
	NULL
};

/* Video Start */

static VIDEO_START( abc802 )
{
	abc802_state *state = (abc802_state *) machine->driver_data;

	/* allocate memory */

	state->charram = auto_alloc_array(machine, UINT8, ABC802_CHAR_RAM_SIZE);

	/* find devices */

	state->mc6845 = devtag_get_device(machine, MC6845_TAG);

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");

	/* register for state saving */

	state_save_register_global_pointer(machine, state->charram, ABC802_CHAR_RAM_SIZE);

	state_save_register_global(machine, state->flshclk_ctr);
	state_save_register_global(machine, state->flshclk);
	state_save_register_global(machine, state->mux80_40);
}

/* Video Update */

static VIDEO_UPDATE( abc802 )
{
	abc802_state *state = (abc802_state *) screen->machine->driver_data;

	/* expand visible area to workaround MC6845 */
	screen->set_visible_area(0, 767, 0, 311);

	/* draw text */
	mc6845_update(state->mc6845, bitmap, cliprect);

	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( abc802_video )
	// device interface
	MDRV_MC6845_ADD(MC6845_TAG, MC6845, ABC800_CCLK, abc802_mc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(monochrome_amber)
	MDRV_VIDEO_START(abc802)
	MDRV_VIDEO_UPDATE(abc802)
MACHINE_DRIVER_END
