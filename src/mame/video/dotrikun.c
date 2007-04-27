/***************************************************************************

Dottori Kun (Head On's mini game)
(c)1990 SEGA

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/12/15 -

***************************************************************************/

#include "driver.h"



/*******************************************************************

    Palette Setting.

*******************************************************************/
WRITE8_HANDLER( dotrikun_color_w )
{
	palette_set_color(Machine, 0, pal1bit(data >> 3), pal1bit(data >> 4), pal1bit(data >> 5));		// BG color
	palette_set_color(Machine, 1, pal1bit(data >> 0), pal1bit(data >> 1), pal1bit(data >> 2));		// DOT color
}


/*******************************************************************

    Draw Pixel.

*******************************************************************/
WRITE8_HANDLER( dotrikun_videoram_w )
{
	int i;
	int x, y;


	videoram[offset] = data;

	x = 2 * (((offset % 16) * 8));
	y = 2 * ((offset / 16));

	if (x >= Machine->screen[0].visarea.min_x &&
			x <= Machine->screen[0].visarea.max_x &&
			y >= Machine->screen[0].visarea.min_y &&
			y <= Machine->screen[0].visarea.max_y)
	{
		for (i = 0; i < 8; i++)
		{
			pen_t pen = Machine->pens[((data >> i) & 0x01)];

			/* I think the video hardware doubles pixels, screen would be too small otherwise */
			*BITMAP_ADDR16(tmpbitmap, y+0, x + 2*(7 - i)+0) = pen;
			*BITMAP_ADDR16(tmpbitmap, y+0, x + 2*(7 - i)+1) = pen;
			*BITMAP_ADDR16(tmpbitmap, y+1, x + 2*(7 - i)+0) = pen;
			*BITMAP_ADDR16(tmpbitmap, y+1, x + 2*(7 - i)+1) = pen;
		}
	}
}


VIDEO_UPDATE( dotrikun )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		/* redraw bitmap */

		for (offs = 0; offs < videoram_size; offs++)
			dotrikun_videoram_w(offs,videoram[offs]);
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&machine->screen[0].visarea,TRANSPARENCY_NONE,0);
	return 0;
}
