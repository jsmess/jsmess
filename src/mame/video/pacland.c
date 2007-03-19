/***************************************************************************

Sprite/tile priority is quite complex in this game: it is handled both
internally to the CUS29 chip, and externally to it.

The bg tilemap is always behind everything.

The CUS29 mixes two 8-bit inputs, one from sprites and one from the fg
tilemap. 0xff is the transparent color. CUS29 also takes a PRI input, telling
which of the two color inputs has priority. Additionally, sprite pixels of
color >= 0xf0 always have priority.
The priority bit comes from the tilemap RAM, but through an additional filter:
sprite pixels of color < 0x80 act as a "cookie cut" mask, handled externally,
which overload the PRI bit, making the sprite always have priority. The external
RAM that holds this mask contains the OR of all sprite pixels drawn at a certain
position, therefore when sprites overlap, it is sufficient for one of them to
have color < 0x80 to promote priority of the frontmost sprite. This is used
to draw the light in round 19.

The CUS29 outputs an 8-bit pixel color, but only the bottom 7 bits are externally
checked to determine whether it is transparent or not; therefore, both 0xff and
0x7f are transparent. This is again used to draw the light in round 19, because
sprite color 0x7f will erase the tilemap and force it to be transparent.

***************************************************************************/

#include "driver.h"


UINT8 *pacland_videoram,*pacland_videoram2,*pacland_spriteram;

static int palette_bank;
static const UINT8 *pacland_color_prom;

static tilemap *bg_tilemap, *fg_tilemap;
static mame_bitmap *sprite_bitmap,*fg_bitmap;

static int scroll0,scroll1;

/***************************************************************************

  Convert the color PROMs.

  Pacland has one 1024x8 and one 1024x4 palette PROM; and three 1024x8 lookup
  table PROMs (sprites, bg tiles, fg tiles).
  The palette has 1024 colors, but it is bank switched (4 banks) and only 256
  colors are visible at a time. So, instead of creating a static palette, we
  modify it when the bank switching takes place.
  The color PROMs are connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 3 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/

static void switch_palette(running_machine *machine)
{
	int i;
	const UINT8 *color_prom = pacland_color_prom + 256 * palette_bank;

	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3;
		int r,g,b;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[1024] >> 0) & 0x01;
		bit1 = (color_prom[1024] >> 1) & 0x01;
		bit2 = (color_prom[1024] >> 2) & 0x01;
		bit3 = (color_prom[1024] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;

		palette_set_color(machine,i,r,g,b);
	}
}

PALETTE_INIT( pacland )
{
	int i;
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	pacland_color_prom = color_prom;	/* we'll need this later */
	/* skip the palette data, it will be initialized later */
	color_prom += 2 * 0x400;
	/* color_prom now points to the beginning of the lookup table */

	/* Foreground */
	for (i = 0;i < 0x400;i++)
		COLOR(0,i) = *(color_prom++);

	/* Background */
	for (i = 0;i < 0x400;i++)
		COLOR(1,i) = *(color_prom++);

	/* Sprites */
	for (i = 0;i < 0x400;i++)
	{
		COLOR(2,i) = *(color_prom++);
		COLOR(2,i + 0x400) = (COLOR(2,i) < 0x80) ? 0x7f : 0xff;	// for high priority mask (round 19)
	}

	palette_bank = 0;
	switch_palette(machine);
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	int offs = tile_index * 2;
	int attr = pacland_videoram2[offs + 1];
	int code = pacland_videoram2[offs] + ((attr & 0x01) << 8);
	int color = ((attr & 0x3e) >> 1) + ((code & 0x1c0) >> 1);
	int flags = TILE_FLIPYX(attr >> 6);

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int offs = tile_index * 2;
	int attr = pacland_videoram[offs + 1];
	int code = pacland_videoram[offs] + ((attr & 0x01) << 8);
	int color = ((attr & 0x1e) >> 1) + ((code & 0x1e0) >> 1);
	int flags = TILE_FLIPYX(attr >> 6);

	tile_info.priority = (attr & 0x20) ? 1 : 0;

	SET_TILE_INFO(0, code, color, flags)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( pacland )
{
	fg_bitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);
	sprite_bitmap = auto_bitmap_alloc(Machine->screen[0].width,Machine->screen[0].height,Machine->screen[0].format);

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,           8,8,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT_COLOR,8,8,64,32);

	tilemap_set_scroll_rows(fg_tilemap, 32);
	tilemap_set_transparent_pen(fg_tilemap, 0xff);

	spriteram = pacland_spriteram + 0x780;
	spriteram_2 = spriteram + 0x800;
	spriteram_3 = spriteram_2 + 0x800;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE8_HANDLER( pacland_videoram_w )
{
	if (pacland_videoram[offset] != data)
	{
		pacland_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset / 2);
	}
}

