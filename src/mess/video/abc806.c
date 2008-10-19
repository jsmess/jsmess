/*****************************************************************************
 *
 * video/abc806.c
 *
 ****************************************************************************/

/*

	TODO:

	- selection between 512x240, 256x240, and 240x240 HR modes
	- add proper screen parameters to startup
	- palette configuration from PAL/PROM
	- vertical sync delay in picture
	- horizontal positioning of the text and HR screens
	- protection device @ 16H
	
*/

#include "driver.h"
#include "includes/abc80x.h"
#include "machine/z80dart.h"
#include "machine/e0516.h"
#include "video/mc6845.h"

/* Palette Initialization */

static PALETTE_INIT( abc806 )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff); // blue
	palette_set_color_rgb(machine, 2, 0xff, 0x00, 0x00); // red
	palette_set_color_rgb(machine, 3, 0xff, 0x00, 0xff); // magenta
	palette_set_color_rgb(machine, 4, 0x00, 0xff, 0x00); // green
	palette_set_color_rgb(machine, 5, 0x00, 0xff, 0xff); // cyan
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00); // yellow
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff); // white
}

/* High Resolution Screen Select */

WRITE8_HANDLER( abc806_hrs_w )
{
	/*

		bit		signal	description

		0		VM14	visible screen memory area bit 0
		1		VM15	visible screen memory area bit 1
		2		VM16	visible screen memory area bit 2 (unused)
		3		VM17	visible screen memory area bit 3 (unused)
		4		F14		cpu accessible screen memory area bit 0
		5		F15		cpu accessible screen memory area bit 1
		6		F16		cpu accessible screen memory area bit 2 (unused)
		7		F17		cpu accessible screen memory area bit 3 (unused)

	*/

	abc806_state *state = machine->driver_data;
	
	state->hrs = data;
}

/* High Resolution Palette */

WRITE8_HANDLER( abc806_hrc_w )
{
	abc806_state *state = machine->driver_data;

	int reg = (offset >> 8) & 0x0f;

	state->hrc[reg] = data & 0x0f;
}

/* Character Memory */

READ8_HANDLER( abc806_charram_r )
{
	abc806_state *state = machine->driver_data;

	state->attr_data = state->colorram[offset];

	return state->charram[offset];
}

WRITE8_HANDLER( abc806_charram_w )
{
	abc806_state *state = machine->driver_data;

	state->colorram[offset] = state->attr_data;
	state->charram[offset] = data;
}

/* Attribute Memory */

READ8_HANDLER( abc806_ami_r )
{
	abc806_state *state = machine->driver_data;

	return state->attr_data;
}

WRITE8_HANDLER( abc806_amo_w )
{
	abc806_state *state = machine->driver_data;

	state->attr_data = data;
}

/* High Resolution Palette / Real-Time Clock */

READ8_HANDLER( abc806_cli_r )
{
	abc806_state *state = machine->driver_data;

	/*

		bit		description

		0		HRU II data bit 0
		1		HRU II data bit 1
		2		HRU II data bit 2
		3		HRU II data bit 3
		4		
		5		
		6		
		7		RTC data output

	*/

	UINT16 hru2_addr = (state->hru2_a8 << 8) | (offset >> 8);
	UINT8 data = state->hru2_prom[hru2_addr] & 0x0f;

	data |= e0516_dio_r(state->e0516) << 7;

	return data;
}

/* Special */

READ8_HANDLER( abc806_sti_r )
{
	/* this is some weird device marked PROT @ 16H */

	/*

		bit		description

		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		PROT DOUT

	*/

	return 0;
}

WRITE8_HANDLER( abc806_sto_w )
{
	abc806_state *state = machine->driver_data;

	int level = BIT(data, 7);

	switch (data & 0x07)
	{
	case 0:
		/* external memory enable */
		state->eme = level;
		break;
	case 1:
		/* 40/80 column display */
		state->_40 = level;
		break;
	case 2:
		/* HRU II address line 8, PROT A0 */
		state->hru2_a8 = level;
		break;
	case 3:
		/* PROT INI */
		break;
	case 4:
		/* text display enable */
		state->txoff = level;
		break;
	case 5:
		/* RTC chip select */
		e0516_cs_w(state->e0516, !level);
		break;
	case 6:
		/* RTC clock */
		e0516_clk_w(state->e0516, level);
		break;
	case 7:
		/* RTC data in, PROT DIN */
		e0516_dio_w(state->e0516, level);
		break;
	}
}

