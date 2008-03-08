/*****************************************************************************
 *
 * video/abc80x.c
 *
 ****************************************************************************/

#include "driver.h"
#include "includes/abc80x.h"
#include "video/mc6845.h"

extern mc6845_t *abc800_mc6845;

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

static MC6845_UPDATE_ROW( abc800m_crtc6845_update_row )
{
	int col;

	for (col = 0; col < x_count; col++)
	{
		int bit;

		UINT8 *charrom = memory_region(REGION_GFX1);
		UINT16 address = (videoram[(ma + col) & 0x7ff] * 16) + ra;
		UINT8 data = charrom[address & 0x7ff];

		if (col == cursor_x)
		{
			data = ~data; // TODO this might be wrong
		}

		data <<= 2;

		for (bit = 0; bit < 6; bit++)
		{
			*BITMAP_ADDR16(bitmap, y, (col * 6) + bit) = (data & 0x80) ? 1 : 0;
			data <<= 1;
		}
	}
}

static VIDEO_START( abc800 )
{
}

static VIDEO_UPDATE( abc800 )
{
	mc6845_update(abc800_mc6845, bitmap, cliprect);

	return 0;
}

static const mc6845_interface abc800m_crtc6845_interface = {
	0,
	ABC800_X01/6,
	6,
	NULL,
	abc800m_crtc6845_update_row,
	NULL,
	NULL,
	NULL,
	NULL
};

MACHINE_DRIVER_START( abc800m_video )
	// device interface
	MDRV_DEVICE_ADD("crtc", MC6845)
	MDRV_DEVICE_CONFIG( abc800m_crtc6845_interface )

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
	MDRV_IMPORT_FROM(abc800m_video)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc806_video )
	MDRV_IMPORT_FROM(abc800m_video)
MACHINE_DRIVER_END
