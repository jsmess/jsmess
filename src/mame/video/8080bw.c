/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "8080bw.h"

static int screen_red;
static int screen_red_enabled;		/* 1 for games that can turn the screen red */
static int color_map_select;
static int background_color;
static int schaser_sx10_done;
static UINT8 cloud_pos;

static write8_handler videoram_w_p;

static WRITE8_HANDLER( bw_videoram_w );
static WRITE8_HANDLER( rollingc_videoram_w );
static WRITE8_HANDLER( schaser_videoram_w );
static WRITE8_HANDLER( lupin3_videoram_w );
static WRITE8_HANDLER( polaris_videoram_w );
static WRITE8_HANDLER( sstrngr2_videoram_w );
static WRITE8_HANDLER( invadpt2_videoram_w );
static WRITE8_HANDLER( cosmo_videoram_w );
static WRITE8_HANDLER( shuttlei_videoram_w );

static void plot_pixel_8080(int x, int y, int col);

/* smoothed colors, overlays are not so contrasted */

/*
#define OVERLAY_RED         MAKE_ARGB(0x08,0xff,0x20,0x20)
#define OVERLAY_GREEN       MAKE_ARGB(0x08,0x20,0xff,0x20)
#define OVERLAY_YELLOW      MAKE_ARGB(0x08,0xff,0xff,0x20)

OVERLAY_START( invdpt2m_overlay )
    OVERLAY_RECT(  16,   0,  72, 224, OVERLAY_GREEN )
    OVERLAY_RECT(   0,  16,  16, 134, OVERLAY_GREEN )
    OVERLAY_RECT(  72,   0, 192, 224, OVERLAY_YELLOW )
    OVERLAY_RECT( 192,   0, 224, 224, OVERLAY_RED )
OVERLAY_END
*/


DRIVER_INIT( 8080bw )
{
	videoram_w_p = bw_videoram_w;
	screen_red = 0;
	screen_red_enabled = 0;
	color_map_select = 0;
	flip_screen_set(0);
}

DRIVER_INIT( invaddlx )
{
	init_8080bw(machine);
/*  artwork_set_overlay(invdpt2m_overlay);*/
}

DRIVER_INIT( sstrngr2 )
{
	init_8080bw(machine);
	videoram_w_p = sstrngr2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( schaser )
{
	int i;
	UINT8* promm = memory_region( REGION_PROMS );

	schaser_effect_555_timer = timer_alloc(schaser_effect_555_cb);

	init_8080bw(machine);
	videoram_w_p = schaser_videoram_w;
	// make background palette same as foreground one
	for (i = 0; i < 0x80; i++) promm[i] = 0;

	for (i = 0x80; i < 0x400; i++)
	{
		if (promm[i] == 0x0c) promm[i] = 4;
		if (promm[i] == 0x03) promm[i] = 2;
	}

	memcpy(promm+0x400,promm,0x400);

	for (i = 0x500; i < 0x800; i++)
		if (promm[i] == 4) promm[i] = 2;
}

DRIVER_INIT( schasrcv )
{
	init_8080bw(machine);
	videoram_w_p = rollingc_videoram_w;
	background_color = 2;	/* blue */
}

DRIVER_INIT( rollingc )
{
	init_8080bw(machine);
	videoram_w_p = rollingc_videoram_w;
	background_color = 0;	/* black */
}

DRIVER_INIT( polaris )
{
	init_8080bw(machine);
	videoram_w_p = polaris_videoram_w;
}

DRIVER_INIT( lupin3 )
{
	init_8080bw(machine);
	videoram_w_p = lupin3_videoram_w;
}

DRIVER_INIT( invadpt2 )
{
	init_8080bw(machine);
	videoram_w_p = invadpt2_videoram_w;
	screen_red_enabled = 1;
}

DRIVER_INIT( cosmo )
{
	init_8080bw(machine);
	videoram_w_p = cosmo_videoram_w;
}

DRIVER_INIT( indianbt )
{
	init_8080bw(machine);
	videoram_w_p = invadpt2_videoram_w;
}

DRIVER_INIT( shuttlei )
{
	init_8080bw(machine);
	videoram_w_p = shuttlei_videoram_w;
}

void c8080bw_flip_screen_w(int data)
{
	set_vh_global_attribute(&color_map_select, data);

	if (input_port_3_r(0) & 0x01)
	{
		flip_screen_set(data);
	}
}


void c8080bw_screen_red_w(int data)
{
	if (screen_red_enabled)
	{
		set_vh_global_attribute(&screen_red, data);
	}
}


INTERRUPT_GEN( polaris_interrupt )
{
	static int cloud_speed;

	cloud_speed++;

	if (cloud_speed >= 8)	/* every 4 frames - this was verified against real machine */
	{
		cloud_speed = 0;

		cloud_pos++;

		set_vh_global_attribute(NULL,0);
	}

	c8080bw_interrupt();
}


static void plot_pixel_8080(int x, int y, int col)
{
	if (flip_screen)
	{
		x = 255-x;
		y = 255-y;
	}

	plot_pixel(tmpbitmap,x,y,Machine->pens[col]);
}

INLINE void plot_byte(int x, int y, int data, int fore_color, int back_color)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		plot_pixel_8080(x, y, (data & 0x01) ? fore_color : back_color);

		x++;
		data >>= 1;
	}
}


WRITE8_HANDLER( c8080bw_videoram_w )
{
	videoram_w_p(offset, data);
}


static WRITE8_HANDLER( bw_videoram_w )
{
	int x,y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	plot_byte(x, y, data, 1, 0);
}

static WRITE8_HANDLER( rollingc_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, background_color);
}

