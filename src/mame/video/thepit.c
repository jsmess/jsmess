/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


UINT8 *thepit_videoram;
UINT8 *thepit_colorram;
UINT8 *thepit_attributesram;
UINT8 *thepit_spriteram;
size_t thepit_spriteram_size;

static UINT8 graphics_bank;
static UINT8 color_count;
static int color_granularity;
static UINT8 thepit_flip_screen_x;
static UINT8 thepit_flip_screen_y;
static tilemap *thepit_tilemap;


static rectangle spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8, 30*8-1
};

static rectangle spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED


***************************************************************************/

PALETTE_INIT( thepit )
{
	int i;

	/* allocate primary colors for the background and foreground
       this is wrong, but I don't know where to pick the colors from */
	for (i = 0; i < 8; i++)
	{
		palette_set_color(machine, i, pal1bit(i >> 2), pal1bit(i >> 1), pal1bit(i >> 0));
	}

	for (i = 0; i < machine->drv->total_colors - 8; i++)
	{
		int bit0, bit1, bit2, r, g, b;

		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(machine, i + 8, r, g, b);
	}

	/* background priority colors */
	for (i = 0; i < machine->drv->color_table_len - 32; i++)
	{
		colortable[i] = i + 8;
	}

	/* foreground priority colors - creates solid square */
	for (i = 0; i < 32; i++)
	{
		colortable[machine->drv->color_table_len - 32 + i] = i >> 2;
	}
}


/***************************************************************************

 Super Mouse has 5 bits per gun (maybe 6 for green), exact weights are
 unknown.

***************************************************************************/

