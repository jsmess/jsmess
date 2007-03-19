/* Kick Goal - video */

#include "driver.h"

extern UINT16 *kickgoal_fgram, *kickgoal_bgram, *kickgoal_bg2ram, *kickgoal_scrram;
tilemap *kickgoal_fgtm, *kickgoal_bgtm, *kickgoal_bg2tm;

/* FG */
static void get_kickgoal_fg_tile_info(int tile_index)
{
	int tileno = kickgoal_fgram[tile_index*2] & 0x0fff;
	int color = kickgoal_fgram[tile_index*2+1] & 0x000f;

	SET_TILE_INFO(0,tileno + 0x7000,color + 0x00,0)
}

/* BG */
static void get_kickgoal_bg_tile_info(int tile_index)
{
	int tileno = kickgoal_bgram[tile_index*2] & 0x0fff;
	int color = kickgoal_bgram[tile_index*2+1] & 0x000f;

	SET_TILE_INFO(1,tileno + 0x1000,color + 0x10,0)
}

/* BG 2 */
static void get_kickgoal_bg2_tile_info(int tile_index)
{
	int tileno = kickgoal_bg2ram[tile_index*2] & 0x07ff;
	int color = kickgoal_bg2ram[tile_index*2+1] & 0x000f;
	int flipx = kickgoal_bg2ram[tile_index*2+1] & 0x0020;

	SET_TILE_INFO(2,tileno + 0x800,color + 0x20,flipx ? TILE_FLIPX : 0);
}


static UINT32 tilemap_scan_kicksbg2( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*8 + (row & 0x7) + ((row & 0x3c) >> 3) * 0x200;
}

static UINT32 tilemap_scan_kicksbg( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*16 + (row & 0xf) + ((row & 0x70) >> 4) * 0x400;
}

static UINT32 tilemap_scan_kicksfg( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*32 + (row & 0x1f) + ((row & 0x20) >> 5) * 0x800;
}


VIDEO_START( kickgoal )
{
	kickgoal_fgtm = tilemap_create(get_kickgoal_fg_tile_info,tilemap_scan_kicksfg,TILEMAP_TRANSPARENT, 8, 16,64,64);
		tilemap_set_transparent_pen(kickgoal_fgtm,15);
	kickgoal_bgtm = tilemap_create(get_kickgoal_bg_tile_info,tilemap_scan_kicksbg,TILEMAP_TRANSPARENT, 16, 32,64,64);
		tilemap_set_transparent_pen(kickgoal_bgtm,15);
	kickgoal_bg2tm = tilemap_create(get_kickgoal_bg2_tile_info,tilemap_scan_kicksbg2,TILEMAP_OPAQUE, 32, 64,64,64);
	return 0;
}



WRITE16_HANDLER( kickgoal_fgram_w )
{
	if (kickgoal_fgram[offset] != data)
	{
		kickgoal_fgram[offset] = data;
		tilemap_mark_tile_dirty(kickgoal_fgtm,offset/2);
	}
}

WRITE16_HANDLER( kickgoal_bgram_w )
{
	if (kickgoal_bgram[offset] != data)
	{
		kickgoal_bgram[offset] = data;
		tilemap_mark_tile_dirty(kickgoal_bgtm,offset/2);
	}
}

WRITE16_HANDLER( kickgoal_bg2ram_w )
{
	if (kickgoal_bg2ram[offset] != data)
	{
		kickgoal_bg2ram[offset] = data;
		tilemap_mark_tile_dirty(kickgoal_bg2tm,offset/2);
	}
}



static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	const gfx_element *gfx = Machine->gfx[1];
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int xpos = spriteram16[offs+3];
		int ypos = spriteram16[offs+0] & 0x00ff;
		int tileno = spriteram16[offs+2] & 0x0fff;
		int flipx = spriteram16[offs+1] & 0x0020;
		int color = spriteram16[offs+1] & 0x000f;

		if (spriteram16[offs+0] & 0x0100) break;

		ypos *= 2;

		ypos = 0x200-ypos;

		drawgfx(bitmap,gfx,
				tileno,
				0x30 + color,
				flipx,0,
				xpos-16+4,ypos-32,
				cliprect,TRANSPARENCY_PEN,15);
	}
}


VIDEO_UPDATE( kickgoal )
{
	/* set scroll */
	tilemap_set_scrollx( kickgoal_fgtm, 0, kickgoal_scrram[0]  );
	tilemap_set_scrolly( kickgoal_fgtm, 0, kickgoal_scrram[1]*2  );
	tilemap_set_scrollx( kickgoal_bgtm, 0, kickgoal_scrram[2]  );
	tilemap_set_scrolly( kickgoal_bgtm, 0, kickgoal_scrram[3]*2  );
	tilemap_set_scrollx( kickgoal_bg2tm, 0, kickgoal_scrram[4]  );
	tilemap_set_scrolly( kickgoal_bg2tm, 0, kickgoal_scrram[5]*2  );

	/* draw */
	tilemap_draw(bitmap,cliprect,kickgoal_bg2tm,0,0);
	tilemap_draw(bitmap,cliprect,kickgoal_bgtm,0,0);

	draw_sprites(bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,kickgoal_fgtm,0,0);

	/*
    popmessage ("Regs %04x %04x %04x %04x %04x %04x %04x %04x",
    kickgoal_scrram[0],
    kickgoal_scrram[1],
    kickgoal_scrram[2],
    kickgoal_scrram[3],
    kickgoal_scrram[4],
    kickgoal_scrram[5],
    kickgoal_scrram[6],
    kickgoal_scrram[7]);
    */
	return 0;
}