WRITE8_HANDLER( pacland_videoram2_w )
{
	if (pacland_videoram2[offset] != data)
	{
		pacland_videoram2[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}

WRITE8_HANDLER( pacland_scroll0_w )
{
	scroll0 = data + 256 * offset;
}

WRITE8_HANDLER( pacland_scroll1_w )
{
	scroll1 = data + 256 * offset;
}

WRITE8_HANDLER( pacland_bankswitch_w )
{
	int bankaddress;
	UINT8 *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + ((data & 0x07) << 13);
	memory_set_bankptr(1,&RAM[bankaddress]);

//  pbc = data & 0x20;

	if (palette_bank != ((data & 0x18) >> 3))
	{
		palette_bank = (data & 0x18) >> 3;
		switch_palette(Machine);
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

/* the sprite generator IC is the same as Mappy */
static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect, int draw_mask )
{
	int offs;

	for (offs = 0;offs < 0x80;offs += 2)
	{
		static int gfx_offs[2][2] =
		{
			{ 0, 1 },
			{ 2, 3 }
		};
		int sprite = spriteram[offs] + ((spriteram_3[offs] & 0x80) << 1);
		int color = (spriteram[offs+1] & 0x3f) + draw_mask * 64;
		int sx = (spriteram_2[offs+1]) + 0x100*(spriteram_3[offs+1] & 1) - 47;
		int sy = 256 - spriteram_2[offs] + 9;
		int flipx = (spriteram_3[offs] & 0x01);
		int flipy = (spriteram_3[offs] & 0x02) >> 1;
		int sizex = (spriteram_3[offs] & 0x04) >> 2;
		int sizey = (spriteram_3[offs] & 0x08) >> 3;
		int x,y;

		sprite &= ~sizex;
		sprite &= ~(sizey << 1);

		if (flip_screen)
		{
			flipx ^= 1;
			flipy ^= 1;
		}

		sy -= 16 * sizey;
		sy = (sy & 0xff) - 32;	// fix wraparound

		for (y = 0;y <= sizey;y++)
		{
			for (x = 0;x <= sizex;x++)
			{
				drawgfx(bitmap,Machine->gfx[2],
					sprite + gfx_offs[y ^ (sizey * flipy)][x ^ (sizex * flipx)],
					color,
					flipx,flipy,
					sx + 16*x,sy + 16*y,
					cliprect,TRANSPARENCY_COLOR,0xff);
			}
		}
	}
}


static void draw_fg( mame_bitmap *bitmap, const rectangle *cliprect, int priority )
{
	/* clear temp bitmap using color 0x7f */
	fillbitmap(fg_bitmap, 0x7f, cliprect);

	/* draw tilemap over it (transparent color is 0xff) */
	tilemap_draw(fg_bitmap, cliprect, fg_tilemap, priority, 0);

	/* draw sprite high priority mask (color 0x7f) over it */
	draw_sprites(fg_bitmap, cliprect, 1);

	/* copy temp bitmap to the screen (transparent color is 0x7f) */
	copybitmap(bitmap, fg_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_COLOR, 0x7f);
}


VIDEO_UPDATE( pacland )
{
	int x,y,pen;
	int row;

	for (row = 5; row < 29; row++)
		tilemap_set_scrollx(fg_tilemap, row, flip_screen ? scroll0-7 : scroll0);
	tilemap_set_scrollx(bg_tilemap, 0, flip_screen ? scroll1-4 : scroll1-3);

	/* draw background */
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* draw low priority fg tiles */
	draw_fg(bitmap, cliprect, 0);

	/* draw sprites in a temporary bitmap */
	fillbitmap(sprite_bitmap, 0x7f, cliprect);
	draw_sprites(sprite_bitmap, cliprect, 0);
	/* copy sprites */
	copybitmap(bitmap, sprite_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_COLOR, 0x7f);

	/* draw high priority fg tiles */
	draw_fg(bitmap, cliprect, 1);

	/* sprite colors >=0xf0 still have priority over that */
	for (y = 0;y < sprite_bitmap->height;y++)
	{
		for (x = 0;x < sprite_bitmap->width;x++)
		{
			pen = read_pixel(sprite_bitmap,x,y);
			if (pen >= 0xf0)
				plot_pixel(bitmap,x,y,pen);
		}
	}
	return 0;
}
