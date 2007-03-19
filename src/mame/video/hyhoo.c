/******************************************************************************

    Video Hardware for Nichibutsu Mahjong series.

    Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/01/28 -

******************************************************************************/

#include "driver.h"
#include "nb1413m3.h"


static int blitter_destx, blitter_desty;
static int blitter_sizex, blitter_sizey;
static int blitter_src_addr;
static int blitter_direction_x, blitter_direction_y;
static int hyhoo_gfxrom;
static int hyhoo_dispflag;
static int hyhoo_highcolorflag;
static int hyhoo_flipscreen;
static int hyhoo_screen_refresh;

static mame_bitmap *hyhoo_tmpbitmap;
static unsigned short *hyhoo_videoram;
static unsigned short *hyhoo_videoworkram;
static unsigned char *hyhoo_clut;


static void hyhoo_vramflip(void);
static void hyhoo_gfxdraw(void);


/******************************************************************************


******************************************************************************/
PALETTE_INIT( hyhoo )
{
	int i;

	/* initialize 655 RGB lookup */
	for (i = 0; i < 65536; i++)
	{
		// bbbbbggg_ggrrrrrr
		palette_set_color(machine, i, pal6bit(i >> 0), pal5bit(i >> 6), pal5bit(i >> 11));
	}
}

WRITE8_HANDLER( hyhoo_clut_w )
{
	hyhoo_clut[offset & 0x0f] = (data ^ 0xff);
}

/******************************************************************************


******************************************************************************/
WRITE8_HANDLER( hyhoo_blitter_w )
{
	switch (offset)
	{
		case 0x00:	blitter_src_addr = (blitter_src_addr & 0xff00) | data;
					nb1413m3_gfxradr_l_w(0, data); break;
		case 0x01:	blitter_src_addr = (blitter_src_addr & 0x00ff) | (data << 8);
					nb1413m3_gfxradr_h_w(0, data); break;
		case 0x02:	blitter_destx = data; break;
		case 0x03:	blitter_desty = data; break;
		case 0x04:	blitter_sizex = data; break;
		case 0x05:	blitter_sizey = data;
					/* writing here also starts the blit */
					hyhoo_gfxdraw();
					break;
		case 0x06:	blitter_direction_x = (data & 0x01) ? 1 : 0;
					blitter_direction_y = (data & 0x02) ? 1 : 0;
					hyhoo_flipscreen = (data & 0x04) ? 0 : 1;
					hyhoo_dispflag = (data & 0x08) ? 0 : 1;
					hyhoo_vramflip();
					break;
		case 0x07:	break;
	}
}

