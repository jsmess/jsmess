#include "driver.h"



UINT8 *lastday_txvideoram;
UINT16 *popbingo_scroll2;
static UINT8 bgscroll8[0x20], bg2scroll8[0x20], fgscroll8[0x20], fg2scroll8[0x20];
static UINT8 sprites_disabled;		/* Used by lastday/lastdaya */
static UINT8 flytiger_pri;			/* Used by flytiger */
static UINT8 tx_pri;				/* Used by sadari/gundl94/primella */
static UINT16 rshark_pri;			/* Used by rshark/superx/popbingo */

static tilemap *bg_tilemap, *bg2_tilemap;
static tilemap *fg_tilemap, *fg2_tilemap;
static tilemap *tx_tilemap;

INLINE void dooyong_scroll8_w(offs_t offset, UINT8 data, UINT8 *scroll, tilemap *map)
{
	UINT8 old = scroll[offset];
	if (old != data)
	{
		scroll[offset] = data;
		switch (offset)
		{
		case 0:
			if ((old ^ data) & 0x1f) tilemap_set_scrollx(map, 0, data & 0x1f);
			if ((old ^ data) & ~0x1f) tilemap_mark_all_tiles_dirty(map);
			break;
		case 1:
			tilemap_mark_all_tiles_dirty(map);
			break;
		case 3: /* Offset 4 is the high byte of the y scroll, but the tilemap is only 256 pixels wide */
			tilemap_set_scrolly(map, 0, data);
			break;
		case 6:	/* Not mapped for bluehawk/primella */
			tilemap_set_enable(map, !(data & 0x10));
			break;
		default:
			break;
		}
	}
}

INLINE void rshark_scroll16_w(offs_t offset, UINT16 data, UINT16 mem_mask, UINT8 *scroll, tilemap *map)
{
	/* This is an 8-bit peripheral in a 16-bit machine */
	if (ACCESSING_LSB)
	{
		UINT8 old = scroll[offset];
		if (old != (data & 0x00ff))
		{
			scroll[offset] = data & 0x00ff;
			switch (offset)
			{
			case 0:
				if ((old ^ (data & 0x00ff)) & 0x0f) tilemap_set_scrollx(map, 0, data & 0x0f);
				if ((old ^ (data & 0x00ff)) & ~0x0f) tilemap_mark_all_tiles_dirty(map);
				break;
			case 1:
				tilemap_mark_all_tiles_dirty(map);
				break;
			case 3:
			case 4:
				tilemap_set_scrolly(map, 0, ((int)scroll[3] | ((int)scroll[4] << 8)) & 0x01ff);
				break;
			case 6:	/* Not mapped, but here for consistency */
				tilemap_set_enable(map, !(data & 0x10));
				break;
			default:
				break;
			}
		}
	}
}

WRITE8_HANDLER( dooyong_bgscroll8_w )
{
	dooyong_scroll8_w(offset, data, bgscroll8, bg_tilemap);
}

WRITE8_HANDLER( dooyong_fgscroll8_w )
{
	dooyong_scroll8_w(offset, data, fgscroll8, fg_tilemap);
}

WRITE8_HANDLER( dooyong_fg2scroll8_w )
{
	dooyong_scroll8_w(offset, data, fg2scroll8, fg2_tilemap);
}

WRITE16_HANDLER( rshark_bgscroll16_w )
{
	rshark_scroll16_w(offset, data, mem_mask, bgscroll8, bg_tilemap);
}

WRITE16_HANDLER( rshark_bg2scroll16_w )
{
	rshark_scroll16_w(offset, data, mem_mask, bg2scroll8, bg2_tilemap);
}

WRITE16_HANDLER( rshark_fgscroll16_w )
{
	rshark_scroll16_w(offset, data, mem_mask, fgscroll8, fg_tilemap);
}

WRITE16_HANDLER( rshark_fg2scroll16_w )
{
	rshark_scroll16_w(offset, data, mem_mask, fg2scroll8, fg2_tilemap);
}

