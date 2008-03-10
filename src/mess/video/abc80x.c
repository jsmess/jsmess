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
	- abc800c character generator needs to be dumped!
	- abc800c row update
	- abc800c palette
	- abc806 video proms need to be dumped!
	- abc806 row update
	- abc806 palette
	- abc806 high resolution (HR)

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

/*

	ABC 806 Video
	*************

	74LS166 @ 8B
	------------

	A	<-	GND
	B	<-	GND
	C	<-	CHARGEN D0
	D	<-	CHARGEN D1
	E	<-	CHARGEN D2
	F	<-	CHARGEN D3
	G	<-	CHARGEN D4
	H	<-	CHARGEN D5

	SI	<-	GND

	CHARGEN @ 7C
	------------

	A0	<-	RAD Q1
	A1	<-	RAD Q2
	A2	<-	RAD Q3
	A3	<-	RAD Q4
	A4	<-	TX DATA 0
	A5	<-	TX DATA 1
	A6	<-	TX DATA 2
	A7	<-	TX DATA 3
	A8	<-	TX DATA 4
	A9	<-	TX DATA 5
	A10	<-	TX DATA 6
	A11	<-	TX DATA 7
	A12	<-	ATTHAND THP

	D0	->	74LS166 C
	D1	->	74LS166 D
	D2	->	74LS166 E
	D3	->	74LS166 F
	D4	->	74LS166 G
	D5	->	74LS166 H
	D6	->	N/C
	D7	->	N/C

	RAD @ 9B
	--------

	A0	<-	CRTC RA0
	A1	<-	CRTC RA1
	A2	<-	CRTC RA2
	A3	<-	CRTC RA3
	A4	<-	FLSHCLK
	A5	<-	ATTHAND ULP
	A6	<-	ATTHAND FLP
	A7	<-	ATTHAND E5P
	A8	<-	ATTHAND E6P

	Q1	->	CHARGEN A0
	Q2	->	CHARGEN A1
	Q3	->	CHARGEN A2
	Q4	->	CHARGEN A3

	CE	<-	CUR+4

	Notes:

	- lines Q1 thru Q4 are pulled high when cursor is enabled

	ATTHAND @ 11C
	-------------

	B0	<-	TX ATT 7
	B1	<-	TX ATT 6
	B2	<-	TX ATT 0
	B3	<-	TX ATT 1
	B4	<-	TX ATT 2
	B5	<-	TX ATT 3
	B6	<-	TX ATT 4
	B7	<-	TX ATT 5

	COND<-	40
	LP	<-	_DEN+3

	B5P	->	RTB
	B6P	->	GTB
	B7P	->	BTB
	F2P	->	RTF
	F3P	->	BTF
	F4P	->	GTF
	E5P	->	RAD A7
	E6P	->	RAD A8
	THP	->	CHARGEN A12
	ULP	->	RAD A5
	FLP	->	RAD A6

	74ALS256 @ 15G
	--------------

	Q0	->	EME
	Q1	->	40
	Q2	->	HRU11 A8, PROT A0
	Q3	->	PROT INI
	Q4	->	TXOFF
	Q5	->	E05-16 CS
	Q6	->	E05-16 CLK
	Q7	->	PROT DIN

	A	<-	D0
	B	<-	D1
	C	<-	D2
	D	<-	D7

	12G		7621		HRU11
	9B		7621/7643	RAD			Character generator row address handler
	11C		40033A		ATT.HAND	Text attribute handler
	2D		PAL16L8		ABC P4		HR
	1A		PAL16R4		ABC P3		Color mixer
	6E		7603		HRUI		HR
	7E		7621		V50			Video control

*/

#include "driver.h"
#include "includes/abc80x.h"
#include "video/mc6845.h"

#define ABC800_CCLK			ABC800_X01/6
#define ABC802_FLSHCLK		2

#define ABC800_CHAR_WIDTH	8

