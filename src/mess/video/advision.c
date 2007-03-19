/***************************************************************************

  vidhrdw/advision.c

  Routines to control the Adventurevision video hardware

  Video hardware is composed of a vertical array of 40 LEDs which is
  reflected off a spinning mirror.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/advision.h"

static UINT8 advision_led_latch[8];
static UINT8 *advision_display;

int advision_vh_hpos;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( advision )
{
    advision_vh_hpos = 0;
	advision_display = (UINT8 *)auto_malloc(8 * 8 * 256);
	memset(advision_display, 0, 8 * 8 * 256);
    return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

PALETTE_INIT( advision )
{
	int i;
	for( i = 0; i < 8; i++ )
	{
		/* 8 shades of RED */
		palette_set_color(machine, i, i * 0x22, 0x00, 0x00);
		colortable[i*2+0] = 0;
		colortable[i*2+0] = i;
	}

	palette_set_color(machine, 8, 0x55, 0x55, 0x55);	/* DK GREY - for MAME text only */
	palette_set_color(machine, 9, 0xf0, 0xf0, 0xf0);	/* LT GREY - for MAME text only */
}

void advision_vh_write(int data)
{
	if (advision_videobank >= 1 && advision_videobank <=5)
	{
		advision_led_latch[advision_videobank] = data;
	}
}

void advision_vh_update(int x)
{
    UINT8 *dst = &advision_display[x];
	int y;

	for( y = 0; y < 8; y++ )
	{
		UINT8 data = advision_led_latch[7-y];
		if( (data & 0x80) == 0 ) dst[0 * 256] = 8;
		if( (data & 0x40) == 0 ) dst[1 * 256] = 8;
		if( (data & 0x20) == 0 ) dst[2 * 256] = 8;
		if( (data & 0x10) == 0 ) dst[3 * 256] = 8;
		if( (data & 0x08) == 0 ) dst[4 * 256] = 8;
		if( (data & 0x04) == 0 ) dst[5 * 256] = 8;
		if( (data & 0x02) == 0 ) dst[6 * 256] = 8;
		if( (data & 0x01) == 0 ) dst[7 * 256] = 8;
		advision_led_latch[7-y] = 0xff;
		dst += 8 * 256;
    }
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/

VIDEO_UPDATE( advision )
{
	int x, y, bit;

    static int framecount = 0;

    if( (framecount++ % 8) == 0 )
	{
		advision_framestart = 1;
		advision_vh_hpos = 0;
    }

	for( x = (framecount%2)*128; x < (framecount%2)*128+128; x++ )
	{
		UINT8 *led = &advision_display[x];
		for( y = 0; y < 8; y++ )
		{
			for( bit = 0; bit < 8; bit++ )
			{
				if( *led > 0 )
					plot_pixel(bitmap, 85 + x, 30 + 2 *( y * 8 + bit), Machine->pens[--(*led)]);
				led += 256;
			}
		}
	}
	return 0;
}