WRITE16_HANDLER( popbingo_bgscroll16_w )
{
	/* This is an 8-bit peripheral in a 16-bit machine */
	if (ACCESSING_LSB) dooyong_bgscroll8_w(offset, data & 0x00ff);
}

WRITE8_HANDLER( lastday_txvideoram8_w )
{
	if (lastday_txvideoram[offset] != data)
	{
		lastday_txvideoram[offset] = data;
		tilemap_mark_tile_dirty(tx_tilemap, offset & 0x07ff);
	}
}

WRITE8_HANDLER( bluehawk_txvideoram8_w )
{
	if (lastday_txvideoram[offset] != data)
	{
		lastday_txvideoram[offset] = data;
		tilemap_mark_tile_dirty(tx_tilemap, offset >> 1);
	}
}

WRITE8_HANDLER( lastday_ctrl_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 3 is used but unknown */

	/* bit 4 disables sprites */
	sprites_disabled = data & 0x10;

	/* bit 6 is flip screen */
	flip_screen_set(data & 0x40);
}

WRITE8_HANDLER( pollux_ctrl_w )
{
	/* bit 0 is flip screen */
	flip_screen_set(data & 0x01);

	/* bits 6 and 7 are coin counters */
	coin_counter_w(0,data & 0x80);
	coin_counter_w(1,data & 0x40);

	/* bit 1 is used but unknown */

	/* bit 2 is continuously toggled (unknown) */

	/* bit 4 is used but unknown */
}

WRITE8_HANDLER( primella_ctrl_w )
{
	/* bits 0-2 select ROM bank */
	memory_set_bank(1, data & 0x07);

	/* bit 3 disables tx layer */
	tx_pri = data & 0x08;

	/* bit 4 flips screen */
	flip_screen_set(data & 0x10);

	/* bit 5 used but unknown */

//  logerror("%04x: bankswitch = %02x\n",activecpu_get_pc(),data&0xe0);
}

WRITE8_HANDLER( flytiger_ctrl_w )
{
	/* bit 0 is flip screen */
	flip_screen_set(data & 0x01);

	/* bits 1, 2, 3 used but unknown */

	/* bit 4 changes tilemaps priority */
	flytiger_pri = data & 0x10;
}

WRITE16_HANDLER( rshark_ctrl_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0 flips screen */
		flip_screen_set(data & 0x0001);

		/* bit 4 changes tilemaps priority */
		rshark_pri = data & 0x0010;

		/* bit 5 used but unknown */
	}
}

INLINE void lastday_get_tile_info(running_machine *machine, tile_data *tileinfo, int tile_index,
		const UINT8 *tilerom, UINT8 *scroll, int graphics)
{
	int offs = (tile_index * 2) + ((((int)scroll[0] | ((int)scroll[1] << 8)) & ~0x001f) >> 1);
	int attr = tilerom[offs];
	int code = tilerom[offs + 1] | ((attr & 0x01) << 8) | ((attr & 0x80) << 2);
	int color = (attr & 0x78) >> 3;
	int flags = ((attr & 0x02) ? TILE_FLIPX : 0) | ((attr & 0x04) ? TILE_FLIPY : 0);

	SET_TILE_INFO(graphics, code, color, flags);
}

INLINE void bluehawk_get_tile_info(running_machine *machine, tile_data *tileinfo, int tile_index,
		const UINT8 *tilerom, UINT8 *scroll, int graphics)
{
	int offs = (tile_index * 2) + ((((int)scroll[0] | ((int)scroll[1] << 8)) & ~0x001f) >> 1);
	int attr = tilerom[offs];
	int code = tilerom[offs + 1] | ((attr & 0x03) << 8);
	int color = (attr & 0x3c) >> 2;
	int flags = ((attr & 0x40) ? TILE_FLIPX : 0) | ((attr & 0x80) ? TILE_FLIPY : 0);

	SET_TILE_INFO(graphics, code, color, flags);
}

