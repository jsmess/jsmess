/***************************************************************************

  TIA-MC1 video hardware

  driver by Eugene Sandulenko
  special thanks to Shiru for his standalone emulator and documentation

***************************************************************************/

#include "driver.h"


UINT8 *tiamc1_tileram;
UINT8 *tiamc1_charram;
UINT8 *tiamc1_spriteram_x;
UINT8 *tiamc1_spriteram_y;
UINT8 *tiamc1_spriteram_a;
UINT8 *tiamc1_spriteram_n;
UINT8 tiamc1_layers_ctrl;
UINT8 tiamc1_bg_vshift;
UINT8 tiamc1_bg_hshift;

UINT8 tiamc1_colormap[16];

static tilemap *bg_tilemap1, *bg_tilemap2;

WRITE8_HANDLER( tiamc1_videoram_w )
{
	if(!(tiamc1_layers_ctrl & 2))
		tiamc1_charram[offset + 0x0000] = data;
	if(!(tiamc1_layers_ctrl & 4))
		tiamc1_charram[offset + 0x0800] = data;
	if(!(tiamc1_layers_ctrl & 8))
		tiamc1_charram[offset + 0x1000] = data;
	if(!(tiamc1_layers_ctrl & 16))
		tiamc1_charram[offset + 0x1800] = data;

	if((tiamc1_layers_ctrl & 30) ^ 0xff)
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	if(!(tiamc1_layers_ctrl & 1)) {
		tiamc1_tileram[offset] = data;
		if (offset >= 1024)
			tilemap_mark_tile_dirty(bg_tilemap1, offset);
		else
			tilemap_mark_tile_dirty(bg_tilemap2, offset);
	}
}

WRITE8_HANDLER( tiamc1_bankswitch_w )
{
	if ((data & 128) != (tiamc1_layers_ctrl & 128))
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	tiamc1_layers_ctrl = data;
}

WRITE8_HANDLER( tiamc1_sprite_x_w )
{
	tiamc1_spriteram_x[offset] = data;
}

WRITE8_HANDLER( tiamc1_sprite_y_w )
{
	tiamc1_spriteram_y[offset] = data;
}

WRITE8_HANDLER( tiamc1_sprite_a_w )
{
	tiamc1_spriteram_a[offset] = data;
}

WRITE8_HANDLER( tiamc1_sprite_n_w )
{
	tiamc1_spriteram_n[offset] = data;
}

WRITE8_HANDLER( tiamc1_bg_vshift_w ) {
	tiamc1_bg_vshift = data;
}
WRITE8_HANDLER( tiamc1_bg_hshift_w ) {
	tiamc1_bg_hshift = data;
}

#define COLOR(gfxn,offs) (Machine->gfx[gfxn]->colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

WRITE8_HANDLER( tiamc1_palette_w )
{
	COLOR(0, offset) = data;
	COLOR(1, offset) = data;

	tiamc1_colormap[offset] = data;

	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

PALETTE_INIT( tiamc1 )
{
	// Voltage computed by Proteus
	//const float g_v[8]={1.05f,0.87f,0.81f,0.62f,0.44f,0.25f,0.19f,0.00f};
	//const float r_v[8]={1.37f,1.13f,1.00f,0.75f,0.63f,0.38f,0.25f,0.00f};
	//const float b_v[4]={1.16f,0.75f,0.42f,0.00f};

	// Voltage adjusted by Shiru
	const float g_v[8] = { 1.2071f,0.9971f,0.9259f,0.7159f,0.4912f,0.2812f,0.2100f,0.0000f};
	const float r_v[8] = { 1.5937f,1.3125f,1.1562f,0.8750f,0.7187f,0.4375f,0.2812f,0.0000f};
	const float b_v[4] = { 1.3523f,0.8750f,0.4773f,0.0000f};

	int col;
	int r, g, b, ir, ig, ib;
	float tcol;

	for (col = 0; col < 256; col++) {
		ir = (col >> 3) & 7;
		ig = col & 7;
		ib = (col >> 6) & 3;
		tcol = 255.0f * r_v[ir] / r_v[0];
		r = 255 - (((int)tcol) & 255);
		tcol = 255.0f * g_v[ig] / g_v[0];
		g = 255 - (((int)tcol) & 255);
		tcol = 255.0f * b_v[ib] / b_v[0];
		b = 255 - (((int)tcol) & 255);

		palette_set_color(machine,col,r,g,b);
	}
}

static void get_bg1_tile_info(int tile_index)
{
	int code = tiamc1_tileram[tile_index];

	decodechar(Machine->gfx[0], code, tiamc1_charram,
		   Machine->drv->gfxdecodeinfo[0].gfxlayout);

	SET_TILE_INFO(0, code, 0, 0)
}

static void get_bg2_tile_info(int tile_index)
{
	int code = tiamc1_tileram[tile_index + 1024];

	decodechar(Machine->gfx[0], code, tiamc1_charram,
		   Machine->drv->gfxdecodeinfo[0].gfxlayout);

	SET_TILE_INFO(0, code, 0, 0)
}

static void restore_colormap(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		COLOR(0, i) = tiamc1_colormap[i];
		COLOR(1, i) = tiamc1_colormap[i];
	}
}

VIDEO_START( tiamc1 )
{
	bg_tilemap1 = tilemap_create(get_bg1_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	bg_tilemap2 = tilemap_create(get_bg2_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	tiamc1_bg_vshift = 0;
	tiamc1_bg_hshift = 0;

	state_save_register_global(tiamc1_layers_ctrl);
	state_save_register_global(tiamc1_bg_vshift);
	state_save_register_global(tiamc1_bg_hshift);
	state_save_register_global_pointer(tiamc1_colormap, 16);

	state_save_register_func_postload(restore_colormap);

	return 0;
}

static void tiamc1_draw_sprites( mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0; offs < 16; offs++)
	{
		int flipx, flipy, sx, sy, spritecode;

		sx = tiamc1_spriteram_x[offs] ^ 0xff;
		sy = tiamc1_spriteram_y[offs] ^ 0xff;
		flipx = !(tiamc1_spriteram_a[offs] & 0x08);
		flipy = !(tiamc1_spriteram_a[offs] & 0x02);
		spritecode = tiamc1_spriteram_n[offs] ^ 0xff;

		if (!(tiamc1_spriteram_a[offs] & 0x01))
			drawgfx(bitmap, Machine->gfx[1],
				spritecode,
				0,
				flipx, flipy,
				sx, sy,
				&Machine->screen[0].visarea, TRANSPARENCY_PEN, 15);
	}
}

VIDEO_UPDATE( tiamc1 )
{
#if 0
	int i;

	for (i = 0; i < 32; i++)
	{
		tilemap_set_scrolly(bg_tilemap1, i, tiamc1_bg_vshift ^ 0xff);
		tilemap_set_scrolly(bg_tilemap2, i, tiamc1_bg_vshift ^ 0xff);
	}

	for (i = 0; i < 32; i++)
	{
		tilemap_set_scrollx(bg_tilemap1, i, tiamc1_bg_hshift ^ 0xff);
		tilemap_set_scrollx(bg_tilemap2, i, tiamc1_bg_hshift ^ 0xff);
	}
#endif

	if (tiamc1_layers_ctrl & 0x80)
		tilemap_draw(bitmap, &Machine->screen[0].visarea, bg_tilemap2, 0, 0);
	else
		tilemap_draw(bitmap, &Machine->screen[0].visarea, bg_tilemap1, 0, 0);


	tiamc1_draw_sprites(bitmap);

	return 0;
}

