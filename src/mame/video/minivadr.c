/***************************************************************************

Minivader (Space Invaders's mini game)
(c)1990 Taito Corporation

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/12/19 -

***************************************************************************/

#include "driver.h"



/*******************************************************************

    Palette Setting.

*******************************************************************/

PALETTE_INIT( minivadr )
{
	palette_set_color(machine,0,0x00,0x00,0x00);
	palette_set_color(machine,1,0xff,0xff,0xff);
}


/*******************************************************************

    Draw Pixel.

*******************************************************************/
WRITE8_HANDLER( minivadr_videoram_w )
{
	int i;
	int x, y;
	int color;


	videoram[offset] = data;

	x = (offset % 32) * 8;
	y = (offset / 32);

	if (x >= Machine->screen[0].visarea.min_x &&
			x <= Machine->screen[0].visarea.max_x &&
			y >= Machine->screen[0].visarea.min_y &&
			y <= Machine->screen[0].visarea.max_y)
	{
		for (i = 0; i < 8; i++)
		{
			color = Machine->pens[((data >> i) & 0x01)];

			plot_pixel(tmpbitmap, x + (7 - i), y, color);
		}
	}
}


VIDEO_UPDATE( minivadr )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		/* redraw bitmap */

		for (offs = 0; offs < videoram_size; offs++)
			minivadr_videoram_w(offs,videoram[offs]);
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);
	return 0;
}