INLINE void rshark_get_tile_info(running_machine *machine, tile_data *tileinfo, int tile_index,
		const UINT8 *tilerom1, const UINT8 *tilerom2, UINT8 *scroll, int graphics)
{
	int xoffs = ((int)scroll[0] | ((int)scroll[1] << 8)) & ~0x000f;
	int offs1 = (tile_index * 2) + (xoffs << 2);
	int offs2 = (tile_index * 1) + (xoffs << 1);
	int attr1 = tilerom1[offs1];
	int attr2 = tilerom2[offs2];
	int code = tilerom1[offs1 + 1] | ((attr1 & 0x1f) << 8);
	int color = attr2 & 0x0f;
	int flags = ((attr1 & 0x40) ? TILE_FLIPX : 0) | ((attr1 & 0x80) ? TILE_FLIPY : 0);

	SET_TILE_INFO(graphics, code, color, flags);
}

static TILE_GET_INFO( lastday_get_bg_tile_info )
{
	lastday_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX5), bgscroll8, 2);
}

static TILE_GET_INFO( lastday_get_fg_tile_info )
{
	lastday_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX6), fgscroll8, 3);
}

static TILE_GET_INFO( lastday_get_tx_tile_info )
{
	int attr = lastday_txvideoram[tile_index + 0x0800];
	int code = lastday_txvideoram[tile_index] | ((attr & 0x0f) << 8);
	int color = (attr & 0xf0) >> 4;

	SET_TILE_INFO(0, code, color, 0);
}

static TILE_GET_INFO( bluehawk_get_bg_tile_info )
{
	bluehawk_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX3) + 0x78000, bgscroll8, 2);
}

static TILE_GET_INFO( bluehawk_get_fg_tile_info )
{
	bluehawk_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX4) + 0x78000, fgscroll8, 3);
}

static TILE_GET_INFO( bluehawk_get_fg2_tile_info )
{
	const UINT8 *tilerom = memory_region(REGION_GFX5) + 0x38000;

	int offs = (tile_index * 2) + ((((int)fg2scroll8[0] | ((int)fg2scroll8[1] << 8)) & ~0x001f) >> 1);
	int attr = tilerom[offs];
	int code = tilerom[offs + 1] | ((attr & 0x01) << 8);
	int color = (attr & 0x78) >> 3;
	int flags = ((attr & 0x04) ? TILE_FLIPY : 0);

	SET_TILE_INFO(4, code, color, flags);
}

static TILE_GET_INFO( bluehawk_get_tx_tile_info )
{
	int offs = (tile_index * 2);
	int attr = lastday_txvideoram[offs + 1];
	int code = lastday_txvideoram[offs] | ((attr & 0x0f) << 8);
	int color = (attr & 0xf0) >> 4;

	SET_TILE_INFO(0, code, color, 0);
}

static TILE_GET_INFO( flytiger_get_bg_tile_info )
{
	lastday_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX3) + 0x78000, bgscroll8, 2);
}

static TILE_GET_INFO( flytiger_get_fg_tile_info )
{
	const UINT8 *tilerom = memory_region(REGION_GFX4) + 0x78000;

	int offs = (tile_index * 2) + ((((int)fgscroll8[0] | ((int)fgscroll8[1] << 8)) & ~0x001f) >> 1);
	int attr = tilerom[offs];
	int code = tilerom[offs + 1] | ((attr & 0x01) << 8) | ((attr & 0x80) << 2);
	int color = (attr & 0x78) >> 3; //TODO: missing 4th bit or palette bank
	int flags = ((attr & 0x02) ? TILE_FLIPX : 0) | ((attr & 0x04) ? TILE_FLIPY : 0);

	SET_TILE_INFO(3, code, color, flags);
}

static TILE_GET_INFO( primella_get_bg_tile_info )
{
	bluehawk_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX2) + memory_region_length(REGION_GFX2) - 0x8000, bgscroll8, 1);
}