/* Holywood Action */

/* FG */
static void get_actionhw_fg_tile_info(int tile_index)
{
	int tileno = kickgoal_fgram[tile_index*2] & 0x0fff;
	int color = kickgoal_fgram[tile_index*2+1] & 0x000f;

	SET_TILE_INFO(0,tileno + 0x7000*2,color + 0x00,0)
}

/* BG */
static void get_actionhw_bg_tile_info(int tile_index)
{
	int tileno = kickgoal_bgram[tile_index*2] & 0x1fff;
	int color = kickgoal_bgram[tile_index*2+1] & 0x000f;
	int flipx = kickgoal_bgram[tile_index*2+1] & 0x0020;
	int flipy = kickgoal_bgram[tile_index*2+1] & 0x0040;

	SET_TILE_INFO(1,tileno + 0x0000,color + 0x10,(flipx ? TILE_FLIPX : 0) | (flipy ? TILE_FLIPY : 0))
}

/* BG 2 */
static void get_actionhw_bg2_tile_info(int tile_index)
{
	int tileno = kickgoal_bg2ram[tile_index*2] & 0x1fff;
	int color = kickgoal_bg2ram[tile_index*2+1] & 0x000f;
	int flipx = kickgoal_bg2ram[tile_index*2+1] & 0x0020;
	int flipy = kickgoal_bg2ram[tile_index*2+1] & 0x0040;

	SET_TILE_INFO(1,tileno + 0x2000,color + 0x20,(flipx ? TILE_FLIPX : 0) | (flipy ? TILE_FLIPY : 0))
}


static UINT32 tilemap_scan_actionhwbg2( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*16 + (row & 0xf) + ((row & 0x70) >> 4) * 0x400;
}

static UINT32 tilemap_scan_actionhwbg( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*16 + (row & 0xf) + ((row & 0x70) >> 4) * 0x400;
}

static UINT32 tilemap_scan_actionhwfg( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*32 + (row & 0x1f) + ((row & 0x20) >> 5) * 0x800;
}


VIDEO_START( actionhw )
{
	kickgoal_fgtm  = tilemap_create(get_actionhw_fg_tile_info,tilemap_scan_actionhwfg,TILEMAP_TRANSPARENT,  8, 8,64,64);
	kickgoal_bgtm  = tilemap_create(get_actionhw_bg_tile_info,tilemap_scan_actionhwbg,TILEMAP_TRANSPARENT, 16,16,64,64);
	kickgoal_bg2tm = tilemap_create(get_actionhw_bg2_tile_info,tilemap_scan_actionhwbg2,TILEMAP_OPAQUE,    16,16,64,64);

	tilemap_set_transparent_pen(kickgoal_fgtm,15);
	tilemap_set_transparent_pen(kickgoal_bgtm,15);

	return 0;
}


static void actionhw_draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	const gfx_element *gfx = Machine->gfx[1];
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int xpos = spriteram16[offs+3];
		int ypos = spriteram16[offs+0] & 0x00ff;
		int tileno = spriteram16[offs+2] & 0x3fff;
		int flipx = spriteram16[offs+1] & 0x0020;
		int color = spriteram16[offs+1] & 0x000f;

		if (spriteram16[offs+0] & 0x0100) break;

		ypos = 0x110-ypos;

		drawgfx(bitmap,gfx,
				tileno+0x4000,
				0x30 + color,
				flipx,0,
				xpos-16+4,ypos-32,
				cliprect,TRANSPARENCY_PEN,15);
	}
}


VIDEO_UPDATE( actionhw )
{
	/* set scroll */
	tilemap_set_scrollx( kickgoal_fgtm, 0, kickgoal_scrram[0]  );
	tilemap_set_scrolly( kickgoal_fgtm, 0, kickgoal_scrram[1]  );
	tilemap_set_scrollx( kickgoal_bgtm, 0, kickgoal_scrram[2]  );
	tilemap_set_scrolly( kickgoal_bgtm, 0, kickgoal_scrram[3]  );
	tilemap_set_scrollx( kickgoal_bg2tm, 0, kickgoal_scrram[4]  );
	tilemap_set_scrolly( kickgoal_bg2tm, 0, kickgoal_scrram[5]  );

	/* draw */
	tilemap_draw(bitmap,cliprect,kickgoal_bg2tm,0,0);
	tilemap_draw(bitmap,cliprect,kickgoal_bgtm,0,0);

	actionhw_draw_sprites(bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,kickgoal_fgtm,0,0);
	return 0;
}
