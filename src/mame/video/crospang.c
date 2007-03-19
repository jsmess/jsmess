/*

  Cross Pang
  video hardware emulation

 -- this seems to be the same as the tumblepop bootleg based hardware
    in tumbleb.c

 previously the driver used alpha, now we do a flicker effect like
 tumblep.c instead..  I think the flicker effect is more correct

*/

#include "driver.h"

static tilemap *bg_layer,*fg_layer;
UINT16 *crospang_bg_videoram,*crospang_fg_videoram;

WRITE16_HANDLER ( crospang_fg_scrolly_w )
{
	tilemap_set_scrolly(fg_layer,0,data+8);
}

WRITE16_HANDLER ( crospang_bg_scrolly_w )
{
	tilemap_set_scrolly(bg_layer,0,data+8);
}

WRITE16_HANDLER ( crospang_fg_scrollx_w )
{
	tilemap_set_scrollx(fg_layer,0,data);
}

WRITE16_HANDLER ( crospang_bg_scrollx_w )
{
	tilemap_set_scrollx(bg_layer,0,data+4);
}

WRITE16_HANDLER ( crospang_fg_videoram_w )
{
	COMBINE_DATA(&crospang_fg_videoram[offset]);
	tilemap_mark_tile_dirty(fg_layer,offset);
}

WRITE16_HANDLER ( crospang_bg_videoram_w )
{
	COMBINE_DATA(&crospang_bg_videoram[offset]);
	tilemap_mark_tile_dirty(bg_layer,offset);
}

static void get_bg_tile_info(int tile_index)
{
	int data  = crospang_bg_videoram[tile_index];
	int tile  = data & 0xfff;
	int color = (data >> 12) & 0x0f;

	SET_TILE_INFO(1,tile,color + 0x20,0)
}

static void get_fg_tile_info(int tile_index)
{
	int data  = crospang_fg_videoram[tile_index];
	int tile  = data & 0xfff;
	int color = (data >> 12) & 0x0f;

	SET_TILE_INFO(1,tile,color + 0x10,0)
}

/*

 offset

      0     -------yyyyyyyyy  y offset
            -----hh---------  sprite height
            ---a------------  alpha blending enable
            f---------------  flip x
            -??-?-----------  unused

      1     --ssssssssssssss  sprite code
            ??--------------  unused

      2     -------xxxxxxxxx  x offset
            ---cccc---------  colors
            ???-------------  unused

      3     ----------------  unused

*/

/* jumpkids / tumbleb.c! */
static void crospang_drawsprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;
	int flipscreen = 0;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spriteram16[offs+1] & 0x7fff;
		if (!sprite) continue;

		y = spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >>9) & 0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

	//  sprite &= ~multi; /* Todo:  I bet TumblePop bootleg doesn't do this either */
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flipscreen)
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[0],
					sprite - multi * inc,
					colour,
					fx,fy,
					x-4,y-7 + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

VIDEO_START( crospang )
{
	bg_layer = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,32,32);
	fg_layer = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

	tilemap_set_transparent_pen(fg_layer,0);

//  alpha_set_level(0x80);

	return 0;
}

VIDEO_UPDATE( crospang )
{
	tilemap_draw(bitmap,cliprect,bg_layer,0,0);
	tilemap_draw(bitmap,cliprect,fg_layer,0,0);
	crospang_drawsprites(bitmap,cliprect);
	return 0;
}
