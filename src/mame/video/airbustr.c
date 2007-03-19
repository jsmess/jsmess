/**************************************************************************

                                Air Buster
                            (C) 1990  Kaneko

                    driver by Luca Elia (l.elia@tin.it)

[Screen]
    Size:               256 x 256
    Colors:             256 x 3
    Color Space:        32R x 32G x 32B

[Scrolling layers]
    Number:             2
    Size:               512 x 512
    Scrolling:          X,Y
    Tiles Size:         16 x 16
    Tiles Number:       0x1000
    Colors:             256 x 2 (0-511)
    Format:
                Offset:     0x400    0x000
                Bit:        fedc---- --------   Color
                            ----ba98 76543210   Code

[Sprites]
    On Screen:          256 x 2
    In ROM:             0x2000
    Colors:             256     (512-767)
    Format:             See Below


**************************************************************************/
#include "driver.h"

UINT8 *airbustr_videoram2, *airbustr_colorram2;
int airbustr_clear_sprites;
static tilemap *bg_tilemap, *fg_tilemap;
static mame_bitmap *sprites_bitmap;

WRITE8_HANDLER( airbustr_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( airbustr_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( airbustr_videoram2_w )
{
	if (airbustr_videoram2[offset] != data)
	{
		airbustr_videoram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE8_HANDLER( airbustr_colorram2_w )
{
	if (airbustr_colorram2[offset] != data)
	{
		airbustr_colorram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

/*  Scroll Registers

    Port:
    4       Bg Y scroll, low 8 bits
    6       Bg X scroll, low 8 bits
    8       Fg Y scroll, low 8 bits
    A       Fg X scroll, low 8 bits

    C       3       2       1       0       <-Bit
            Bg Y    Bg X    Fg Y    Fg X    <-Scroll High Bits (complemented!)
*/

WRITE8_HANDLER( airbustr_scrollregs_w )
{
	static int bg_scrollx, bg_scrolly, fg_scrollx, fg_scrolly, highbits;

	switch (offset)		// offset 0 <-> port 4
	{
		case 0x00:	fg_scrolly =  data;	break;	// low 8 bits
		case 0x02:	fg_scrollx =  data;	break;
		case 0x04:	bg_scrolly =  data;	break;
		case 0x06:	bg_scrollx =  data;	break;
		case 0x08:	highbits   = ~data;	break;	// complemented high bits

		default:	logerror("CPU #2 - port %02X written with %02X - PC = %04X\n", offset, data, activecpu_get_pc());
	}

	tilemap_set_scrolly(bg_tilemap, 0, ((highbits << 5) & 0x100) + bg_scrolly);
	tilemap_set_scrollx(bg_tilemap, 0, ((highbits << 6) & 0x100) + bg_scrollx);
	tilemap_set_scrolly(fg_tilemap, 0, ((highbits << 7) & 0x100) + fg_scrolly);
	tilemap_set_scrollx(fg_tilemap, 0, ((highbits << 8) & 0x100) + fg_scrollx);
}

static void get_fg_tile_info(int tile_index)
{
	int attr = airbustr_colorram2[tile_index];
	int code = airbustr_videoram2[tile_index] + ((attr & 0x0f) << 8);
	int color = attr >> 4;

	SET_TILE_INFO(0, code, color, 0)
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0x0f) << 8);
	int color = (attr >> 4) + 16;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( airbustr )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 16, 16, 32, 32);

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT, 16, 16, 32, 32);

	sprites_bitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);
	tilemap_set_transparent_pen(fg_tilemap, 0);

	tilemap_set_scrolldx(bg_tilemap, 0x094, 0x06a);
	tilemap_set_scrolldy(bg_tilemap, 0x100, 0x1ff);
	tilemap_set_scrolldx(fg_tilemap, 0x094, 0x06a);
	tilemap_set_scrolldy(fg_tilemap, 0x100, 0x1ff);

	return 0;
}

/*      Sprites

Offset:                 Values:

000-0ff                 ?
100-1ff                 ?
200-2ff                 ?

300-3ff     7654----    Color Code
            ----3---    ?
            -----2--    Multi Sprite
            ------1-    Y Position High Bit
            -------0    X Position High Bit

400-4ff                 X Position Low 8 Bits
500-5ff                 Y Position Low 8 Bits
600-6ff                 Code Low 8 Bits

700-7ff     7-------    Flip X
            -6------    Flip Y
            --5-----    ?
            ---43210    Code High Bits

*/

static void airbustr_draw_sprites( mame_bitmap *bitmap,const rectangle *cliprect )
{
	int i, offs;

	for (i = 0; i < 2; i++)
	{
		UINT8 *ram = &spriteram[i * 0x800];
		int sx = 0;
		int sy = 0;

		for (offs = 0; offs < 0x100; offs++)
		{
			int attr = ram[offs + 0x300];
			int x = ram[offs + 0x400] - ((attr << 8) & 0x100);
			int y = ram[offs + 0x500] - ((attr << 7) & 0x100);

			int gfx = ram[offs + 0x700];
			int code = ram[offs + 0x600] + ((gfx & 0x1f) << 8);
			int color = attr >> 4;
			int flipx = gfx & 0x80;
			int flipy = gfx & 0x40;

			// multi sprite
			if (attr & 0x04)
			{
				sx += x;
				sy += y;
			}
			else
			{
				sx = x;
				sy = y;
			}

			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[1], code, color, flipx, flipy, sx, sy,
				cliprect, TRANSPARENCY_PEN, 0);

			// let's get back to normal to support multi sprites
			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}
		}
	}
}

VIDEO_UPDATE( airbustr )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);

	if(airbustr_clear_sprites)
	{
		fillbitmap(sprites_bitmap,0,cliprect);
		airbustr_draw_sprites(bitmap, cliprect);
	}
	else
	{
		/* keep sprites on the bitmap without clearing them */
		airbustr_draw_sprites(sprites_bitmap, cliprect);
		copybitmap(bitmap,sprites_bitmap,0,0,0,0,cliprect,TRANSPARENCY_PEN,0);
	}
	return 0;
}