/* Sync Delay */

WRITE8_HANDLER( abc806_sso_w )
{
	abc806_state *state = machine->driver_data;

	state->sync = data & 0x3f;
}

/* MC6845 */

static MC6845_UPDATE_ROW( abc806_update_row )
{
	abc806_state *state = device->machine->driver_data;

	int column;

	UINT8 old_data = 0xff;
	int fg_color = 7;
	int bg_color = 0;
	int underline = 0;
	int flash = 0;
	int e5 = 0;
	int e6 = 0;
	int th = 0;

	if (state->_40)
	{
		e5 = 1;
		e6 = 1;
	}

	for (column = 0; column < x_count; column++)
	{
		UINT8 data = state->charram[(ma + column) & 0x7ff];
		UINT8 attr = state->colorram[(ma + column) & 0x7ff];
		UINT16 rad_addr, chargen_addr;
		UINT8 rad_data, chargen_data;
		int bit, x;

		if ((attr & 0x07) == ((attr >> 3) & 0x07))
		{
			/* special case */

			switch (attr >> 6)
			{
			case 0:
				/* use previously selected attributes */
				break;
			case 1:
				/* reserved for future use */
				break;
			case 2:
				/* blank */
				fg_color = 0;
				bg_color = 0;
				underline = 0;
				flash = 0;
				break;
			case 3:
				/* double width */
				e5 = BIT(attr, 0);
				e6 = BIT(attr, 1);

				/* read attributes from next byte */
				attr = state->colorram[(ma + column + 1) & 0x7ff];

				if (attr != 0x00)
				{
					fg_color = attr & 0x07;
					bg_color = (attr >> 3) & 0x07;
					underline = BIT(attr, 6);
					flash = BIT(attr, 7);
				}
				break;
			}
		}
		else
		{
			/* normal case */

			fg_color = attr & 0x07;
			bg_color = (attr >> 3) & 0x07;
			underline = BIT(attr, 6);
			flash = BIT(attr, 7);
			e5 = state->_40 ? 1 : 0;
			e6 = state->_40 ? 1 : 0;
		}

		if (column == cursor_x)
		{
			rad_data = 0x0f;
		}
		else
		{
			rad_addr = (e6 << 8) | (e5 << 7) | (flash << 6) | (underline << 5) | (state->flshclk << 4) | ra;
			rad_data = state->rad_prom[rad_addr] & 0x0f;

			rad_data = ra; // HACK because the RAD prom is not dumped yet
		}

		chargen_addr = (th << 12) | (data << 4) | rad_data;
		chargen_data = state->char_rom[chargen_addr & 0xfff] << 2;
		x = column * ABC800_CHAR_WIDTH;

		for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
		{
			int color = BIT(chargen_data, 7) ? fg_color : bg_color;

			if (state->txoff)
			{
				color = 0;
			}

			*BITMAP_ADDR16(bitmap, y, x++) = color;

			if (e5 || e6)
			{
				*BITMAP_ADDR16(bitmap, y, x++) = color;
			}

			chargen_data <<= 1;
		}
		
		if (e5 || e6)
		{
			column++;
		}

		old_data = data;
	}
}

static MC6845_ON_HSYNC_CHANGED(abc806_hsync_changed)
{
	abc806_state *state = device->machine->driver_data;

	int vsync;

	if (!hsync)
	{
		state->v50_addr++;

		/* clock current vsync value into the shift register */
		state->vsync_shift <<= 1;
		state->vsync_shift = (state->vsync_shift & 0xfffffffffffffffell) | state->vsync;

		vsync = BIT(state->vsync_shift, state->sync);

		if (vsync)
		{
			/* clear V50 address */
			state->v50_addr = 0;
		}
		else
		{
			/* flash clock */
			if (state->flshclk_ctr == 31)
			{
				state->flshclk = 1;
				state->flshclk_ctr = 0;
			}
			else
			{
				state->flshclk = 0;
				state->flshclk_ctr++;
			}
		}

		/* _DEW signal to DART */
		z80dart_set_ri(state->z80dart, 1, vsync);
	}
}

