/***************************************************************************

  lviv.c

  Functions to emulate the video hardware of PK-01 Lviv.

  Krzysztof Strzecha

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/lviv.h"

unsigned char lviv_palette[8*3] =
{
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xa4,
	0x00, 0xa4, 0x00,
	0x00, 0xa4, 0xa4,
	0xa4, 0x00, 0x00,
	0xa4, 0x00, 0xa4,
	0xa4, 0xa4, 0x00,
	0xa4, 0xa4, 0xa4
};

unsigned short lviv_colortable[1][4] =
{
	{ 0, 0, 0, 0 }
};

PALETTE_INIT( lviv )
{
	palette_set_colors(machine, 0, lviv_palette, sizeof(lviv_palette) / 3);
	memcpy(colortable, lviv_colortable, sizeof (lviv_colortable));
}


void lviv_update_palette (UINT8 pal)
{
	lviv_colortable[0][0] = 0;
	lviv_colortable[0][1] = 0;
	lviv_colortable[0][2] = 0;
	lviv_colortable[0][3] = 0;

	lviv_colortable[0][0] |= (((pal>>3)&0x01) == ((pal>>4)&0x01)) ? 0x04 : 0x00;
	lviv_colortable[0][0] |= ((pal>>5)&0x01) ? 0x02 : 0x00;
	lviv_colortable[0][0] |= (((pal>>2)&0x01) == ((pal>>6)&0x01)) ? 0x01 : 0x00;

	lviv_colortable[0][1] |= ((pal&0x01) == ((pal>>4)&0x01)) ? 0x04 : 0x00;
	lviv_colortable[0][1] |= ((pal>>5)&0x01) ? 0x02 : 0x00;
	lviv_colortable[0][1] |= ((pal>>6)&0x01) ? 0x00 : 0x01;

	lviv_colortable[0][2] |= ((pal>>4)&0x01) ? 0x04 : 0x00;
	lviv_colortable[0][2] |= ((pal>>5)&0x01) ? 0x00 : 0x02;
	lviv_colortable[0][2] |= ((pal>>6)&0x01) ? 0x01 : 0x00;

	lviv_colortable[0][3] |= ((pal>>4)&0x01) ? 0x00 : 0x04;
	lviv_colortable[0][3] |= (((pal>>1)&0x01) == ((pal>>5)&0x01)) ? 0x02 : 0x00;
	lviv_colortable[0][3] |= ((pal>>6)&0x01) ? 0x01 : 0x00;
}

VIDEO_START( lviv )
{
	return 0;
}

VIDEO_UPDATE( lviv )
{
	int x,y;
	int pen;
	UINT8 data;

	for (y=0; y<256; y++)                  	
		for (x=0; x<256; x+=4)
		{
			data = lviv_video_ram[y*64+x/4];

			pen = lviv_colortable[0][((data & 0x08) >> 3) | ((data & 0x80) >> (3+3))];
			plot_pixel(bitmap, x, y, Machine->pens[pen]);

			pen = lviv_colortable[0][((data & 0x04) >> 2) | ((data & 0x40) >> (2+3))];
			plot_pixel(bitmap, x+1, y, Machine->pens[pen]);

			pen = lviv_colortable[0][((data & 0x02) >> 1) | ((data & 0x20) >> (1+3))];
			plot_pixel(bitmap, x+2, y, Machine->pens[pen]);

			pen = lviv_colortable[0][((data & 0x01) >> 0) | ((data & 0x10) >> (0+3))];
			plot_pixel(bitmap, x+3, y, Machine->pens[pen]);
		}
	return 0;
}
