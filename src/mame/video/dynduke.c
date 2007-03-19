
#include "driver.h"

static tilemap *bg_layer,*fg_layer,*tx_layer;
UINT8 *dynduke_back_data,*dynduke_fore_data,*dynduke_scroll_ram;

static int back_bankbase,fore_bankbase,back_palbase;
static int back_enable,fore_enable,sprite_enable;

/******************************************************************************/

WRITE8_HANDLER( dynduke_paletteram_w )
{
	int color;

	paletteram[offset]=data;
	color=paletteram[offset&0xffe]|(paletteram[offset|1]<<8);
	palette_set_color(Machine,offset/2,pal4bit(color >> 0),pal4bit(color >> 4),pal4bit(color >> 8));

	/* This is a kludge to handle 5bpp graphics but 4bpp palette data */
	/* the 5th bit is actually transparency, so I should use TILEMAP_BITMASK */
	if (offset<1024) {
		palette_set_color(Machine,((offset&0x1f)/2) | (offset&0xffe0) | 2048,pal4bit(color >> 0),pal4bit(color >> 4),pal4bit(color >> 8));
		palette_set_color(Machine,((offset&0x1f)/2) | (offset&0xffe0) | 2048 | 16,pal4bit(color >> 0),pal4bit(color >> 4),pal4bit(color >> 8));
	}
}

WRITE8_HANDLER( dynduke_background_w )
{
	dynduke_back_data[offset]=data;
	tilemap_mark_tile_dirty(bg_layer,offset/2);
}

WRITE8_HANDLER( dynduke_foreground_w )
{
	dynduke_fore_data[offset]=data;
	tilemap_mark_tile_dirty(fg_layer,offset/2);
}

WRITE8_HANDLER( dynduke_text_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty(tx_layer,offset/2);
}

static void get_bg_tile_info(int tile_index)
{
	int tile=dynduke_back_data[2*tile_index]+(dynduke_back_data[2*tile_index+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			1,
			tile+back_bankbase,
			color+back_palbase,
			0)
}

static void get_fg_tile_info(int tile_index)
{
	int tile=dynduke_fore_data[2*tile_index]+(dynduke_fore_data[2*tile_index+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			2,
			tile+fore_bankbase,
			color,
			0)
}

static void get_tx_tile_info(int tile_index)
{
	int tile=videoram[2*tile_index]+((videoram[2*tile_index+1]&0xc0)<<2);
	int color=videoram[2*tile_index+1]&0xf;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

VIDEO_START( dynduke )
{
	bg_layer = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_SPLIT,      16,16,32,32);
	fg_layer = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,32,32);
	tx_layer = tilemap_create(get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);

	tilemap_set_transmask(bg_layer,0,0x0000ffff,0xffff0000); /* 4bpp + The rest - 1bpp */

	tilemap_set_transparent_pen(fg_layer,15);
	tilemap_set_transparent_pen(tx_layer,15);

	return 0;
}

WRITE8_HANDLER( dynduke_gfxbank_w )
{
	static int old_back,old_fore;

	if (data&0x01) back_bankbase=0x1000; else back_bankbase=0;
	if (data&0x10) fore_bankbase=0x1000; else fore_bankbase=0;

	if (back_bankbase!=old_back)
		tilemap_mark_all_tiles_dirty(bg_layer);
	if (fore_bankbase!=old_fore)
		tilemap_mark_all_tiles_dirty(fg_layer);

	old_back=back_bankbase;
	old_fore=fore_bankbase;
}

WRITE8_HANDLER( dynduke_control_w )
{
	static int old_bpal;

	if (data&0x1) back_enable=0; else back_enable=1;
	if (data&0x2) back_palbase=16; else back_palbase=0;
	if (data&0x4) fore_enable=0; else fore_enable=1;
	if (data&0x8) sprite_enable=0; else sprite_enable=1;

	if (back_palbase!=old_bpal)
		tilemap_mark_all_tiles_dirty(bg_layer);

	old_bpal=back_palbase;
	flip_screen_set(data & 0x40);
}

static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect,int pri)
{
	int offs,fx,fy,x,y,color,sprite;

	if (!sprite_enable) return;

	for (offs = 0x1000-8;offs >= 0;offs -= 8)
	{
		/* Don't draw empty sprite table entries */
		if (buffered_spriteram[offs+7]!=0xf) continue;
		if (((buffered_spriteram[offs+5]>>5)&3)!=pri) continue;

		fx= buffered_spriteram[offs+1]&0x20;
		fy= buffered_spriteram[offs+1]&0x40;
		y = buffered_spriteram[offs+0];
		x = buffered_spriteram[offs+4];

		if (buffered_spriteram[offs+5]&1) x=0-(0x100-x);

		color = buffered_spriteram[offs+1]&0x1f;
		sprite = buffered_spriteram[offs+2]+(buffered_spriteram[offs+3]<<8);
		sprite &= 0x3fff;

		if (flip_screen) {
			x=240-x;
			y=240-y;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
		}

		drawgfx(bitmap,Machine->gfx[3],
				sprite,
				color,fx,fy,x,y,
				cliprect,TRANSPARENCY_PEN,15);
	}
}

VIDEO_UPDATE( dynduke )
{
	/* Setup the tilemaps */
	tilemap_set_scrolly( bg_layer,0, ((dynduke_scroll_ram[0x02]&0x30)<<4)+((dynduke_scroll_ram[0x04]&0x7f)<<1)+((dynduke_scroll_ram[0x04]&0x80)>>7) );
	tilemap_set_scrollx( bg_layer,0, ((dynduke_scroll_ram[0x12]&0x30)<<4)+((dynduke_scroll_ram[0x14]&0x7f)<<1)+((dynduke_scroll_ram[0x14]&0x80)>>7) );
	tilemap_set_scrolly( fg_layer,0, ((dynduke_scroll_ram[0x22]&0x30)<<4)+((dynduke_scroll_ram[0x24]&0x7f)<<1)+((dynduke_scroll_ram[0x24]&0x80)>>7) );
	tilemap_set_scrollx( fg_layer,0, ((dynduke_scroll_ram[0x32]&0x30)<<4)+((dynduke_scroll_ram[0x34]&0x7f)<<1)+((dynduke_scroll_ram[0x34]&0x80)>>7) );
	tilemap_set_enable( bg_layer,back_enable);
	tilemap_set_enable( fg_layer,fore_enable);

	if (back_enable)
		tilemap_draw(bitmap,cliprect,bg_layer,TILEMAP_BACK,0);
	else
		fillbitmap(bitmap,get_black_pen(machine),cliprect);

	draw_sprites(bitmap,cliprect,0); // Untested: does anything use it? Could be behind background
	draw_sprites(bitmap,cliprect,1);
	tilemap_draw(bitmap,cliprect,bg_layer,TILEMAP_FRONT,0);
	draw_sprites(bitmap,cliprect,2);
	tilemap_draw(bitmap,cliprect,fg_layer,0,0);
	draw_sprites(bitmap,cliprect,3);
	tilemap_draw(bitmap,cliprect,tx_layer,0,0);
	return 0;
}

VIDEO_EOF( dynduke )
{
	buffer_spriteram_w(0,0); // Could be a memory location instead
}