static TILE_GET_INFO( primella_get_fg_tile_info )
{
	bluehawk_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX3) + memory_region_length(REGION_GFX3) - 0x8000, fgscroll8, 2);
}

static TILE_GET_INFO( rshark_get_bg_tile_info )
{
	rshark_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX5), memory_region(REGION_GFX6) + 0x60000, bgscroll8, 4);
}

static TILE_GET_INFO( rshark_get_bg2_tile_info )
{
	rshark_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX4), memory_region(REGION_GFX6) + 0x40000, bg2scroll8, 3);
}

static TILE_GET_INFO( rshark_get_fg_tile_info )
{
	rshark_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX3), memory_region(REGION_GFX6) + 0x20000, fgscroll8, 2);
}

static TILE_GET_INFO( rshark_get_fg2_tile_info )
{
	rshark_get_tile_info(machine, tileinfo, tile_index,
			memory_region(REGION_GFX2), memory_region(REGION_GFX6) + 0x00000, fg2scroll8, 1);
}

static TILE_GET_INFO( popbingo_get_bg_tile_info )
{
	const UINT8 *tilerom = memory_region(REGION_GFX2);

	int offs = (tile_index * 2) + ((((int)bgscroll8[0] | ((int)bgscroll8[1] << 8)) & ~0x001f) >> 1);
	int attr = tilerom[offs];
	int code = tilerom[offs + 1] | ((attr & 0x07) << 8);
	int color = 0;
	int flags = ((attr & 0x40) ? TILE_FLIPX : 0) | ((attr & 0x80) ? TILE_FLIPY : 0);

	SET_TILE_INFO(1, code, color, flags);
}

static void draw_sprites(mame_bitmap *bitmap,int pollux_extensions)
{
	int offs;

	for (offs = 0; offs < spriteram_size; offs += 32)
	{
		int sx,sy,code,color,pri;
		int flipx=0,flipy=0,height=0,y;

		sx = buffered_spriteram[offs+3] | ((buffered_spriteram[offs+1] & 0x10) << 4);
		sy = buffered_spriteram[offs+2];
		code = buffered_spriteram[offs] | ((buffered_spriteram[offs+1] & 0xe0) << 3);
		color = buffered_spriteram[offs+1] & 0x0f;
		//TODO: This priority mechanism works for known games, but seems a bit strange.
		//Are we missing something?
		pri = (((color == 0x00) || (color == 0x0f)) ? 0xfc : 0xf0);

		if (pollux_extensions)
		{
			/* gulfstrm, pollux, bluehawk */
			code |= ((buffered_spriteram[offs+0x1c] & 0x01) << 11);

			if (pollux_extensions >= 2)
			{
				/* pollux, bluehawk */
				height = (buffered_spriteram[offs+0x1c] & 0x70) >> 4;
				code &= ~height;

				if (pollux_extensions == 3)
				{
					/* bluehawk */
					sy += 6 - ((~buffered_spriteram[offs+0x1c] & 0x02) << 7);
					flipx = buffered_spriteram[offs+0x1c] & 0x08;
					flipy = buffered_spriteram[offs+0x1c] & 0x04;
				}

				if (pollux_extensions == 4)
				{
					/* flytiger */
					sy -=(buffered_spriteram[offs+0x1c] & 0x02) << 7;
					flipx = buffered_spriteram[offs+0x1c] & 0x08;
					flipy = buffered_spriteram[offs+0x1c] & 0x04;
				}
			}
		}

		if (flip_screen)
		{
			sx = 498 - sx;
			sy = 240-16*height - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		for (y = 0;y <= height;y++)
		{
			pdrawgfx(bitmap,Machine->gfx[1],
					code+y,
					color,
					flipx,flipy,
					sx,flipy ? sy + 16*(height-y) : sy + 16*y,
					&Machine->screen[0].visarea,TRANSPARENCY_PEN,15,
					pri);
		}
	}
}

static void rshark_draw_sprites(mame_bitmap *bitmap)
{
	int offs;

	for (offs = (spriteram_size/2)-8;offs >= 0;offs -= 8)
	{
		if (buffered_spriteram16[offs] & 0x0001)	/* enable */
		{
			int sx,sy,code,color,pri;
			int flipx=0,flipy=0,width,height,x,y;

			sx = buffered_spriteram16[offs+4] & 0x01ff;
			sy = (INT16)buffered_spriteram16[offs+6];
			code = buffered_spriteram16[offs+3];
			color = buffered_spriteram16[offs+7];
			//TODO: This priority mechanism works for known games, but seems a bit strange.
			//Are we missing something?
			pri = ((((color&0x0f) == 0x00) || ((color&0x0f) == 0x0f)) ? 0xfc : 0xf0);
			width = buffered_spriteram16[offs+1] & 0x000f;
			height = (buffered_spriteram16[offs+1] & 0x00f0) >> 4;

			if (flip_screen)
			{
				sx = 498-16*width - sx;
				sy = 240-16*height - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			for (y = 0;y <= height;y++)
			{
				for (x = 0;x <= width;x++)
				{
					pdrawgfx(bitmap,Machine->gfx[0],
							code,
							color,
							flipx,flipy,
							flipx ? sx + 16*(width-x) : sx + 16*x,
							flipy ? sy + 16*(height-y) : sy + 16*y,
							&Machine->screen[0].visarea,TRANSPARENCY_PEN,15,
							pri);

					code++;
				}
			}
		}
	}
}


VIDEO_UPDATE( lastday )
{
	fillbitmap(bitmap, get_black_pen(machine), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 1);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 2);
	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 4);

	if(!sprites_disabled)
		draw_sprites(bitmap,0);
	return 0;
}