PALETTE_INIT( suprmous )
{
	int i;

	/* allocate primary colors for the background and foreground
       this is wrong, but I don't know where to pick the colors from */
	for (i = 0; i < 8; i++)
	{
		palette_set_color(machine, i, pal1bit(i >> 2), pal1bit(i >> 1), pal1bit(i >> 0));
	}

	for (i = 0; i < machine->drv->total_colors - 8; i++)
	{
		UINT8 r = ((color_prom[i + 0x20] >> 6) | (color_prom[i + 0x00] << 2)) & 0x1f;

		UINT8 g = BITSWAP8(color_prom[i + 0x20], 7, 6, 0, 1, 2, 3, 4, 5) & 0x1f;

		UINT8 b = BITSWAP8(color_prom[i + 0x00], 0, 1, 2, 3, 4, 5, 6, 7) & 0x1f;

		palette_set_color(machine, i + 8, pal5bit(r), pal5bit(g), pal5bit(b));
	}

	for (i = 0;i < machine->drv->color_table_len;i++)
	{
		colortable[i] = i + 8;
	}

	/* background priority colors */
	for (i = 0; i < machine->drv->color_table_len - 64; i++)
	{
		colortable[i] = i + 8;
	}

	/* foreground priority colors - creates solid square */
	for (i = 0; i < 64; i++)
	{
		colortable[machine->drv->color_table_len - 64 + i] = i >> 3;
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static TILE_GET_INFO( get_tile_info )
{
	UINT8 fore_color;

	UINT8 code = thepit_videoram[tile_index];
	UINT8 back_color = (thepit_colorram[tile_index] & 0x70) >> 4;
	int priority = (back_color != 0) && ((thepit_colorram[tile_index] & 0x80) == 0);

	if (priority)
	{
		/* foreground - solid square */
		fore_color = color_count + back_color;
	}
	else
	{
		/* background */
		fore_color = thepit_colorram[tile_index] & (color_count - 1);

		/* set up pen #0 color */
		Machine->gfx[graphics_bank]->colortable[fore_color * color_granularity] = Machine->pens[back_color];
	}

	SET_TILE_INFO(2 * graphics_bank, code, fore_color, 0);
	tileinfo->priority = priority;
}



/*************************************
 *
 *  Video system start
 *
 *************************************/

static void video_start_common(UINT8 _color_count, int _color_granularity)
{
	thepit_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	tilemap_set_scroll_cols(thepit_tilemap, 32);

	color_count = _color_count;
	color_granularity = _color_granularity;
}


VIDEO_START( thepit )
{
	video_start_common(8, 4);

	graphics_bank = 0;	/* not used in this game */

	return 0;
}


VIDEO_START( intrepid )
{
	video_start_common(8, 4);

	return 0;
}


VIDEO_START( suprmous )
{
	video_start_common(4, 8);

	graphics_bank = 0;	/* not used in this game */

	return 0;
}



/*************************************
 *
 *  Memory handlers
 *
 *************************************/

WRITE8_HANDLER( thepit_videoram_w )
{
	thepit_videoram[offset] = data;
	tilemap_mark_tile_dirty(thepit_tilemap, offset);
}


WRITE8_HANDLER( thepit_colorram_w )
{
	thepit_colorram[offset] = data;
	tilemap_mark_tile_dirty(thepit_tilemap, offset);
}


WRITE8_HANDLER( thepit_flip_screen_x_w )
{
	thepit_flip_screen_x = data & 0x01;

	tilemap_set_flip(thepit_tilemap, thepit_flip_screen_x ? TILEMAP_FLIPX : 0);
}


WRITE8_HANDLER( thepit_flip_screen_y_w )
{
	thepit_flip_screen_y = data & 0x01;

	tilemap_set_flip(thepit_tilemap, thepit_flip_screen_y ? TILEMAP_FLIPY : 0);
}


WRITE8_HANDLER( intrepid_graphics_bank_w )
{
	if (graphics_bank != (data & 0x01))
	{
		graphics_bank = data & 0x01;

		tilemap_mark_all_tiles_dirty(thepit_tilemap);
	}
}


READ8_HANDLER( thepit_input_port_0_r )
{
	/* Read either the real or the fake input ports depending on the
       horizontal flip switch. (This is how the real PCB does it) */
	if (thepit_flip_screen_x)
	{
		return input_port_3_r(offset);
	}
	else
	{
		return input_port_0_r(offset);
	}
}



/*************************************
 *
 *  Video update
 *
 *************************************/

static void draw_sprites(running_machine *machine,
						 mame_bitmap *bitmap,
						 const rectangle *cliprect,
						 int priority_to_draw)
{
	int offs;

	for (offs = thepit_spriteram_size - 4; offs >= 0; offs -= 4)
	{
		if (((thepit_spriteram[offs + 2] & 0x08) >> 3) == priority_to_draw)
		{
			UINT8 y, x, flipx, flipy;

			if ((thepit_spriteram[offs + 0] == 0) || (thepit_spriteram[offs + 3] == 0))
			{
				continue;
			}

			y = 240 - thepit_spriteram[offs];
			x = thepit_spriteram[offs + 3] + 1;

			flipx = thepit_spriteram[offs + 1] & 0x40;
			flipy = thepit_spriteram[offs + 1] & 0x80;

			if (thepit_flip_screen_y)
			{
				y = 240 - y;
				flipy = !flipy;
			}

			if (thepit_flip_screen_x)
			{
				x = 242 - x;
				flipx = !flipx;
			}

			/* sprites 0-3 are drawn one pixel down */
			if (offs < 16) y++;

			drawgfx(bitmap, machine->gfx[2 * graphics_bank + 1],
					thepit_spriteram[offs + 1] & 0x3f,
					thepit_spriteram[offs + 2] & (color_count - 1),
					flipx, flipy, x, y,
					thepit_flip_screen_x ? &spritevisibleareaflipx : &spritevisiblearea,
					TRANSPARENCY_PEN, 0);
		}
	}
}


VIDEO_UPDATE( thepit )
{
	offs_t offs;

	for (offs = 0; offs < 32; offs++)
	{
		tilemap_set_scrolly(thepit_tilemap, offs, thepit_attributesram[offs << 1]);
	}

	/* low priority tiles */
	tilemap_draw(bitmap, cliprect, thepit_tilemap, 0, 0);

	/* low priority sprites */
	draw_sprites(machine, bitmap, cliprect, 0);

	/* high priority tiles */
	tilemap_draw(bitmap, cliprect, thepit_tilemap, 1, 1);

	/* high priority sprites */
	draw_sprites(machine, bitmap, cliprect, 1);

	return 0;
}