WRITE8_HANDLER( hyhoo_romsel_w )
{
	hyhoo_gfxrom = (((data & 0xc0) >> 4) + (data & 0x03));
	hyhoo_highcolorflag = data;
	nb1413m3_gfxrombank_w(0, data);

	if ((0x20000 * hyhoo_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		popmessage("GFXROM BANK OVER!!");
#endif
		hyhoo_gfxrom &= (memory_region_length(REGION_GFX1) / 0x20000 - 1);
	}
}

/******************************************************************************


******************************************************************************/
void hyhoo_vramflip(void)
{
	static int hyhoo_flipscreen_old = 0;
	int x, y;
	unsigned short color1, color2;

	if (hyhoo_flipscreen == hyhoo_flipscreen_old) return;

	for (y = 0; y < (Machine->screen[0].height / 2); y++)
	{
		for (x = 0; x < Machine->screen[0].width; x++)
		{
			color1 = hyhoo_videoram[(y * Machine->screen[0].width) + x];
			color2 = hyhoo_videoram[((y ^ 0xff) * Machine->screen[0].width) + (x ^ 0x1ff)];
			hyhoo_videoram[(y * Machine->screen[0].width) + x] = color2;
			hyhoo_videoram[((y ^ 0xff) * Machine->screen[0].width) + (x ^ 0x1ff)] = color1;

			color1 = hyhoo_videoworkram[(y * Machine->screen[0].width) + x];
			color2 = hyhoo_videoworkram[((y ^ 0xff) * Machine->screen[0].width) + (x ^ 0x1ff)];
			hyhoo_videoworkram[(y * Machine->screen[0].width) + x] = color2;
			hyhoo_videoworkram[((y ^ 0xff) * Machine->screen[0].width) + (x ^ 0x1ff)] = color1;
		}
	}

	hyhoo_flipscreen_old = hyhoo_flipscreen;
	hyhoo_screen_refresh = 1;
}

static void update_pixel(int x, int y)
{
	int color = hyhoo_videoram[(y * 512) + x];
	plot_pixel(hyhoo_tmpbitmap, x, y, Machine->pens[color]);
}


static void blitter_timer_callback(int param)
{
	nb1413m3_busyflag = 1;
}

void hyhoo_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int gfxaddr;
	unsigned short color, color1, color2;
	unsigned char r, g, b;

	nb1413m3_busyctr = 0;

	hyhoo_gfxrom |= ((nb1413m3_sndrombank1 & 0x02) << 3);

	startx = blitter_destx + blitter_sizex;
	starty = blitter_desty + blitter_sizey;

	if (blitter_direction_x)
	{
		sizex = blitter_sizex ^ 0xff;
		skipx = 1;
	}
	else
	{
		sizex = blitter_sizex;
		skipx = -1;
	}

	if (blitter_direction_y)
	{
		sizey = blitter_sizey ^ 0xff;
		skipy = 1;
	}
	else
	{
		sizey = blitter_sizey;
		skipy = -1;
	}

	gfxaddr = (hyhoo_gfxrom << 17) + (blitter_src_addr << 1);

	for (y = starty, ctry = sizey; ctry >= 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx >= 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				popmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			dx1 = (2 * x + 0) & 0x1ff;
			dx2 = (2 * x + 1) & 0x1ff;
			dy = y & 0xff;

			if (hyhoo_flipscreen)
			{
				dx1 ^= 0x1ff;
				dx2 ^= 0x1ff;
				dy ^= 0xff;
			}

			if (hyhoo_highcolorflag & 0x04)
			{
				// direct mode

				if (hyhoo_highcolorflag & 0x20)
				{
					/* least significant bits */

					// src xxxxxxxx_bbbggrrr
					// dst xxbbbxxx_ggxxxrrr

					r = (((color & 0x07) >> 0) & 0x07);
					g = (((color & 0x18) >> 3) & 0x03);
					b = (((color & 0xe0) >> 5) & 0x07);
					color = ((b << (11 + 0)) | (g << (6 + 0)) | (r << (0 + 0)));

					if (color != 0xff)
					{
						hyhoo_videoram[(dy * Machine->screen[0].width) + dx1] |= color;
						hyhoo_videoram[(dy * Machine->screen[0].width) + dx2] |= color;
						update_pixel(dx1, dy);
						update_pixel(dx2, dy);
					}

					continue;
				}
				else
				{
					/* most significant bits */

					// src xxxxxxxx_bbgggrrr
					// dst bbxxxggg_xxrrrxxx

					r = (((color & 0x07) >> 0) & 0x07);
					g = (((color & 0x38) >> 3) & 0x07);
					b = (((color & 0xc0) >> 6) & 0x03);
					color = ((b << (11 + 3)) | (g << (6 + 2)) | (r << (0 + 3)));

					if (color != 0xff)
					{
						hyhoo_videoram[(dy * Machine->screen[0].width) + dx1] = color;
						hyhoo_videoram[(dy * Machine->screen[0].width) + dx2] = color;
						update_pixel(dx1, dy);
						update_pixel(dx2, dy);
					}
				}
			}
			else
			{
				// lookup table mode

				if (blitter_direction_x)
				{
					// flip
					color1 = (color & 0x0f) >> 0;
					color2 = (color & 0xf0) >> 4;
				}
				else
				{
					// normal
					color1 = (color & 0xf0) >> 4;
					color2 = (color & 0x0f) >> 0;
				}

				if (hyhoo_clut[color1] != 0xff)
				{
					// src xxxxxxxx_bbgggrrr
					// dst bbxxxggg_xxrrrxxx

					r = (hyhoo_clut[color1] & 0x07) >> 0;
					g = (hyhoo_clut[color1] & 0x38) >> 3;
					b = (hyhoo_clut[color1] & 0xc0) >> 6;
					color1 = ((b << (11 + 3)) | (g << (6 + 2)) | (r << (0 + 3)));

					hyhoo_videoram[(dy * Machine->screen[0].width) + dx1] = color1;
					update_pixel(dx1, dy);
				}

				if (hyhoo_clut[color2] != 0xff)
				{
					// src xxxxxxxx_bbgggrrr
					// dst bbxxxggg_xxrrrxxx

					r = (hyhoo_clut[color2] & 0x07) >> 0;
					g = (hyhoo_clut[color2] & 0x38) >> 3;
					b = (hyhoo_clut[color2] & 0xc0) >> 6;
					color2 = ((b << (11 + 3)) | (g << (6 + 2)) | (r << (0 + 3)));

					hyhoo_videoram[(dy * Machine->screen[0].width) + dx2] = color2;
					update_pixel(dx2, dy);
				}
			}
			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = 0;
	timer_set((double)nb1413m3_busyctr * TIME_IN_NSEC(2500), 0, blitter_timer_callback);
}

/******************************************************************************


******************************************************************************/
VIDEO_START( hyhoo )
{
	hyhoo_tmpbitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);
	hyhoo_videoram = auto_malloc(Machine->screen[0].width * Machine->screen[0].height * sizeof(UINT16));
	hyhoo_videoworkram = auto_malloc(Machine->screen[0].width * Machine->screen[0].height * sizeof(UINT16));
	hyhoo_clut = auto_malloc(0x10 * sizeof(UINT8));
	memset(hyhoo_videoram, 0x0000, (Machine->screen[0].width * Machine->screen[0].height * sizeof(UINT16)));
	return 0;
}

/******************************************************************************


******************************************************************************/
VIDEO_UPDATE( hyhoo )
{
	int x, y;

	if (get_vh_global_attribute_changed() || hyhoo_screen_refresh)
	{
		hyhoo_screen_refresh = 0;

		for (y = 0; y < Machine->screen[0].height; y++)
		{
			for (x = 0; x < Machine->screen[0].width; x++)
			{
				update_pixel(x, y);
			}
		}
	}

	if (hyhoo_dispflag)
	{
		copyscrollbitmap(bitmap, hyhoo_tmpbitmap, 0, 0, 0, 0, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x0000], 0);
	}
	return 0;
}
