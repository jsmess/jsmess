#include "driver.h"



UINT16 *galpanic_bgvideoram,*galpanic_fgvideoram;
size_t galpanic_fgvideoram_size;
int galpanic_clear_sprites;

static mame_bitmap *sprites_bitmap;

VIDEO_START( galpanic )
{
	tmpbitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);
	sprites_bitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);

	galpanic_clear_sprites = 1;

	return 0;
}

PALETTE_INIT( galpanic )
{
	int i;

	/* first 1024 colors are dynamic */

	/* initialize 555 RGB lookup */
	for (i = 0;i < 32768;i++)
		palette_set_color(machine,i+1024,pal5bit(i >> 5),pal5bit(i >> 10),pal5bit(i >> 0));
}



WRITE16_HANDLER( galpanic_bgvideoram_w )
{
	int sx,sy;


	data = COMBINE_DATA(&galpanic_bgvideoram[offset]);

	sy = offset / 256;
	sx = offset % 256;

	plot_pixel(tmpbitmap, sx, sy, Machine->pens[1024 + (data >> 1)]);
}

WRITE16_HANDLER( galpanic_paletteram_w )
{
	data = COMBINE_DATA(&paletteram16[offset]);
	/* bit 0 seems to be a transparency flag for the front bitmap */
	palette_set_color(Machine,offset,pal5bit(data >> 6),pal5bit(data >> 11),pal5bit(data >> 1));
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static void galpanic_draw_sprites(mame_bitmap *bitmap)
{
	int offs;
	int sx,sy;

	sx = sy = 0;
	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		int x,y,code,color,flipx,flipy,attr1,attr2;

		attr1 = spriteram16[offs + 3];
		x = spriteram16[offs + 4] - ((attr1 & 0x01) << 8);
		y = spriteram16[offs + 5] + ((attr1 & 0x02) << 7);
		if (attr1 & 0x04)	/* multi sprite */
		{
			sx += x;
			sy += y;
		}
		else
		{
			sx = x;
			sy = y;
		}

		color = (attr1 & 0xf0) >> 4;

		/* bit 0 [offs + 0] is used but I don't know what for */

		attr2 = spriteram16[offs + 7];
		code = spriteram16[offs + 6] + ((attr2 & 0x1f) << 8);
		flipx = attr2 & 0x80;
		flipy = attr2 & 0x40;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				sx,sy - 16,
				&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);
	}
}

static void comad_draw_sprites(mame_bitmap *bitmap)
{
	int offs;
	int sx=0, sy=0;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int code,color,flipx,flipy;

		code = spriteram16[offs + 1] & 0x1fff;
		color = (spriteram16[offs] & 0x003c) >> 2;
		flipx = spriteram16[offs] & 0x0002;
		flipy = spriteram16[offs] & 0x0001;

		if((spriteram16[offs] & 0x6000) == 0x6000) /* Link bits */
		{
			sx += spriteram16[offs + 2] >> 6;
			sy += spriteram16[offs + 3] >> 6;
		}
		else
		{
			sx = spriteram16[offs + 2] >> 6;
			sy = spriteram16[offs + 3] >> 6;
		}

		sx = (sx&0x1ff) - (sx&0x200);
		sy = (sy&0x1ff) - (sy&0x200);

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);
	}
}

static void draw_fgbitmap(mame_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < galpanic_fgvideoram_size/2;offs++)
	{
		int sx,sy,color;

		sx = offs % 256;
		sy = offs / 256;
		color = galpanic_fgvideoram[offs];
		if (color)
			plot_pixel(bitmap, sx, sy, Machine->pens[color]);
	}
}

VIDEO_UPDATE( galpanic )
{
	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);

	draw_fgbitmap(bitmap);

	if(galpanic_clear_sprites)
	{
		fillbitmap(sprites_bitmap,0,cliprect);
		galpanic_draw_sprites(bitmap);
	}
	else
	{
		/* keep sprites on the bitmap without clearing them */
		galpanic_draw_sprites(sprites_bitmap);
		copybitmap(bitmap,sprites_bitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);
	}
	return 0;
}

VIDEO_UPDATE( comad )
{
	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);

	draw_fgbitmap(bitmap);

	if(galpanic_clear_sprites)
	{
		fillbitmap(sprites_bitmap,0,cliprect);
		comad_draw_sprites(bitmap);
	}
	else
	{
		/* keep sprites on the bitmap without clearing them */
		comad_draw_sprites(sprites_bitmap);
		copybitmap(bitmap,sprites_bitmap,0,0,0,0,&Machine->screen[0].visarea,TRANSPARENCY_PEN,0);
	}
	return 0;
}