VIDEO_UPDATE( gulfstrm )
{
	fillbitmap(bitmap, get_black_pen(machine), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 1);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 2);
	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 4);

	draw_sprites(bitmap,1);
	return 0;
}

VIDEO_UPDATE( pollux )
{
	fillbitmap(bitmap, get_black_pen(machine), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 1);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 2);
	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 4);

	draw_sprites(bitmap,2);
	return 0;
}

VIDEO_UPDATE( flytiger )
{
	fillbitmap(bitmap, get_black_pen(machine), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);

	if(flytiger_pri)
	{
		tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 1);
		tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 2);
	}
	else
	{
		tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 1);
		tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 2);
	}
	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 4);

	draw_sprites(bitmap,4);
	return 0;
}


VIDEO_UPDATE( bluehawk )
{
	fillbitmap(priority_bitmap, 0, cliprect);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 1);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 2);
	tilemap_draw(bitmap, cliprect, fg2_tilemap, 0, 4);
	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 4);

	draw_sprites(bitmap,3);
	return 0;
}

VIDEO_UPDATE( primella )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	if (tx_pri) tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
	if (!tx_pri) tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 0);
	return 0;
}

VIDEO_UPDATE( rshark )
{
	fillbitmap(priority_bitmap, 0, cliprect);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 1);
	tilemap_draw(bitmap, cliprect, bg2_tilemap, 0, (rshark_pri ? 2 : 1));
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 2);
	tilemap_draw(bitmap, cliprect, fg2_tilemap, 0, 2);

	rshark_draw_sprites(bitmap);
	return 0;
}

VIDEO_UPDATE( popbingo )
{
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 1);
	rshark_draw_sprites(bitmap);
	return 0;
}

