#include "driver.h"
#include "video/generic.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1869.h"
#include "sound/cdp1869.h"

/*

    RCA CDP1869/70/76 Video Interface System (VIS)

    http://homepage.mac.com/ruske/cosmacelf/cdp1869.pdf

*/

typedef struct
{
	int dispoff;
	int fresvert, freshorz;
	int dblpage, line16, line9, cmem, cfc;
	UINT8 col, bkg;
	UINT16 pma, hma;
	UINT8 cram[4096];
	UINT8 pram[2048];
	UINT8 pcb[2048];
} CDP1869_VIDEO_CONFIG;

static CDP1869_VIDEO_CONFIG cdp1869;

int cdp1869_pcb = 0;

static void cdp1869_set_color(int i, int c, int l)
{
	int luma = 0, r, g, b;

	luma += (l & 4) ? 30 : 0; // red
	luma += (l & 1) ? 59 : 0; // green
	luma += (l & 2) ? 11 : 0; // blue

	luma = (luma * 0xff) / 100;

	r = (c & 4) ? luma : 0;
	g = (c & 1) ? luma : 0;
	b = (c & 2) ? luma : 0;

	palette_set_color( Machine, i, r, g, b );
}

PALETTE_INIT( cdp1869 )
{
	int i, c, l;

	for (i = 0; i < 8; i++)
	{
		cdp1869_set_color(i, i, i);
	}

	// HACK for tone-on-tone display (CFC=1)

	for (c = 0; c < 8; c++)
	{
		for (l = 0; l < 8; l++)
		{
			cdp1869_set_color(i, c, l);
			i++;
		}
	}
}

static int cdp1869_get_pma(void)
{
	if (cdp1869.dblpage)
	{
		return cdp1869.pma;
	}
	else
	{
		return cdp1869.pma & 0x3ff;
	}
}

static int cdp1869_get_cma(int offset)
{
	int column = cdp1869.pram[cdp1869_get_pma()];
	int row = offset & 0x07;

	int addr = (column * 8) + row;

	if (offset & 0x08)
	{
		addr += 2048;
	}

	return addr;
}

WRITE8_HANDLER ( cdp1869_charram_w )
{
	if (cdp1869.cmem)
	{
		int addr = cdp1869_get_cma(offset);
		cdp1869.cram[addr] = data;
		cdp1869.pcb[addr] = cdp1869_pcb;
	}
}

READ8_HANDLER ( cdp1869_charram_r )
{
	if (cdp1869.cmem)
	{
		int addr = cdp1869_get_cma(offset);
		cdp1869_pcb = cdp1869.pcb[addr];
		return cdp1869.cram[addr];
	}
	else
	{
		return 0;
	}
}

WRITE8_HANDLER ( cdp1869_pageram_w )
{
	int addr;

	if (cdp1869.cmem)
	{
		addr = cdp1869_get_pma();
	}
	else
	{
		addr = offset;
	}

	cdp1869.pram[addr] = data;
}

READ8_HANDLER ( cdp1869_pageram_r )
{
	int addr;

	if (cdp1869.cmem)
	{
		addr = cdp1869_get_pma();
	}
	else
	{
		addr = offset;
	}

	return cdp1869.pram[addr];
}

static int cdp1869_get_lines(void)
{
	if (cdp1869.line16 && !cdp1869.dblpage)
	{
		return 16;
	}
	else if (!cdp1869.line9)
	{
		return 9;
	}
	else
	{
		return 8;
	}
}

static int cdp1869_get_color(int ccb0, int ccb1, int pcb)
{
	int r = 0, g = 0, b = 0, color;

	switch (cdp1869.col)
	{
	case 0:
		r = ccb0;
		g = pcb;
		b = ccb1;
		break;

	case 1:
		r = ccb0;
		g = ccb1;
		b = pcb;
		break;

	case 2:
	case 3:
		r = pcb;
		g = ccb1;
		b = ccb0;
		break;
	}

	color = (r << 2) + (b << 1) + g;

	if (cdp1869.cfc)
	{
		return color + (cdp1869.bkg * 8);
	}
	else
	{
		return color;
	}
}

static void cdp1869_draw_line(mame_bitmap *bitmap, int x, int y, int data, int pcb)
{
	int i;
	int ccb0 = (data & 0x40) ? 1 : 0;
	int ccb1 = (data & 0x80) ? 1 : 0;
	int color = cdp1869_get_color(ccb0, ccb1, pcb);

	data <<= 2;

	for (i = 0; i < CDP1870_CHAR_WIDTH; i++)
	{
		if (data & 0x80)
		{
			plot_pixel(bitmap, x, y, Machine->pens[color]);

			if (!cdp1869.fresvert)
			{
				plot_pixel(bitmap, x, y + 1, Machine->pens[color]);
			}

			if (!cdp1869.freshorz)
			{
				x++;

				plot_pixel(bitmap, x, y, Machine->pens[color]);

				if (!cdp1869.fresvert)
				{
					plot_pixel(bitmap, x, y + 1, Machine->pens[color]);
				}
			}
		}

		x++;

		data <<= 1;
	}
}

static void cdp1869_draw_char(mame_bitmap *bitmap, int x, int y, int code, const rectangle *screenrect)
{
	int i, addr, addr2, pcb, data;

	addr = code * 8;
	addr2 = addr + 2048;

	for (i = 0; i < cdp1869_get_lines(); i++)
	{
		if (i == 8)
		{
			addr = addr2;
		}

		data = cdp1869.cram[addr];
		pcb = cdp1869.pcb[addr];

		cdp1869_draw_line(bitmap, screenrect->min_x + x, screenrect->min_y + y, data, pcb);

		addr++;
		y++;

		if (!cdp1869.fresvert)
		{
			y++;
		}
	}
}

