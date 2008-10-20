/*****************************************************************************
 *
 * video/abc80x.c
 *
 ****************************************************************************/

/*

	TODO:

	- add proper screen parameters to startup
	- abc802 delay CUR and DEN by 3 CCLK's
	- abc800m high resolution (HR)
	- abc800c row update
	- abc800c palette
	- abc800c high resolution (HR)

*/

/*

	ABC 802 Video
	*************

	74LS166 @ 8B
	------------

	A	<-	ATTHAND INV
	B	<-	ATTHAND INV
	C	<-	ATTHAND O0
	D	<-	ATTHAND O1
	E	<-	CHARGEN A2
	F	<-	CHARGEN A3
	G	<-	CHARGEN A4
	H	<-	CHARGEN A5

	SI	<-	ATTHAND RI

	CHARGEN @ 3G
	------------

	A0	<-	CRTC RA0
	A1	<-	CRTC RA1
	A2	<-	CRTC RA2
	A3	<-	CRTC RA3
	A4	<-	TX DATA 0
	A5	<-	TX DATA 1
	A6	<-	TX DATA 2
	A7	<-	TX DATA 3
	A8	<-	TX DATA 4
	A9	<-	TX DATA 5
	A10	<-	TX DATA 6
	A11	<-	ATTHAND RG
	A12	<-	TX DATA 7

	D0	->	ATTHAND AT0
	D1	->	ATTHAND AT1
	D2	->	74LS166 E
	D3	->	74LS166 F
	D4	->	74LS166 G
	D5	->	74LS166 H
	D6	->	ATTHAND ATD
	D7	->	ATTHAND ATE

	Notes:

	- lines A0 thru A3 are pulled high when cursor is enabled

	ATTHAND @ 2G
	------------

	AT0	<-	CHARGEN D0
	AT1	<-	CHARGEN D1
	ATD	<-	CHARGEN D6
	ATE	<-	CHARGEN D7
	CUR	<-	CUR-3
	FC	<-	FLSHCLK
	IHS	<-	HS
	LL	<-	74LS166 L

	O0	->	74LS166 C
	O1	->	74LS166 D
	RG	->	CHARGEN A11
	RI	->	74LS166 SI
	INV	->	74LS166 A, 74LS166 B
	O	->

	PAL equation:

	IF (VCC)	*OS	  =	FC + RF / RC
				*RG:  =	HS / *RG + *ATE / *RG + ATD / *RG + LL /
						*RG + AT1 / *RG + AT0 / ATE + *ATD + *LL +
						*AT1 + *AT0
				*RI:  =	*RI + *INV / *RI + LL / *INV + *LL
				*RF:  =	HS / *RF + *ATE / *RF + ATD / *RF + LL /
						*RF + AT1 / *RF + AT0 / ATE + *ATD + *LL +
						*AT1 + AT0
				*RC:  =	HS / *RC + *ATE / *RC + *ATD / *RC + LL /
						*RC + *ATI / *RC + AT0 / ATE + *LL + *AT1 +
						*AT0
	IF (VCC)	*O0	  =	*CUR + *AT0 / *CUR + ATE
				*O1   =	*CUR + *AT1 / *CUR + ATE


	ATD		Attribute data
	ATE		Attribute enable
	AT0,AT1	Attribute address
	CUR		Cursor
	FC		FLSH clock
	HS		Horizontal sync
	INV		Inverted signal input
	LL		Load when Low
	OEL		Output Enable when Low
	RC
	RF		Row flash
	RG		Row graphic
	RI		Row inverted

*/

#include "driver.h"
#include "includes/abc80x.h"
#include "machine/z80dart.h"
#include "video/mc6845.h"

/* Palette Initialization */

static PALETTE_INIT( abc800m )
{
	palette_set_color_rgb(machine,  0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine,  1, 0xff, 0xff, 0x00); // yellow (really white, but blue signal is disconnected from monitor)
}

static PALETTE_INIT( abc800c )
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

/* External Interface */

WRITE8_HANDLER( abc800m_hrs_w )
{
}

WRITE8_HANDLER( abc800m_hrc_w )
{
}

WRITE8_HANDLER( abc800c_hrs_w )
{
}

WRITE8_HANDLER( abc800c_hrc_w )
{
}

/* MC6845 Row Update */