VIDEO_START( lastday )
{
	bg_tilemap = tilemap_create(lastday_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 32, 32, 16, 8);
	fg_tilemap = tilemap_create(lastday_get_fg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	tx_tilemap = tilemap_create(lastday_get_tx_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(tx_tilemap, 15);

	tilemap_set_scrolly(tx_tilemap, 0, 8);

	state_save_register_global_array(bgscroll8);
	state_save_register_global_array(fgscroll8);
	state_save_register_global(sprites_disabled);

	return 0;
}

VIDEO_START( gulfstrm )
{
	bg_tilemap = tilemap_create(lastday_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 32, 32, 16, 8);
	fg_tilemap = tilemap_create(lastday_get_fg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	tx_tilemap = tilemap_create(lastday_get_tx_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(tx_tilemap, 15);

	tilemap_set_scrolly(tx_tilemap, 0, 8);

	state_save_register_global_array(bgscroll8);
	state_save_register_global_array(fgscroll8);

	return 0;
}

VIDEO_START( pollux )
{
	bg_tilemap = tilemap_create(lastday_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 32, 32, 16, 8);
	fg_tilemap = tilemap_create(lastday_get_fg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	tx_tilemap = tilemap_create(lastday_get_tx_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(tx_tilemap, 15);

	state_save_register_global_array(bgscroll8);
	state_save_register_global_array(fgscroll8);

	return 0;
}

VIDEO_START( bluehawk )
{
	bg_tilemap = tilemap_create(bluehawk_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 32, 32, 16, 8);
	fg_tilemap = tilemap_create(bluehawk_get_fg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	fg2_tilemap = tilemap_create(bluehawk_get_fg2_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	tx_tilemap = tilemap_create(bluehawk_get_tx_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(fg2_tilemap, 15);
	tilemap_set_transparent_pen(tx_tilemap, 15);

	state_save_register_global_array(bgscroll8);
	state_save_register_global_array(fgscroll8);
	state_save_register_global_array(fg2scroll8);

	return 0;
}

VIDEO_START( flytiger )
{
	bg_tilemap = tilemap_create(flytiger_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	fg_tilemap = tilemap_create(flytiger_get_fg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	tx_tilemap = tilemap_create(lastday_get_tx_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(bg_tilemap, 15);
	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(tx_tilemap, 15);

	state_save_register_global_array(bgscroll8);
	state_save_register_global_array(fgscroll8);
	state_save_register_global(flytiger_pri);

	return 0;
}

VIDEO_START( primella )
{
	memory_configure_bank(1, 0, 8, memory_region(REGION_CPU1) + 0x10000, 0x4000);

	bg_tilemap = tilemap_create(primella_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 32, 32, 16, 8);
	fg_tilemap = tilemap_create(primella_get_fg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 32, 32, 16, 8);
	tx_tilemap = tilemap_create(bluehawk_get_tx_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(tx_tilemap, 15);

	state_save_register_global_array(bgscroll8);
	state_save_register_global_array(fgscroll8);
	state_save_register_global(tx_pri);

	return 0;
}

VIDEO_START( rshark )
{
	bg_tilemap = tilemap_create(rshark_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 16, 16, 32, 32);
	bg2_tilemap = tilemap_create(rshark_get_bg2_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	fg_tilemap = tilemap_create(rshark_get_fg_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	fg2_tilemap = tilemap_create(rshark_get_fg2_tile_info, tilemap_scan_cols,
		TILEMAP_TRANSPARENT, 16, 16, 32, 32);

	tilemap_set_transparent_pen(bg2_tilemap, 15);
	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(fg2_tilemap, 15);

	state_save_register_global_array(bgscroll8);
	state_save_register_global_array(bg2scroll8);
	state_save_register_global_array(fgscroll8);
	state_save_register_global_array(fg2scroll8);
	state_save_register_global(rshark_pri);

	return 0;
}

VIDEO_START( popbingo )
{
	bg_tilemap = tilemap_create(popbingo_get_bg_tile_info, tilemap_scan_cols,
		TILEMAP_OPAQUE, 32, 32, 16, 8);

	state_save_register_global_array(bgscroll8);
	state_save_register_global(rshark_pri);

	return 0;
}

VIDEO_EOF( dooyong )
{
	buffer_spriteram_w(0,0);
}

VIDEO_EOF( rshark )
{
	buffer_spriteram16_w(0,0,0);
}