#define ABC802_AT0	0x01
#define ABC802_AT1	0x02
#define ABC802_ATD	0x40
#define ABC802_ATE	0x80
#define ABC802_INV	0x80

static emu_timer *abc802_flash_timer;

static int abc802_flshclk;
static int abc802_mux80_40;

extern mc6845_t *abc800_mc6845;

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

void abc802_mux80_40_w(int level)
{
	abc802_mux80_40 = level;
}

/* Timer Callbacks */

static TIMER_CALLBACK( abc802_flash_tick )
{
	abc802_flshclk = !abc802_flshclk;
}

/* MC6845 Row Update */

static MC6845_UPDATE_ROW( abc800m_update_row )
{
	int column;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT8 *charrom = memory_region(REGION_GFX1);
		UINT16 address = (videoram[(ma + column) & 0x7ff] << 4) | (ra & 0x0f);
		UINT8 data = charrom[address & 0x7ff] & 0x3f;

		if (column == cursor_x)
		{
			data = 0x3f;
		}

		for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
		{
			int x = (column * ABC800_CHAR_WIDTH) + bit;
			int color = BIT(data, 7);

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static MC6845_UPDATE_ROW( abc802_update_row )
{
	int column;

	if (!abc802_mux80_40)
	{
		x_count >>= 1;
		cursor_x >>= 1;
	}

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT8 *charrom = memory_region(REGION_GFX1);
		UINT8 code = videoram[(ma + column) & 0x7ff];
		UINT16 address = ((code & 0x80) << 5) | ((code & 0x7f) << 4);
		UINT8 ra_latch = ra;
		UINT8 data;

		if (column == cursor_x)
		{
			ra_latch = 0x0f;
		}

		data = charrom[(address + ra_latch) & 0x7ff];

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

				if ((abc802_flshclk && rf) || rc)
				{
					ra_latch = 0x0e;
				}
			}

			// reload data and mask out two bottom bits
			data = charrom[(address + ra_latch) & 0x7ff] & 0xfc;
		}

		if (abc802_mux80_40)
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
				int x = (column * ABC800_CHAR_WIDTH * 2) + (bit * 2);
				int color = BIT(data, 7) ^ BIT(code, 7);

				*BITMAP_ADDR16(bitmap, y, x) = color;
				*BITMAP_ADDR16(bitmap, y, x + 1) = color;

				data <<= 1;
			}
		}
	}
}

/* MC6845 Interfaces */

static const mc6845_interface abc800m_crtc6845_interface = {
	0,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc800m_update_row,
	NULL,
	NULL,
	NULL,
	NULL
};

static const mc6845_interface abc802_crtc6845_interface = {
	0,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc802_update_row,
	NULL,
	NULL,
	NULL,
	NULL
};

/* Video Start */

static VIDEO_START( abc800 )
{
}

static VIDEO_START( abc802 )
{
	state_save_register_global(abc802_flshclk);
	state_save_register_global(abc802_mux80_40);

	abc802_flash_timer = timer_alloc(abc802_flash_tick, NULL);
	timer_adjust_periodic(abc802_flash_timer, attotime_zero, 0, ATTOTIME_IN_HZ(ABC802_FLSHCLK));
}

/* Video Update */

static VIDEO_UPDATE( abc800 )
{
	mc6845_update(abc800_mc6845, bitmap, cliprect);

	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( abc800m_video )
	// device interface
	MDRV_DEVICE_ADD("crtc", MC6845)
	MDRV_DEVICE_CONFIG(abc800m_crtc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc800)
	MDRV_VIDEO_UPDATE(abc800)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc800c_video )
	MDRV_IMPORT_FROM(abc800m_video)
	MDRV_PALETTE_INIT(abc800c)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc802_video )
	// device interface
	MDRV_DEVICE_ADD("crtc", MC6845)
	MDRV_DEVICE_CONFIG(abc802_crtc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc802)
	MDRV_VIDEO_UPDATE(abc800)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc806_video )
	MDRV_IMPORT_FROM(abc800m_video)
MACHINE_DRIVER_END