VIDEO_START( cdp1869 )
{
	return 0;
}

VIDEO_UPDATE( cdp1869 )
{
	fillbitmap(bitmap, get_black_pen(machine), cliprect);

	if (!cdp1869.dispoff)
	{
		int sx, sy, rows, cols, width, height, addr;
		rectangle screen;

		screen.min_x = CDP1870_SCREEN_START;
		screen.max_x = CDP1870_SCREEN_END - 1;
		screen.min_y = CPD1870_SCANLINE_DISPLAY_START_PAL;
		screen.max_y = CPD1870_SCANLINE_DISPLAY_END_PAL - 1;

		fillbitmap(bitmap, Machine->pens[cdp1869.bkg], &screen);

		cols = cdp1869.freshorz ? 40 : 20;
		rows = cdp1869.fresvert ? 25 : 12;
		width = CDP1870_CHAR_WIDTH;
		height = cdp1869_get_lines();

		if (!cdp1869.freshorz)
		{
			width *= 2;
		}

		if (!cdp1869.fresvert)
		{
			height *= 2;
		}

		addr = cdp1869.hma;

		for (sy = 0; sy < rows; sy++)
		{
			for (sx = 0; sx < cols; sx++)
			{
				int x = sx * width;
				int y = sy * height;
				int code = cdp1869.pram[addr];

				cdp1869_draw_char(bitmap, x, y, code, &screen);

				addr++;

				if (addr > 2048) addr = 0;
			}
		}
	}

	return 0;
}

static void cdp1869_out4_w(UINT16 data)
{
	/*
      bit   description

        0   tone amp 2^0
        1   tone amp 2^1
        2   tone amp 2^2
        3   tone amp 2^3
        4   tone freq sel0
        5   tone freq sel1
        6   tone freq sel2
        7   tone off
        8   tone / 2^0
        9   tone / 2^1
       10   tone / 2^2
       11   tone / 2^3
       12   tone / 2^4
       13   tone / 2^5
       14   tone / 2^6
       15   always 0
    */

	cdp1869_set_toneamp(0, data & 0x0f);
	cdp1869_set_tonefreq(0, (data & 0x70) >> 4);
	cdp1869_set_toneoff(0, (data & 0x80) >> 7);
	cdp1869_set_tonediv(0, (data & 0x7f00) >> 8);
}

static void cdp1869_out5_w(UINT16 data)
{
	/*
      bit   description

        0   cmem access mode
        1   x
        2   x
        3   9-line
        4   x
        5   16 line hi-res
        6   double page
        7   fres vert
        8   wn amp 2^0
        9   wn amp 2^1
       10   wn amp 2^2
       11   wn amp 2^3
       12   wn freq sel0
       13   wn freq sel1
       14   wn freq sel2
       15   wn off
    */

	cdp1869.cmem = (data & 0x01);
	cdp1869.line9 = (data & 0x08) >> 3;
	cdp1869.line16 = (data & 0x20) >> 5;
	cdp1869.dblpage = (data & 0x40) >> 6;
	cdp1869.fresvert = (data & 0x80) >> 7;

	cdp1869_set_wnamp(0, (data & 0x0f00) >> 8);
	cdp1869_set_wnfreq(0, (data & 0x7000) >> 12);
	cdp1869_set_wnoff(0, (data & 0x8000) >> 15);
}

static void cdp1869_out6_w(UINT16 data)
{
	/*
      bit   description

        0   pma0 reg
        1   pma1 reg
        2   pma2 reg
        3   pma3 reg
        4   pma4 reg
        5   pma5 reg
        6   pma6 reg
        7   pma7 reg
        8   pma8 reg
        9   pma9 reg
       10   pma10 reg
       11   x
       12   x
       13   x
       14   x
       15   x
    */

	cdp1869.pma = data & 0x7ff;
}

static void cdp1869_out7_w(UINT16 data)
{
	/*
      bit   description

        0   x
        1   x
        2   hma2 reg
        3   hma3 reg
        4   hma4 reg
        5   hma5 reg
        6   hma6 reg
        7   hma7 reg
        8   hma8 reg
        9   hma9 reg
       10   hma10 reg
       11   x
       12   x
       13   x
       14   x
       15   x
    */

	cdp1869.hma = data & 0x7fc;
}

WRITE8_HANDLER ( cdp1870_out3_w )
{
	/*
      bit   description

        0   bkg green
        1   bkg blue
        2   bkg red
        3   cfc
        4   disp off
        5   colb0
        6   colb1
        7   fres horz
    */

	cdp1869.bkg = (data & 0x07);
	cdp1869.cfc = (data & 0x08) >> 3;
	cdp1869.dispoff = (data & 0x10) >> 4;
	cdp1869.col = (data & 0x60) >> 5;
	cdp1869.freshorz = (data & 0x80) >> 7;
}

WRITE8_HANDLER ( cdp1869_out_w )
{
	UINT16 word = activecpu_get_reg(CDP1802_R0 + activecpu_get_reg(CDP1802_X));

	switch (offset)
	{
	case 0:
		cdp1869_out4_w(word);
		break;
	case 1:
		cdp1869_out5_w(word);
		break;
	case 2:
		cdp1869_out6_w(word);
		break;
	case 3:
		cdp1869_out7_w(word);
		break;
	}
}
