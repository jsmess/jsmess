/**
 * @file video/djboy.c
 *
 * video hardware for DJ Boy
 */
#include "driver.h"

static UINT8 djboy_videoreg, djboy_scrollx, djboy_scrolly;
static tilemap *background;

void djboy_set_videoreg( UINT8 data )
{
	djboy_videoreg = data;
}

WRITE8_HANDLER( djboy_scrollx_w )
{
	djboy_scrollx = data;
}

WRITE8_HANDLER( djboy_scrolly_w )
{
	djboy_scrolly = data;
}


static TILE_GET_INFO( get_bg_tile_info )
{
	UINT8 attr = videoram[tile_index + 0x400];
	int code = videoram[tile_index] + (attr&0xf)*256;
	int color = attr>>4;
	if( color&8 )
	{
		code |= 0x1000;
	}
	SET_TILE_INFO(2, code, color, 0);	/* no flip */
}

WRITE8_HANDLER( djboy_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty( background, offset & 0x7ff);
}

VIDEO_START( djboy )
{
	background = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,64,32);
}

static void draw_sprites(running_machine* machine, mame_bitmap *bitmap,const rectangle *cliprect )
{
	int page;
	for( page=0; page<2; page++ )
	{
		const UINT8 *pSource = &spriteram[page*0x800];
		int sx = 0;
		int sy = 0;
		int offs;
		for ( offs = 0 ; offs < 0x100 ; offs++)
		{
			int attr	=	pSource[offs + 0x300];
			int color	= 	attr>>4;
			int x		=	pSource[offs + 0x400] - ((attr << 8) & 0x100);
			int y		=	pSource[offs + 0x500] - ((attr << 7) & 0x100);
			int gfx		=	pSource[offs + 0x700];
			int code	=	pSource[offs + 0x600] + ((gfx & 0x3f) << 8);
			int flipx	=	gfx & 0x80;
			int flipy	=	gfx & 0x40;
			int gfxbank = 1;
			if( code>=0x3e00 )
			{
				gfxbank = 0;
			}
			if( attr & 0x04 )
			{
				sx += x;
				sy += y;
			}
			else
			{
				sx  = x;
				sy  = y;
			}
			drawgfx(
				bitmap,machine->gfx[gfxbank],
				code,
				color,
				flipx, flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,0);
		} /* next tile */
	} /* next page */
} /* draw_sprites */

WRITE8_HANDLER( djboy_paletteram_w )
{
	int val;

	paletteram[offset] = data;
	offset &= ~1;
	val = (paletteram[offset]<<8) | paletteram[offset+1];

	palette_set_color_rgb(Machine,offset/2,pal4bit(val >> 8),pal4bit(val >> 4),pal4bit(val >> 0));
}

VIDEO_UPDATE( djboy )
{
	/**
     * xx------ msb x
     * --x----- msb y
     * ---x---- flipscreen?
     * ----xxxx ROM bank
     */
	int scroll;
	scroll = djboy_scrollx | ((djboy_videoreg&0xc0)<<2);
	tilemap_set_scrollx( background, 0, scroll-0x391 );
	scroll = djboy_scrolly | ((djboy_videoreg&0x20)<<3);
	tilemap_set_scrolly( background, 0, scroll );
	tilemap_draw( bitmap, cliprect,background,0,0 );
	draw_sprites(machine, bitmap, cliprect );
	return 0;
}
