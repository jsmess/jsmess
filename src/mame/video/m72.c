#include "driver.h"
#include "audio/m72.h"



UINT8 *m72_videoram1,*m72_videoram2,*majtitle_rowscrollram;
static UINT8 *m72_spriteram;
static INT32 splitline;
static tilemap *fg_tilemap,*bg_tilemap;
static int xadjust;
static int bgadjust;
static INT32 scrollx1,scrolly1,scrollx2,scrolly2;
static INT32 video_off;

static int irqbase;

MACHINE_RESET( m72 )
{
	irqbase = 0x20;
	machine_reset_m72_sound(machine);
}

MACHINE_RESET( xmultipl )
{
	irqbase = 0x08;
	machine_reset_m72_sound(machine);
}

MACHINE_RESET( kengo )
{
	irqbase = 0x18;
	machine_reset_m72_sound(machine);
}

INTERRUPT_GEN( m72_interrupt )
{
	int line = 255 - cpu_getiloops();

	if (line == 255)	/* vblank */
	{
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, irqbase+0);
	}
	else
	{
		if (line != splitline - 128)
			return;

		video_screen_update_partial(0, line + 128);

		/* this is used to do a raster effect and show the score display at
           the bottom of the screen or other things. The line where the
           interrupt happens is programmable (and the interrupt can be triggered
           multiple times, by changing the interrupt line register in the
           interrupt handler).
         */
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, irqbase+2);
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void m72_get_tile_info(int tile_index,unsigned char *vram,int gfxnum)
{
	int code,attr,color,pri;

	tile_index *= 4;

	code  = vram[tile_index];
	attr  = vram[tile_index+1];
	color = vram[tile_index+2];

	if (color & 0x80) pri = 2;
	else if (color & 0x40) pri = 1;
	else pri = 0;
/* color & 0x10 is used in bchopper and hharry, more priority? */

	SET_TILE_INFO(
			gfxnum,
			code + ((attr & 0x3f) << 8),
			color & 0x0f,
			TILE_FLIPYX((attr & 0xc0) >> 6) | TILE_SPLIT(pri))
}

INLINE void rtype2_get_tile_info(int tile_index,unsigned char *vram,int gfxnum)
{
	int code,attr,color,pri;

	tile_index *= 4;

	code  = vram[tile_index] + (vram[tile_index+1] << 8);
	color = vram[tile_index+2];
	attr  = vram[tile_index+3];

	if (attr & 0x01) pri = 2;
	else if (color & 0x80) pri = 1;
	else pri = 0;

/* (vram[tile_index+2] & 0x10) is used by majtitle on the green, but it's not clear for what */
/* (vram[tile_index+3] & 0xfe) are used as well */

	SET_TILE_INFO(
			gfxnum,
			code,
			color & 0x0f,
			TILE_FLIPYX((color & 0x60) >> 5) | TILE_SPLIT(pri))
}


static void m72_get_bg_tile_info(int tile_index)
{
	m72_get_tile_info(tile_index,m72_videoram2,2);
}

static void m72_get_fg_tile_info(int tile_index)
{
	m72_get_tile_info(tile_index,m72_videoram1,1);
}

static void hharry_get_bg_tile_info(int tile_index)
{
	m72_get_tile_info(tile_index,m72_videoram2,1);
}

static void rtype2_get_bg_tile_info(int tile_index)
{
	rtype2_get_tile_info(tile_index,m72_videoram2,1);
}

static void rtype2_get_fg_tile_info(int tile_index)
{
	rtype2_get_tile_info(tile_index,m72_videoram1,1);
}


static UINT32 majtitle_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*256 + col;
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static void register_savestate(void)
{
	state_save_register_global(splitline);
	state_save_register_global(video_off);
	state_save_register_global(scrollx1);
	state_save_register_global(scrolly1);
	state_save_register_global(scrollx2);
	state_save_register_global(scrolly2);
	state_save_register_global_pointer(m72_spriteram, spriteram_size);
}