static MC6845_UPDATE_ROW( abc800m_update_row )
{
	abc800_state *state = device->machine->driver_data;

	int column;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT16 address = (videoram[(ma + column) & 0x7ff] << 4) | (ra & 0x0f);
		UINT8 data = (state->char_rom[address & 0x7ff] & 0x3f);

		if (column == cursor_x)
		{
			data = 0x3f;
		}

		data <<= 2;

		for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
		{
			int x = (column * ABC800_CHAR_WIDTH) + bit;
			int color = BIT(data, 7);

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static MC6845_ON_VSYNC_CHANGED(abc800_vsync_changed)
{
	abc800_state *state = device->machine->driver_data;

	/* signal _DEW to DART */
	z80dart_set_ri(state->z80dart, 1, !vsync);
}

static MC6845_UPDATE_ROW( abc800c_update_row )
{
}

static MC6845_UPDATE_ROW( abc802_update_row )
{
	abc802_state *state = device->machine->driver_data;

	int column;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT8 code = videoram[(ma + column) & 0x7ff];
		UINT16 address = ((code & 0x80) << 5) | ((code & 0x7f) << 4);
		UINT8 ra_latch = ra;
		UINT8 data;

		if (column == cursor_x)
		{
			ra_latch = 0x0f;
		}

		data = state->char_rom[(address + ra_latch) & 0x1fff];

		if (data & ABC802_ATE)
		{
			int rf = 0, rc = 0;

			if (data & ABC802_ATD)
			{
				int attr = data & 0x03;

				switch (attr)
				{
				case 0x00:
					// RG = 1
					address |= 0x800;
					break;

				case 0x01:
					rf = 1;
					break;

				case 0x02:
					rc = 1;
					break;

				case 0x03:
					// undefined
					break;
				}

				if ((state->flshclk && rf) || rc)
				{
					ra_latch = 0x0e;
				}
			}

			// reload data and mask out two bottom bits
			data = state->char_rom[(address + ra_latch) & 0x1fff] & 0xfc;
		}
		
		data <<= 2;

		if (state->mux80_40)
		{
			for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
			{
				int x = (column * ABC800_CHAR_WIDTH) + bit;
				int color = BIT(data, 7) ^ BIT(code, 7);

				*BITMAP_ADDR16(bitmap, y, x) = color;

				data <<= 1;
			}
		}
		else
		{
			for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
			{
				int x = (column * ABC800_CHAR_WIDTH) + (bit << 1);
				int color = BIT(data, 7) ^ BIT(code, 7);

				*BITMAP_ADDR16(bitmap, y, x) = color;
				*BITMAP_ADDR16(bitmap, y, x + 1) = color;

				data <<= 1;
			}

			column++;
		}
	}
}

static MC6845_ON_VSYNC_CHANGED(abc802_vsync_changed)
{
	abc802_state *state = device->machine->driver_data;

	state->flshclk_ctr++;

	if (state->flshclk_ctr == 31)
	{
		state->flshclk = 1;
		state->flshclk_ctr = 0;
	}
	else
	{
		state->flshclk = 0;
	}

	z80dart_set_ri(state->z80dart, 1, vsync);
}

/* MC6845 Interfaces */

static const mc6845_interface abc800m_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc800m_update_row,
	NULL,
	NULL,
	NULL,
	abc800_vsync_changed
};

static const mc6845_interface abc800c_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc800c_update_row,
	NULL,
	NULL,
	NULL,
	abc800_vsync_changed
};

static const mc6845_interface abc802_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc802_update_row,
	NULL,
	NULL,
	NULL,
	abc802_vsync_changed
};

/* Video Start */

static VIDEO_START( abc800 )
{
	abc800_state *state = machine->driver_data;

	/* find devices */

	state->mc6845 = devtag_get_device(machine, MC6845, MC6845_TAG);

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");
}

static VIDEO_START( abc802 )
{
	abc802_state *state = machine->driver_data;

	/* find devices */

	state->mc6845 = devtag_get_device(machine, MC6845, MC6845_TAG);

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");

	/* register for state saving */

	state_save_register_global(state->flshclk_ctr);
	state_save_register_global(state->flshclk);
	state_save_register_global(state->mux80_40);
}

/* Video Update */

static VIDEO_UPDATE( abc800m )
{
	abc800_state *state = screen->machine->driver_data;
	
	mc6845_update(state->mc6845, bitmap, cliprect);
	
	return 0;
}

static VIDEO_UPDATE( abc800c )
{
	abc800_state *state = screen->machine->driver_data;
	
	mc6845_update(state->mc6845, bitmap, cliprect);
	
	return 0;
}

static VIDEO_UPDATE( abc802 )
{
	abc802_state *state = screen->machine->driver_data;
	
	mc6845_update(state->mc6845, bitmap, cliprect);
	
	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( abc800m_video )
	// device interface
	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(abc800m_mc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc800)
	MDRV_VIDEO_UPDATE(abc800m)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc800c_video )
	// device interface
	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(abc800c_mc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800c)
	MDRV_VIDEO_START(abc800)
	MDRV_VIDEO_UPDATE(abc800c)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc802_video )
	// device interface
	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(abc802_mc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc802)
	MDRV_VIDEO_UPDATE(abc802)
MACHINE_DRIVER_END