static MC6845_ON_VSYNC_CHANGED(abc806_vsync_changed)
{
	abc806_state *state = device->machine->driver_data;

	state->vsync = vsync;
}

/* MC6845 Interfaces */

static const mc6845_interface abc806_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc806_update_row,
	NULL,
	NULL,
	abc806_hsync_changed,
	abc806_vsync_changed
};

/* HR */

static void abc806_hr_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	abc806_state *state = machine->driver_data;

	UINT16 addr = (state->hrs & 0x03) << 14;
	int sx, y, dot;

	for (y = 0; y < 240; y++)
	{
		/* 240x240 */
		for (sx = 0; sx < 60; sx++)
		{
			UINT16 data = (state->videoram[addr++] << 8) | state->videoram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				int color = state->hrc[(data >> 12) & 0x0f]; // TODO get colors from HRU II prom
				int x = (sx << 3) | (dot << 1);

				*BITMAP_ADDR16(bitmap, y, x) = color;
				*BITMAP_ADDR16(bitmap, y, x + 1) = color;

				data <<= 4;
			}
		}

		/* 256x240 */
		for (sx = 0; sx < 64; sx++)
		{
			UINT16 data = (state->videoram[addr++] << 8) | state->videoram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				int color = state->hrc[(data >> 12) & 0x0f];
				int x = (sx << 3) | (dot << 1);

				*BITMAP_ADDR16(bitmap, y, x) = color;
				*BITMAP_ADDR16(bitmap, y, x + 1) = color;

				data <<= 4;
			}
		}

		/* 512x240 */
		for (sx = 0; sx < 64; sx++)
		{
			UINT16 data = (state->videoram[addr++] << 8) | state->videoram[addr++];

			for (dot = 0; dot < 8; dot++)
			{
				int color = (data >> 14) & 0x03;
				int x = (sx << 3) | dot;

				*BITMAP_ADDR16(bitmap, y, x) = color;

				data <<= 4;
			}
		}
	}
}

/* Video Start */

static VIDEO_START(abc806)
{
	abc806_state *state = machine->driver_data;
	
	int i;

	/* initialize variables */

	for (i = 0; i < 16; i++)
	{
		state->hrc[i] = 0;
	}

	state->sync = 10;

	/* find devices */

	state->mc6845 = devtag_get_device(machine, MC6845, MC6845_TAG);

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");
	state->rad_prom = memory_region(machine, "rad");
	state->hru2_prom = memory_region(machine, "hru2");

	/* allocate memory */

	state->charram = auto_malloc(ABC806_CHAR_RAM_SIZE);
	state->colorram = auto_malloc(ABC806_ATTR_RAM_SIZE);

	/* register for state saving */

	state_save_register_global_pointer(state->charram, ABC806_CHAR_RAM_SIZE);
	state_save_register_global_pointer(state->colorram, ABC806_ATTR_RAM_SIZE);
	state_save_register_global_pointer(state->videoram, ABC806_VIDEO_RAM_SIZE);

	state_save_register_global(state->v50_addr);
	state_save_register_global(state->attr_data);
	state_save_register_global(state->hrs);
	state_save_register_global(state->sync);
	state_save_register_global_array(state->hrc);
	state_save_register_global(state->eme);
	state_save_register_global(state->txoff);
	state_save_register_global(state->_40);
	state_save_register_global(state->flshclk_ctr);
	state_save_register_global(state->flshclk);
	state_save_register_global(state->hru2_a8);
	state_save_register_global(state->vsync_shift);
	state_save_register_global(state->vsync);
}

/* Video Update */

static VIDEO_UPDATE( abc806 )
{
	abc806_state *state = screen->machine->driver_data;
	
	abc806_hr_update(screen->machine, bitmap, cliprect);
	mc6845_update(state->mc6845, bitmap, cliprect);
	
	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( abc806_video )
	/* device interface */
	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(abc806_mc6845_interface)

	/* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(abc806)
	MDRV_VIDEO_START(abc806)
	MDRV_VIDEO_UPDATE(abc806)
MACHINE_DRIVER_END