VIDEO_START( m72 )
{
	bg_tilemap = tilemap_create(m72_get_bg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,64,64);
	fg_tilemap = tilemap_create(m72_get_fg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = auto_malloc(spriteram_size);

	tilemap_set_transmask(fg_tilemap,0,0xffff,0x0001);
	tilemap_set_transmask(fg_tilemap,1,0x00ff,0xff01);
	tilemap_set_transmask(fg_tilemap,2,0x0001,0xffff);

	tilemap_set_transmask(bg_tilemap,0,0xffff,0x0000);
	tilemap_set_transmask(bg_tilemap,1,0x00ff,0xff00);
	tilemap_set_transmask(bg_tilemap,2,0x0001,0xfffe);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = 0;
	bgadjust = 0;

	register_savestate();

	return 0;
}

VIDEO_START( rtype2 )
{
	bg_tilemap = tilemap_create(rtype2_get_bg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,64,64);
	fg_tilemap = tilemap_create(rtype2_get_fg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = auto_malloc(spriteram_size);

	tilemap_set_transmask(fg_tilemap,0,0xffff,0x0001);
	tilemap_set_transmask(fg_tilemap,1,0x00ff,0xff01);
	tilemap_set_transmask(fg_tilemap,2,0x0001,0xffff);

	tilemap_set_transmask(bg_tilemap,0,0xffff,0x0000);
	tilemap_set_transmask(bg_tilemap,1,0x00ff,0xff00);
	tilemap_set_transmask(bg_tilemap,2,0x0001,0xfffe);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = -4;
	bgadjust = 0;

	register_savestate();

	return 0;
}

VIDEO_START( poundfor )
{
	int res = video_start_rtype2(machine);

	xadjust = -6;

	return res;
}


/* Major Title has a larger background RAM, and rowscroll */
VIDEO_START( majtitle )
{
// The tilemap can be 256x64, but seems to be used at 128x64 (scroll wraparound).
// The layout ramains 256x64, the right half is just not displayed.
//  bg_tilemap = tilemap_create(rtype2_get_bg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,256,64);
	bg_tilemap = tilemap_create(rtype2_get_bg_tile_info,majtitle_scan_rows,TILEMAP_SPLIT,8,8,128,64);
	fg_tilemap = tilemap_create(rtype2_get_fg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = auto_malloc(spriteram_size);

	tilemap_set_transmask(fg_tilemap,0,0xffff,0x0001);
	tilemap_set_transmask(fg_tilemap,1,0x00ff,0xff01);
	tilemap_set_transmask(fg_tilemap,2,0x0001,0xffff);

	tilemap_set_transmask(bg_tilemap,0,0xffff,0x0000);
	tilemap_set_transmask(bg_tilemap,1,0x00ff,0xff00);
	tilemap_set_transmask(bg_tilemap,2,0x0001,0xfffe);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = -4;
	bgadjust = 0;

	register_savestate();

	return 0;
}

VIDEO_START( hharry )
{
	bg_tilemap = tilemap_create(hharry_get_bg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,64,64);
	fg_tilemap = tilemap_create(m72_get_fg_tile_info,   tilemap_scan_rows,TILEMAP_SPLIT,8,8,64,64);

	m72_spriteram = auto_malloc(spriteram_size);

	tilemap_set_transmask(fg_tilemap,0,0xffff,0x0001);
	tilemap_set_transmask(fg_tilemap,1,0x00ff,0xff01);
	tilemap_set_transmask(fg_tilemap,2,0x0001,0xffff);

	tilemap_set_transmask(bg_tilemap,0,0xffff,0x0000);
	tilemap_set_transmask(bg_tilemap,1,0x00ff,0xff00);
	tilemap_set_transmask(bg_tilemap,2,0x0001,0xfffe);

	memset(m72_spriteram,0,spriteram_size);

	xadjust = -4;
	bgadjust = -2;

	register_savestate();

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

READ8_HANDLER( m72_palette1_r )
{
	/* only D0-D4 are connected */
	if (offset & 1) return 0xff;

	/* A9 isn't connected, so 0x200-0x3ff mirrors 0x000-0x1ff etc. */
	offset &= ~0x200;

	return paletteram[offset] | 0xe0;	/* only D0-D4 are connected */
}

READ8_HANDLER( m72_palette2_r )
{
	/* only D0-D4 are connected */
	if (offset & 1) return 0xff;

	/* A9 isn't connected, so 0x200-0x3ff mirrors 0x000-0x1ff etc. */
	offset &= ~0x200;

	return paletteram_2[offset] | 0xe0;	/* only D0-D4 are connected */
}

INLINE void changecolor(int color,int r,int g,int b)
{
	palette_set_color(Machine,color,pal5bit(r),pal5bit(g),pal5bit(b));
}

WRITE8_HANDLER( m72_palette1_w )
{
	/* only D0-D4 are connected */
	if (offset & 1) return;

	/* A9 isn't connected, so 0x200-0x3ff mirrors 0x000-0x1ff etc. */
	offset &= ~0x200;

	paletteram[offset] = data;
	offset &= 0x1ff;
	changecolor(offset / 2,
			paletteram[offset + 0x000],
			paletteram[offset + 0x400],
			paletteram[offset + 0x800]);
}

WRITE8_HANDLER( m72_palette2_w )
{
	/* only D0-D4 are connected */
	if (offset & 1) return;

	/* A9 isn't connected, so 0x200-0x3ff mirrors 0x000-0x1ff etc. */
	offset &= ~0x200;

	paletteram_2[offset] = data;
	offset &= 0x1ff;
	changecolor(offset / 2 + 256,
			paletteram_2[offset + 0x000],
			paletteram_2[offset + 0x400],
			paletteram_2[offset + 0x800]);
}

READ8_HANDLER( m72_videoram1_r )
{
	return m72_videoram1[offset];
}

READ8_HANDLER( m72_videoram2_r )
{
	return m72_videoram2[offset];
}

WRITE8_HANDLER( m72_videoram1_w )
{
	if (m72_videoram1[offset] != data)
	{
		m72_videoram1[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset/4);
	}
}

WRITE8_HANDLER( m72_videoram2_w )
{
	if (m72_videoram2[offset] != data)
	{
		m72_videoram2[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset/4);
	}
}

WRITE8_HANDLER( m72_irq_line_w )
{
	offset *= 8;
	splitline = (splitline & (0xff00 >> offset)) | (data << offset);
}

WRITE8_HANDLER( m72_scrollx1_w )
{
	offset *= 8;
	scrollx1 = (scrollx1 & (0xff00 >> offset)) | (data << offset);
}

WRITE8_HANDLER( m72_scrollx2_w )
{
	offset *= 8;
	scrollx2 = (scrollx2 & (0xff00 >> offset)) | (data << offset);
}

WRITE8_HANDLER( m72_scrolly1_w )
{
	offset *= 8;
	scrolly1 = (scrolly1 & (0xff00 >> offset)) | (data << offset);
}

WRITE8_HANDLER( m72_scrolly2_w )
{
	offset *= 8;
	scrolly2 = (scrolly2 & (0xff00 >> offset)) | (data << offset);
}

WRITE8_HANDLER( m72_dmaon_w )
{
	if (offset == 0)
	{
		memcpy(m72_spriteram,spriteram,spriteram_size);
	}
}


WRITE8_HANDLER( m72_port02_w )
{
	if (offset != 0)
	{
		if (data) logerror("write %02x to port 03\n",data);
		return;
	}
	if (data & 0xe0) logerror("write %02x to port 02\n",data);

	/* bits 0/1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 2 is flip screen (handled both by software and hardware) */
	flip_screen_set(((data & 0x04) >> 2) ^ (~readinputport(5) & 1));

	/* bit 3 is display disable */
	video_off = data & 0x08;

	/* bit 4 resets sound CPU (active low) */
	if (data & 0x10)
		cpunum_set_input_line(1, INPUT_LINE_RESET, CLEAR_LINE);
	else
		cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);

	/* bit 5 = "bank"? */
}

WRITE8_HANDLER( rtype2_port02_w )
{
	if (offset != 0)
	{
		if (data) logerror("write %02x to port 03\n",data);
		return;
	}
	if (data & 0xe0) logerror("write %02x to port 02\n",data);

	/* bits 0/1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 2 is flip screen (handled both by software and hardware) */
	flip_screen_set(((data & 0x04) >> 2) ^ (~readinputport(5) & 1));

	/* bit 3 is display disable */
	video_off = data & 0x08;

	/* other bits unknown */
}


static int majtitle_rowscroll;

/* the following is mostly a kludge. This register seems to be used for something else */
WRITE8_HANDLER( majtitle_gfx_ctrl_w )
{
	if (offset == 1)
	{
		if (data) majtitle_rowscroll = 1;
		else majtitle_rowscroll = 0;
	}
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;

	offs = 0;
	while (offs < spriteram_size)
	{
		int code,color,sx,sy,flipx,flipy,w,h,x,y;


		code = m72_spriteram[offs+2] | (m72_spriteram[offs+3] << 8);
		color = m72_spriteram[offs+4] & 0x0f;
		sx = -256+(m72_spriteram[offs+6] | ((m72_spriteram[offs+7] & 0x03) << 8));
		sy = 512-(m72_spriteram[offs+0] | ((m72_spriteram[offs+1] & 0x01) << 8));
		flipx = m72_spriteram[offs+5] & 0x08;
		flipy = m72_spriteram[offs+5] & 0x04;

		w = 1 << ((m72_spriteram[offs+5] & 0xc0) >> 6);
		h = 1 << ((m72_spriteram[offs+5] & 0x30) >> 4);
		sy -= 16 * h;

		if (flip_screen)
		{
			sx = 512 - 16*w - sx;
			sy = 512 - 16*h - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		for (x = 0;x < w;x++)
		{
			for (y = 0;y < h;y++)
			{
				int c = code;

				if (flipx) c += 8*(w-1-x);
				else c += 8*x;
				if (flipy) c += h-1-y;
				else c += y;

				drawgfx(bitmap,Machine->gfx[0],
						c,
						color,
						flipx,flipy,
						sx + 16*x,sy + 16*y,
						cliprect,TRANSPARENCY_PEN,0);
			}
		}

		offs += w*8;
	}
}

static void majtitle_draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int code,color,sx,sy,flipx,flipy,w,h,x,y;


		code = spriteram_2[offs+2] | (spriteram_2[offs+3] << 8);
		color = spriteram_2[offs+4] & 0x0f;
		sx = -256+(spriteram_2[offs+6] | ((spriteram_2[offs+7] & 0x03) << 8));
		sy = 512-(spriteram_2[offs+0] | ((spriteram_2[offs+1] & 0x01) << 8));
		flipx = spriteram_2[offs+5] & 0x08;
		flipy = spriteram_2[offs+5] & 0x04;

		w = 1;// << ((spriteram_2[offs+5] & 0xc0) >> 6);
		h = 1 << ((spriteram_2[offs+5] & 0x30) >> 4);
		sy -= 16 * h;

		if (flip_screen)
		{
			sx = 512 - 16*w - sx;
			sy = 512 - 16*h - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		for (x = 0;x < w;x++)
		{
			for (y = 0;y < h;y++)
			{
				int c = code;

				if (flipx) c += 8*(w-1-x);
				else c += 8*x;
				if (flipy) c += h-1-y;
				else c += y;

				drawgfx(bitmap,Machine->gfx[2],
						c,
						color,
						flipx,flipy,
						sx + 16*x,sy + 16*y,
						cliprect,TRANSPARENCY_PEN,0);
			}
		}
	}
}

VIDEO_UPDATE( m72 )
{
	if (video_off)
	{
		fillbitmap(bitmap,get_black_pen(machine),cliprect);
		return 0;
	}

	tilemap_set_scrollx(fg_tilemap,0,scrollx1 + xadjust);
	tilemap_set_scrolly(fg_tilemap,0,scrolly1);

	tilemap_set_scrollx(bg_tilemap,0,scrollx2 + xadjust + bgadjust);
	tilemap_set_scrolly(bg_tilemap,0,scrolly2);

	tilemap_draw(bitmap,cliprect,bg_tilemap,TILEMAP_BACK,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,TILEMAP_BACK,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,bg_tilemap,TILEMAP_FRONT,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,TILEMAP_FRONT,0);
	return 0;
}

VIDEO_UPDATE( majtitle )
{
	int i;


	if (video_off)
	{
		fillbitmap(bitmap,get_black_pen(machine),cliprect);
		return 0;
	}

	tilemap_set_scrollx(fg_tilemap,0,scrollx1 + xadjust);
	tilemap_set_scrolly(fg_tilemap,0,scrolly1);

	if (majtitle_rowscroll)
	{
		tilemap_set_scroll_rows(bg_tilemap,512);
		for (i = 0;i < 512;i++)
			tilemap_set_scrollx(bg_tilemap,(i+scrolly2)&0x1ff,
					256 + majtitle_rowscrollram[2*i] + (majtitle_rowscrollram[2*i+1] << 8) + xadjust);
	}
	else
	{
		tilemap_set_scroll_rows(bg_tilemap,1);
		tilemap_set_scrollx(bg_tilemap,0,256 + scrollx2 + xadjust);
	}
	tilemap_set_scrolly(bg_tilemap,0,scrolly2);

	tilemap_draw(bitmap,cliprect,bg_tilemap,TILEMAP_BACK,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,TILEMAP_BACK,0);
	majtitle_draw_sprites(bitmap,cliprect);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,bg_tilemap,TILEMAP_FRONT,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,TILEMAP_FRONT,0);
	return 0;
}