static WRITE8_HANDLER( schaser_videoram_w )
{
	int x,y,z,proma,promb=0x400;	// promb = 0 for green band, promb = 400 for all blue
	UINT8 col,chg=0;
	UINT8* promm = memory_region( REGION_PROMS );
	if (schaser_sx10 != schaser_sx10_done) chg++;
	if (schaser_sx10) promb = 0;

	if (chg)
	{
		for (y = 64; y < 224; y++)
		{
			for (x = 40; x < 200; x+=8)
			{
				z = y*32+x/8;
				col = colorram[z & 0x1f1f] & 0x07;
				proma = promb+((z%32)*32+y/8+93);
				plot_byte(x, y, videoram[z], col, promm[proma&0x7ff]);
			}
		}
		schaser_sx10_done = schaser_sx10;
	}

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = colorram[offset & 0x1f1f] & 0x07;
	proma = promb+((offset%32)*32+y/8+93);
	plot_byte(x, y, data, col, promm[proma&0x7ff]);
}

static WRITE8_HANDLER( lupin3_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	col = ~colorram[offset & 0x1f1f] & 0x07;

	plot_byte(x, y, data, col, 0);
}

static WRITE8_HANDLER( polaris_videoram_w )
{
	int x,i,col,back_color,fore_color,color_map;
	UINT8 y, cloud_y;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* for the background color, bit 0 of the map PROM is connected to green gun.
       red is 0 and blue is 1, giving cyan and blue for the background.  This
       is different from what the schematics shows, but it's supported
       by screenshots. */

	color_map = memory_region(REGION_PROMS)[(y >> 3 << 5) | (x >> 3)];
	back_color = (color_map & 1) ? 6 : 2;
	fore_color = ~colorram[offset & 0x1f1f] & 0x07;

	/* bit 3 is connected to the cloud enable. bits 1 and 2 are marked 'not use' (sic)
       on the schematics */

	cloud_y = y - cloud_pos;

	if ((color_map & 0x08) || (cloud_y > 64))
	{
		plot_byte(x, y, data, fore_color, back_color);
	}
	else
	{
		/* cloud appears in this part of the screen */
		for (i = 0; i < 8; i++)
		{
			if (data & 0x01)
			{
				col = fore_color;
			}
			else
			{
				int bit;
				offs_t offs;

				col = back_color;

				bit = 1 << (~x & 0x03);
				offs = ((x >> 2) & 0x03) | ((~cloud_y & 0x3f) << 2);

				col = (memory_region(REGION_USER1)[offs] & bit) ? 7 : back_color;
			}

			plot_pixel_8080(x, y, col);

			x++;
			data >>= 1;
		}
	}
}


WRITE8_HANDLER( schaser_colorram_w )
{
	int i;


	offset &= 0x1f1f;

	colorram[offset] = data;

	/* redraw region with (possibly) changed color */
	for (i = 0; i < 8; i++, offset += 0x20)
	{
		videoram_w_p(offset, videoram[offset]);
	}
}

READ8_HANDLER( schaser_colorram_r )
{
	return colorram[offset & 0x1f1f];
}


static WRITE8_HANDLER( shuttlei_videoram_w )
{
	int x,y,i;
	videoram[offset] = data;
	y = offset >>5;
	x = 8 * (offset &31);
	for (i = 0; i < 8; i++)
	{
		plot_pixel_8080(x+(7-i), y, (data & 0x01) ? 1 : 0);
		data >>= 1;
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


VIDEO_UPDATE( 8080bw )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		for (offs = 0;offs < videoram_size;offs++)
			videoram_w_p(offs, videoram[offs]);
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
	return 0;
}


PALETTE_INIT( invadpt2 )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 2),pal1bit(i >> 1));
	}
}


PALETTE_INIT( sflush )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		/* this bit arrangment is a little unusual but are confirmed by screen shots */
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 2),pal1bit(i >> 1));
	}
	palette_set_color(machine,0,0x80,0x80,0xff);
}

PALETTE_INIT( indianbt )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 1),pal1bit(i >> 2));
	}

}


static WRITE8_HANDLER( invadpt2_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 32 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0x0400 : 0;
		col = memory_region(REGION_PROMS)[colbase | (y >> 3 << 5) | (x >> 3)] & 0x07;
	}
	else
		col = 1;	/* red */

	plot_byte(x, y, data, col, 0);
}

PALETTE_INIT( cosmo )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		palette_set_color(machine,i,pal1bit(i >> 0),pal1bit(i >> 1),pal1bit(i >> 2));
	}
}

WRITE8_HANDLER( cosmo_colorram_w )
{
	int i;
	int offs = ((offset>>5)<<8) | (offset&0x1f);

	colorram[offset] = data;

	/* redraw region with (possibly) changed color */
	for (i=0; i<8; i++)
	{
		videoram_w_p(offs, videoram[offs]);
		offs+= 0x20;
	}
}

static WRITE8_HANDLER( cosmo_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = offset % 32;

	/* 32 x 32 colormap */
	col = colorram[(y >> 3 << 5) | x ] & 0x07;

	plot_byte(8*x, y, data, col, 0);
}

static WRITE8_HANDLER( sstrngr2_videoram_w )
{
	UINT8 x,y,col;

	videoram[offset] = data;

	y = offset / 32;
	x = 8 * (offset % 32);

	/* 16 x 32 colormap */
	if (!screen_red)
	{
		UINT16 colbase;

		colbase = color_map_select ? 0 : 0x0200;
		col = memory_region(REGION_PROMS)[colbase | (y >> 4 << 5) | (x >> 3)] & 0x0f;
	}
	else
		col = 1;	/* red */

	if (color_map_select)
	{
		x = 240 - x;
		y = 31 - y;
	}

	plot_byte(x, y, data, col, 0);
}
