/***************************************************************************

  gb.c

  Video file to handle emulation of the Nintendo GameBoy.

  Original code                               Carsten Sorensen   1998
  Mess modifications, bug fixes and speedups  Hans de Goede      1998
  Bug fixes, SGB and GBC code                 Anthony Kruize     2002

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/gb.h"
#include "cpu/z80gb/z80gb.h"
#include "profiler.h"

#define LCDCONT		gb_vid_regs[0x00]	/* LCD control register                       */
#define LCDSTAT		gb_vid_regs[0x01]	/* LCD status register                        */
#define SCROLLY		gb_vid_regs[0x02]	/* Starting Y position of the background      */
#define SCROLLX		gb_vid_regs[0x03]	/* Starting X position of the background      */
#define CURLINE		gb_vid_regs[0x04]	/* Current screen line being scanned          */
#define CMPLINE		gb_vid_regs[0x05]	/* Gen. int. when scan reaches this line      */
#define BGRDPAL		gb_vid_regs[0x07]	/* Background palette                         */
#define SPR0PAL		gb_vid_regs[0x08]	/* Sprite palette #0                          */
#define SPR1PAL		gb_vid_regs[0x09]	/* Sprite palette #1                          */
#define WNDPOSY		gb_vid_regs[0x0A]	/* Window Y position                          */
#define WNDPOSX		gb_vid_regs[0x0B]	/* Window X position                          */
#define KEY1		gb_vid_regs[0x0D]	/* Prepare speed switch                       */
#define HDMA1		gb_vid_regs[0x11]	/* HDMA source high byte                      */
#define HDMA2		gb_vid_regs[0x12]	/* HDMA source low byte                       */
#define HDMA3		gb_vid_regs[0x13]	/* HDMA destination high byte                 */
#define HDMA4		gb_vid_regs[0x14]	/* HDMA destination low byte                  */
#define HDMA5		gb_vid_regs[0x15]	/* HDMA length/mode/start                     */
#define GBCBCPS		gb_vid_regs[0x28]	/* Backgound palette spec                     */
#define GBCBCPD		gb_vid_regs[0x29]	/* Backgound palette data                     */
#define GBCOCPS		gb_vid_regs[0x2A]	/* Object palette spec                        */
#define GBCOCPD		gb_vid_regs[0x2B]	/* Object palette data                        */

#define _NR_GB_VID_REGS		0x40

static UINT8 bg_zbuf[160];
UINT8 gb_vid_regs[_NR_GB_VID_REGS];
UINT8 gb_bpal[4];	/* Background palette		*/
UINT8 gb_spal0[4];	/* Sprite 0 palette		*/
UINT8 gb_spal1[4];	/* Sprite 1 palette		*/
UINT8 *gb_oam = NULL;
UINT8 *gb_vram = NULL;
int gbc_hdma_enabled;
UINT8	*gb_chrgen;	/* Character generator           */
UINT8	*gb_bgdtab;	/* Background character table    */
UINT8	*gb_wndtab;	/* Window character table        */
UINT8	gb_tile_no_mod;
UINT8	*gbc_chrgen;	/* Character generator           */
UINT8	*gbc_bgdtab;	/* Background character table    */
UINT8	*gbc_wndtab;	/* Window character table        */

struct layer_struct {
	UINT8  enabled;
	UINT8  *bg_tiles;
	UINT8  *bg_map;
	UINT8  xindex;
	UINT8  xshift;
	UINT8  xstart;
	UINT8  xend;
	/* GBC specific */
	UINT8  *gbc_map;
	INT16  bgline;
};

struct gb_lcd_struct {
	int	window_lines_drawn;

	/* Things used to render current line */
	int	current_line;		/* Current line */
	int	sprCount;		/* Number of sprites on current line */
	int	sprite[10];		/* References to sprites to draw on current line */
	int	previous_line;		/* Previous line we've drawn in */
	int	start_x;		/* Pixel to start drawing from (inclusive) */
	int	end_x;			/* Pixel to end drawing (exclusive) */
	struct layer_struct	layer[2];
	mame_timer	*lcd_timer;
} gb_lcd;

void (*update_scanline)(void);
static void gb_lcd_timer_proc( int dummy );
static void gb_lcd_switch_on( void );

/*
  Select which sprites should be drawn for the current scanline and return the
  number of sprites selected.
 */
int gb_select_sprites( void ) {
	int	i, yindex, line, height;
	UINT8	*oam = gb_oam + 39 * 4;

	gb_lcd.sprCount = 0;

	/* If video hardware is enabled and sprites are enabled */
	if ( ( LCDCONT & 0x80 ) && ( LCDCONT & 0x02 ) ) {
		/* Check for stretched sprites */
		if ( LCDCONT & 0x04 ) {
			height = 16;
		} else {
			height = 8;
		}

		yindex = gb_lcd.current_line;
		line = gb_lcd.current_line + 16;

		for( i = 39; i >= 0; i-- ) {
			if ( line >= oam[0] && line < ( oam[0] + height ) && oam[1] && oam[1] < 168 ) {
				/* We limit the sprite count to max 10 here;
				   proper games should not exceed this... */
				if ( gb_lcd.sprCount < 10 ) {
					gb_lcd.sprite[gb_lcd.sprCount] = i;
					gb_lcd.sprCount++;
				}
			}
		}
	}
	return gb_lcd.sprCount;
}

INLINE void gb_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam, *vram;
	int i, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	yindex = gb_lcd.current_line;
	line = gb_lcd.current_line + 16;

	oam = gb_oam + 39 * 4;
	vram = gb_vram;
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, *spal;
			int xindex, adr;

			spal = (oam[3] & 0x10) ? gb_spal1 : gb_spal0;
			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				adr = (oam[2] & tilemask) * 16 + (height - 1 - line + oam[0]) * 2;
			}
			else
			{
				adr = (oam[2] & tilemask) * 16 + (line - oam[0]) * 2;
			}
			data = (vram[adr + 1] << 8) | vram[adr];

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

void gb_update_scanline (void) {
	mame_bitmap *bitmap = tmpbitmap;

	profiler_mark(PROFILER_VIDEO);

	/* Make sure we're in mode 3 */
	if ( ( LCDSTAT & 0x03 ) == 0x03 ) {
		/* Calculate number of pixels to render based on time still left on the timer */
		UINT32 cycles_to_go = MAME_TIME_TO_CYCLES( 0, mame_timer_timeleft( gb_lcd.lcd_timer ) );
		int l = 0;

		if ( gb_lcd.start_x == 0 ) {
			/* Window is enabled if the hardware says so AND the current scanline is
			 * within the window AND the window X coordinate is <=166 */
			gb_lcd.layer[1].enabled = ( ( LCDCONT & 0x20 ) && ( gb_lcd.current_line >= WNDPOSY ) && ( WNDPOSX <= 166 ) ) ? 1 : 0;

			/* BG is enabled if the hardware says so AND (window_off OR (window_on
 			* AND window's X position is >=7 ) ) */
			gb_lcd.layer[0].enabled = ( ( LCDCONT & 0x01 ) && ( ( ! gb_lcd.layer[1].enabled ) || ( gb_lcd.layer[1].enabled && ( WNDPOSX >= 7 ) ) ) ) ? 1 : 0;

			if ( gb_lcd.layer[0].enabled ) {
				int bgline = ( SCROLLY + gb_lcd.current_line ) & 0xFF;

				gb_lcd.layer[0].bg_map = gb_bgdtab;
				gb_lcd.layer[0].bg_map += ( bgline << 2 ) & 0x3E0;
				gb_lcd.layer[0].bg_tiles = gb_chrgen + ( ( bgline & 7 ) << 1 );
				gb_lcd.layer[0].xindex = SCROLLX >> 3;
				gb_lcd.layer[0].xshift = SCROLLX & 7;
				gb_lcd.layer[0].xstart = 0;
				gb_lcd.layer[0].xend = 160;
			}

			if ( gb_lcd.layer[1].enabled ) {
				int bgline, xpos;

				bgline = gb_lcd.window_lines_drawn;
				xpos = WNDPOSX - 7;             /* Window is offset by 7 pixels */
				if ( xpos < 0 )
					xpos = 0;

				gb_lcd.layer[1].bg_map = gb_wndtab;
				gb_lcd.layer[1].bg_map += ( bgline << 2 ) & 0x3E0;
				gb_lcd.layer[1].bg_tiles = gb_chrgen + ( ( bgline & 7 ) << 1);
				gb_lcd.layer[1].xindex = 0;
				gb_lcd.layer[1].xshift = 0;
				gb_lcd.layer[1].xstart = xpos;
				gb_lcd.layer[1].xend = 160;
				gb_lcd.layer[0].xend = xpos;
			}
		}

		if ( cycles_to_go < 160 ) {
			gb_lcd.end_x = 160 - cycles_to_go;
			/* Draw empty pixels when the background is disabled */
			if ( ! ( LCDCONT & 0x01 ) ) {
				rectangle r;
				r.min_y = r.max_y = gb_lcd.current_line;
				r.min_x = gb_lcd.start_x;
				r.max_x = gb_lcd.end_x - 1;
				fillbitmap( bitmap, Machine->pens[0], &r );
			}
			while ( l < 2 ) {
				UINT8	xindex, *map, *tiles;
				UINT16	data;
				int	i, tile_index;

				if ( ! gb_lcd.layer[l].enabled ) {
					l++;
					continue;
				}
				map = gb_lcd.layer[l].bg_map;
				tiles = gb_lcd.layer[l].bg_tiles;
				xindex = gb_lcd.start_x;
				if ( xindex < gb_lcd.layer[l].xstart )
					xindex = gb_lcd.layer[l].xstart;
				i = gb_lcd.end_x;
				if ( i > gb_lcd.layer[l].xend )
					i = gb_lcd.layer[l].xend;
				i = i - xindex;

				tile_index = ( map[ gb_lcd.layer[l].xindex ] ^ gb_tile_no_mod ) * 16;
				data = tiles[ tile_index ] | ( tiles[ tile_index+1 ] << 8 );
				data <<= gb_lcd.layer[l].xshift;

				while ( i > 0 ) {
					while ( ( gb_lcd.layer[l].xshift < 8 ) && i ) {
						register int colour = ( ( data & 0x8000 ) ? 2 : 0 ) | ( ( data & 0x0080 ) ? 1 : 0 );
						plot_pixel( bitmap, xindex, gb_lcd.current_line, Machine->pens[ gb_bpal[ colour ] ] );
						bg_zbuf[ xindex ] = colour;
						xindex++;
						data <<= 1;
						gb_lcd.layer[l].xshift++;
						i--;
					}
					if ( gb_lcd.layer[l].xshift == 8 ) {
						gb_lcd.layer[l].xindex = ( gb_lcd.layer[l].xindex + 1 ) & 31;
						gb_lcd.layer[l].xshift = 0;
						tile_index = ( map[ gb_lcd.layer[l].xindex ] ^ gb_tile_no_mod ) * 16;
						data = tiles[ tile_index ] | ( tiles[ tile_index+1 ] << 8 );
					}
				}
				l++;
			}
			if ( gb_lcd.end_x == 160 && LCDCONT & 0x02 ) {
				gb_update_sprites();
			}
			gb_lcd.start_x = gb_lcd.end_x;
		}
	} else {
		if ( ! ( LCDCONT & 0x80 ) ) {
			/* Draw an empty line when LCD is disabled */
			if ( gb_lcd.previous_line != gb_lcd.current_line ) {
				if ( gb_lcd.current_line < 144 ) {
					rectangle r = Machine->screen[0].visarea;
					r.min_y = r.max_y = gb_lcd.current_line;
					fillbitmap( bitmap, Machine->pens[0], &r );
				}
				gb_lcd.previous_line = gb_lcd.current_line;
			}
		}
	}

	profiler_mark(PROFILER_END);
}

/* --- Super Gameboy Specific --- */

INLINE void sgb_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam, *vram, pal;
	INT16 i, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	/* Offset to center of screen */
	yindex = gb_lcd.current_line + SGB_YOFFSET;
	line = gb_lcd.current_line + 16;

	oam = gb_oam + 39 * 4;
	vram = gb_vram;
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, *spal;
			INT16 xindex;
			int adr;

			spal = (oam[3] & 0x10) ? gb_spal1 : gb_spal0;
			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				adr = (oam[2] & tilemask) * 16 + (height -1 - line + oam[0]) * 2;
			}
			else
			{
				adr = (oam[2] & tilemask) * 16 + (line - oam[0]) * 2;
			}
			data = (vram[adr + 1] << 8) | vram[adr];

			/* Find the palette to use */
			pal = sgb_pal_map[(xindex >> 3)][((yindex - SGB_YOFFSET) >> 3)] << 2;

			/* Offset to center of screen */
			xindex += SGB_XOFFSET;

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour && !bg_zbuf[xindex - SGB_XOFFSET])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour && !bg_zbuf[xindex - SGB_XOFFSET])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

void sgb_refresh_border(void) {
	UINT16 data, data2;
	UINT16 yidx, xidx, xindex;
	UINT8 *map, *tiles, *tiles2;
	UINT8 pal, i;
	mame_bitmap *bitmap = tmpbitmap;

	map = sgb_tile_map - 64;

	for( yidx = 0; yidx < 224; yidx++ ) {
		xindex = 0;
		map += (yidx % 8) ? 0 : 64;
		for( xidx = 0; xidx < 64; xidx+=2 ) {
			if( map[xidx+1] & 0x80 ) /* Vertical flip */
				tiles = sgb_tile_data + ( ( 7 - ( yidx % 8 ) ) << 1 );
			else /* No vertical flip */
				tiles = sgb_tile_data + ( ( yidx % 8 ) << 1 );
			tiles2 = tiles + 16;

			pal = (map[xidx+1] & 0x1C) >> 2;
			if( pal == 0 )
				pal = 1;
			pal <<= 4;

			if( sgb_hack ) { /* A few games do weird stuff */
				UINT8 tileno = map[xidx];
				if( tileno >= 128 ) tileno = ((64 + tileno) % 128) + 128;
				else tileno = (64 + tileno) % 128;
				data = tiles[ tileno * 32 ] | ( tiles[ ( tileno * 32 ) + 1 ] << 8 );
				data2 = tiles2[ tileno * 32 ] | ( tiles2[ ( tileno * 32 ) + 1 ] << 8 );
			} else {
				data = tiles[ map[xidx] * 32 ] | ( tiles[ (map[xidx] * 32 ) + 1 ] << 8 );
				data2 = tiles2[ map[xidx] * 32 ] | ( tiles2[ (map[xidx] * 32 ) + 1 ] << 8 );
			}

			for( i = 0; i < 8; i++ ) {
				register UINT8 colour;
				if( (map[xidx+1] & 0x40) ) { /* Horizontal flip */
					colour = ((data  & 0x0001) ? 1 : 0) | ((data  & 0x0100) ? 2 : 0) |
						 ((data2 & 0x0001) ? 4 : 0) | ((data2 & 0x0100) ? 8 : 0);
					data >>= 1;
					data2 >>= 1;
				} else { /* No horizontal flip */
					colour = ((data  & 0x0080) ? 1 : 0) | ((data  & 0x8000) ? 2 : 0) |
						 ((data2 & 0x0080) ? 4 : 0) | ((data2 & 0x8000) ? 8 : 0);
					data <<= 1;
					data2 <<= 1;
				}
				/* A slight hack below so we don't draw over the GB screen.
				 * Drawing there is allowed, but due to the way we draw the
				 * scanline, it can obscure the screen even when it shouldn't.
				 */
				if( !((yidx >= SGB_YOFFSET && yidx < SGB_YOFFSET + 144) &&
					(xindex >= SGB_XOFFSET && xindex < SGB_XOFFSET + 160)) ) {
					plot_pixel(bitmap, xindex, yidx, Machine->remapped_colortable[pal + colour]);
				}
				xindex++;
			}
		}
	}
}

void sgb_update_scanline (void) {
	mame_bitmap *bitmap = tmpbitmap;

	profiler_mark(PROFILER_VIDEO);

	if ( ( LCDSTAT & 0x03 ) == 0x03 ) {
		/* Calcuate number of pixels to render based on time still left on the timer */
		UINT32 cycles_to_go = MAME_TIME_TO_CYCLES( 0, mame_timer_timeleft( gb_lcd.lcd_timer ) );
		int l = 0;

		if ( gb_lcd.start_x == 0 ) {

			/* Window is enabled if the hardware says so AND the current scanline is
			 * within the window AND the window X coordinate is <=166 */
			gb_lcd.layer[1].enabled = ((LCDCONT & 0x20) && gb_lcd.current_line >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

			/* BG is enabled if the hardware says so AND (window_off OR (window_on
			 * AND window's X position is >=7 ) ) */
			gb_lcd.layer[0].enabled = ((LCDCONT & 0x01) && ((!gb_lcd.layer[1].enabled) || (gb_lcd.layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

			if ( gb_lcd.layer[0].enabled ) {
				int bgline = ( SCROLLY + gb_lcd.current_line ) & 0xFF;

				gb_lcd.layer[0].bg_map = gb_bgdtab;
				gb_lcd.layer[0].bg_map += (bgline << 2) & 0x3E0;
				gb_lcd.layer[0].bg_tiles = gb_chrgen + ( (bgline & 7) << 1);
				gb_lcd.layer[0].xindex = SCROLLX >> 3;
				gb_lcd.layer[0].xshift = SCROLLX & 7;
				gb_lcd.layer[0].xstart = 0;
				gb_lcd.layer[0].xend = 160;
			}

			if ( gb_lcd.layer[1].enabled ) {
				int bgline, xpos;

				bgline = (gb_lcd.current_line - WNDPOSY) & 0xFF;
				/* Window X position is offset by 7 so we'll need to adjust */
				xpos = WNDPOSX - 7;
				if (xpos < 0)
					xpos = 0;

				gb_lcd.layer[1].bg_map = gb_wndtab;
				gb_lcd.layer[1].bg_map += (bgline << 2) & 0x3E0;
				gb_lcd.layer[1].bg_tiles = gb_chrgen + ( (bgline & 7) << 1);
				gb_lcd.layer[1].xindex = 0;
				gb_lcd.layer[1].xshift = 0;
				gb_lcd.layer[1].xstart = xpos;
				gb_lcd.layer[1].xend = 160;
				gb_lcd.layer[0].xend = xpos;
			}
		}

		if ( cycles_to_go == 0 ) {

			/* Does this belong here? or should it be moved to the else block */
			/* Handle SGB mask */
			switch( sgb_window_mask ) {
			case 1: /* Freeze screen */
				return;
			case 2: /* Blank screen (black) */
				{
					rectangle r = Machine->screen[0].visarea;
					r.min_x = SGB_XOFFSET;
					r.max_x -= SGB_XOFFSET;
					r.min_y = SGB_YOFFSET;
					r.max_y -= SGB_YOFFSET;
					fillbitmap( bitmap, Machine->pens[0], &r );
				} return;
			case 3: /* Blank screen (white - or should it be color 0?) */
				{
					rectangle r = Machine->screen[0].visarea;
					r.min_x = SGB_XOFFSET;
					r.max_x -= SGB_XOFFSET;
					r.min_y = SGB_YOFFSET;
					r.max_y -= SGB_YOFFSET;
					fillbitmap( bitmap, Machine->pens[32767], &r );
				} return;
			}

			/* Draw the "border" if we're on the first line */
			if ( gb_lcd.current_line == 0 ) {
				sgb_refresh_border();
			}
		}
		if ( cycles_to_go < 160 ) {
			gb_lcd.end_x = 160 - cycles_to_go;

			/* if background or screen disabled clear line */
			if ( ! ( LCDCONT & 0x01 ) ) {
				rectangle r = Machine->screen[0].visarea;
				r.min_x = SGB_XOFFSET;
				r.max_x -= SGB_XOFFSET;
				r.min_y = r.max_y = gb_lcd.current_line + SGB_YOFFSET;
				fillbitmap( bitmap, Machine->pens[0], &r );
			}
			while( l < 2 ) {
				UINT8	xindex, sgb_pal, *map, *tiles;
				UINT16	data;
				int	i, tile_index;

				if ( ! gb_lcd.layer[l].enabled ) {
					l++;
					continue;
				}
				map = gb_lcd.layer[l].bg_map;
				tiles = gb_lcd.layer[l].bg_tiles;
				xindex = gb_lcd.start_x;
				if ( xindex < gb_lcd.layer[l].xstart )
					xindex = gb_lcd.layer[l].xstart;
				i = gb_lcd.end_x;
				if ( i > gb_lcd.layer[l].xend )
					i = gb_lcd.layer[l].xend;
				i = i - xindex;

				tile_index = (map[gb_lcd.layer[l].xindex] ^ gb_tile_no_mod) * 16;
				data = tiles[tile_index] | ( tiles[tile_index + 1] << 8 );
				data <<= gb_lcd.layer[l].xshift;

				/* Figure out which palette we're using */
				sgb_pal = sgb_pal_map[ ( gb_lcd.end_x - i ) >> 3 ][ gb_lcd.current_line >> 3 ] << 2;

				while( i > 0 ) {
					while( ( gb_lcd.layer[l].xshift < 8 ) && i ) {
						register int colour = ( ( data & 0x8000 ) ? 2 : 0 ) | ( ( data & 0x0080 ) ? 1 : 0 );
						plot_pixel( bitmap, xindex + SGB_XOFFSET, gb_lcd.current_line + SGB_YOFFSET, Machine->remapped_colortable[ sgb_pal + gb_bpal[colour]] );
						bg_zbuf[xindex] = colour;
						xindex++;
						data <<= 1;
						gb_lcd.layer[l].xshift++;
						i--;
					}
					if ( gb_lcd.layer[l].xshift == 8 ) {
						gb_lcd.layer[l].xindex = ( gb_lcd.layer[l].xindex + 1 ) & 31;
						gb_lcd.layer[l].xshift = 0;
						tile_index = ( map[ gb_lcd.layer[l].xindex ] ^ gb_tile_no_mod ) * 16;
						data = tiles[ tile_index ] | ( tiles[ tile_index + 1 ] << 8 );
						sgb_pal = sgb_pal_map[ ( gb_lcd.end_x - i ) >> 3 ][ gb_lcd.current_line >> 3 ] << 2;
					}
				}
				l++;
			}
			if ( ( gb_lcd.end_x == 160 ) && ( LCDCONT & 0x02 ) ) {
				sgb_update_sprites();
			}
			gb_lcd.start_x = gb_lcd.end_x;
		}
	} else {
		if ( ! ( LCDCONT * 0x80 ) ) {
			/* if screen disabled clear line */
			if ( gb_lcd.previous_line != gb_lcd.current_line ) {
				/* Also refresh border here??? */
				if ( gb_lcd.current_line < 144 ) {
					rectangle r = Machine->screen[0].visarea;
					r.min_x = SGB_XOFFSET;
					r.max_x -= SGB_XOFFSET;
					r.min_y = r.max_y = gb_lcd.current_line + SGB_YOFFSET;
					fillbitmap(bitmap, Machine->pens[0], &r);
				}
				gb_lcd.previous_line = gb_lcd.current_line;
			}
		}
	}

	profiler_mark(PROFILER_END);
}

/* --- Gameboy Color Specific --- */

INLINE void cgb_update_sprites (void) {
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam;
	int i, xindex, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	yindex = gb_lcd.current_line;
	line = gb_lcd.current_line + 16;

	oam = gb_oam + 39 * 4;
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, pal;

			/* Handle mono mode for GB games */
			if( gbc_mode == GBC_MODE_MONO )
				pal = (oam[3] & 0x10) ? 8 : 4;
			else
				pal = GBC_PAL_OBJ_OFFSET + ((oam[3] & 0x7) * 4);

			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				data = *((UINT16 *) &GBC_VRAMMap[(oam[3] & 0x8)>>3][(oam[2] & tilemask) * 16 + (height - 1 - line + oam[0]) * 2]);
			}
			else
			{
				data = *((UINT16 *) &GBC_VRAMMap[(oam[3] & 0x8)>>3][(oam[2] & tilemask) * 16 + (line - oam[0]) * 2]);
			}
#ifndef LSB_FIRST
			data = (data << 8) | (data >> 8);
#endif

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if((bg_zbuf[xindex] & 0x80) && (bg_zbuf[xindex] & 0x7f) && (LCDCONT & 0x1))
						colour = 0;
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if((bg_zbuf[xindex] & 0x80) && (bg_zbuf[xindex] & 0x7f) && (LCDCONT & 0x1))
						colour = 0;
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

void cgb_update_scanline (void) {
	mame_bitmap *bitmap = tmpbitmap;

	profiler_mark(PROFILER_VIDEO);

	if ( ( LCDSTAT & 0x03 ) == 0x03 ) {
		/* Calcuate number of pixels to render based on time still left on the timer */
		UINT32 cycles_to_go = MAME_TIME_TO_CYCLES( 0, mame_timer_timeleft( gb_lcd.lcd_timer ) );
		int l = 0;

		if ( gb_lcd.start_x == 0 ) {

			/* Window is enabled if the hardware says so AND the current scanline is
			 * within the window AND the window X coordinate is <=166 */
			gb_lcd.layer[1].enabled = ( ( LCDCONT & 0x20 ) && ( gb_lcd.current_line >= WNDPOSY ) && ( WNDPOSX <= 166 ) ) ? 1 : 0;

			/* BG is enabled if the hardware says so AND (window_off OR (window_on
			 * AND window's X position is >=7 ) ) */
			gb_lcd.layer[0].enabled = ( ( LCDCONT & 0x01 ) && ( ( ! gb_lcd.layer[1].enabled ) || ( gb_lcd.layer[1].enabled && ( WNDPOSX >= 7 ) ) ) ) ? 1 : 0;

			if ( gb_lcd.layer[0].enabled ) {
				int bgline = ( SCROLLY + gb_lcd.current_line ) & 0xFF;

				gb_lcd.layer[0].bgline = bgline;
				gb_lcd.layer[0].bg_map = gb_bgdtab;
				gb_lcd.layer[0].bg_map += ( bgline << 2 ) & 0x3E0;
				gb_lcd.layer[0].gbc_map = gbc_bgdtab;
				gb_lcd.layer[0].gbc_map += ( bgline << 2 ) & 0x3E0;
				gb_lcd.layer[0].xindex = SCROLLX >> 3;
				gb_lcd.layer[0].xshift = SCROLLX & 7;
				gb_lcd.layer[0].xstart = 0;
				gb_lcd.layer[0].xend = 160;
			}

			if ( gb_lcd.layer[1].enabled ) {
				int bgline, xpos;

				bgline = gb_lcd.window_lines_drawn;
				/* Window X position is offset by 7 so we'll need to adust */
				xpos = WNDPOSX - 7;
				if (xpos < 0)
					xpos = 0;

				gb_lcd.layer[1].bgline = bgline;
				gb_lcd.layer[1].bg_map = gb_wndtab;
				gb_lcd.layer[1].bg_map += ( bgline << 2 ) & 0x3E0;
				gb_lcd.layer[1].gbc_map = gbc_wndtab;
				gb_lcd.layer[1].gbc_map += ( bgline << 2 ) & 0x3E0;
				gb_lcd.layer[1].xindex = 0;
				gb_lcd.layer[1].xshift = 0;
				gb_lcd.layer[1].xstart = xpos;
				gb_lcd.layer[1].xend = 160;
				gb_lcd.layer[0].xend = xpos;
			}
		}

		if ( cycles_to_go < 160 ) {
			gb_lcd.end_x = 160 - cycles_to_go;
			/* Draw empty line when the background is disabled */
			if ( ! ( LCDCONT & 0x01 ) ) {
				rectangle r = Machine->screen[0].visarea;
				r.min_y = r.max_y = gb_lcd.current_line;
				r.min_x = gb_lcd.start_x;
				r.max_x = gb_lcd.end_x - 1;
				fillbitmap( bitmap, Machine->pens[0], &r );
			}
			while ( l < 2 ) {
				UINT8	xindex, *map, *tiles, *gbcmap;
				UINT16	data;
				int	i, tile_index;

				if ( ! gb_lcd.layer[l].enabled ) {
					l++;
					continue;
				}
				map = gb_lcd.layer[l].bg_map;
				gbcmap = gb_lcd.layer[l].gbc_map;
				tiles = ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x08 ) ? gbc_chrgen : gb_chrgen;

				/* Check for vertical flip */
				if ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x40 ) {
					tiles += ( ( 7 - ( gb_lcd.layer[l].bgline & 0x07 ) ) << 1 );
				} else {
					tiles += ( ( gb_lcd.layer[l].bgline & 0x07 ) << 1 );
				}
				xindex = gb_lcd.start_x;
				if ( xindex < gb_lcd.layer[l].xstart )
					xindex = gb_lcd.layer[l].xstart;
				i = gb_lcd.end_x;
				if ( i > gb_lcd.layer[l].xend )
					i = gb_lcd.layer[l].xend;
				i = i - xindex;

				tile_index = ( map[ gb_lcd.layer[l].xindex ] ^ gb_tile_no_mod ) * 16;
				data = tiles[ tile_index ] | ( tiles[ tile_index + 1 ] << 8 );
				/* Check for horinzontal flip */
				if ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x20 ) {
					data >>= gb_lcd.layer[l].xshift;
				} else {
					data <<= gb_lcd.layer[l].xshift;
				}

				while ( i > 0 ) {
					while ( ( gb_lcd.layer[l].xshift < 8 ) && i ) {
						int colour;
						/* Check for horinzontal flip */
						if ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x20 ) {
							colour = ( ( data & 0x0100 ) ? 2 : 0 ) | ( ( data & 0x0001 ) ? 1 : 0 );
							data >>= 1;
						} else {
							colour = ( ( data & 0x8000 ) ? 2 : 0 ) | ( ( data & 0x0080 ) ? 1 : 0 );
							data <<= 1;
						}
						plot_pixel( bitmap, xindex, gb_lcd.current_line, Machine->remapped_colortable[ ( ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x07 ) * 4 ) + colour ] );
						bg_zbuf[ xindex ] = colour + ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x80 );
						xindex++;
						gb_lcd.layer[l].xshift++;
						i--;
					}
					if ( gb_lcd.layer[l].xshift == 8 ) {
						gb_lcd.layer[l].xindex = ( gb_lcd.layer[l].xindex + 1 ) & 31;
						gb_lcd.layer[l].xshift = 0;
						tiles = ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x08 ) ? gbc_chrgen : gb_chrgen;

						/* Check for vertical flip */
						if ( gbcmap[ gb_lcd.layer[l].xindex ] & 0x40 ) {
							tiles += ( ( 7 - ( gb_lcd.layer[l].bgline & 0x07 ) ) << 1 );
						} else {
							tiles += ( ( gb_lcd.layer[l].bgline & 0x07 ) << 1 );
						}
						tile_index = ( map[ gb_lcd.layer[l].xindex ] ^ gb_tile_no_mod ) * 16;
						data = tiles[ tile_index ] | ( tiles[ tile_index + 1 ] << 8 );
					}
				}
				l++;
			}
			if ( gb_lcd.end_x == 160 && ( LCDCONT & 0x02 ) ) {
				cgb_update_sprites();
			}
			gb_lcd.start_x = gb_lcd.end_x;
		}
	} else {
		if ( ! ( LCDCONT & 0x80 ) ) {
			/* Draw an empty line when LCD is disabled */
			if ( gb_lcd.previous_line != gb_lcd.current_line ) {
				if ( gb_lcd.current_line < 144 ) {
					rectangle r = Machine->screen[0].visarea;
					r.min_y = r.max_y = gb_lcd.current_line;
					fillbitmap( bitmap, Machine->pens[0], &r );
				}
				gb_lcd.previous_line = gb_lcd.current_line;
			}
		}
	}

	profiler_mark(PROFILER_END);
}

void gb_video_init( void ) {
	int	i;

	gb_chrgen = gb_vram;
	gb_bgdtab = gb_vram + 0x1C00;
	gb_wndtab = gb_vram + 0x1C00;

	gb_vid_regs[0x06] = 0xFF;
	for( i = 0x0c; i < _NR_GB_VID_REGS; i++ ) {
		gb_vid_regs[i] = 0xFF;
	}

	LCDSTAT = 0x80;
	LCDCONT = 0x00;
	gb_lcd.current_line = CURLINE = CMPLINE = 0x00;
	SCROLLX = SCROLLY = 0x00;
	SPR0PAL = SPR1PAL = 0xFF;
	WNDPOSX = WNDPOSY = 0x00;

	/* Initialize palette arrays */
	for( i = 0; i < 4; i++ ) {
		gb_bpal[i] = gb_spal0[i] = gb_spal1[i] = i;
	}

	/* initialize OAM extra area */
	for( i = 0xa0; i < 0x100; i++ ) {
		gb_oam[i] = 0x00;
	}

	/* set the scanline update function */
	update_scanline = gb_update_scanline;

	gb_lcd.lcd_timer = mame_timer_alloc( gb_lcd_timer_proc );
	mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(456,0), 0, time_never );
}

void sgb_video_init( void ) {
	gb_video_init();

	/* Override the scanline refresh function */
	update_scanline = sgb_update_scanline;
}

/* The CGB seems to have some data in the FEA0-FEFF area upon booting.
   The contents of this area are almost the same on each boot, but always
   a couple of bits are different.
   The data could be some kind of fingerprint for each CGB, I've just taken
   this data from my CGB on a boot once.
*/

const UINT8 cgb_oam_extra[0x60] = {
	0x74, 0xFF, 0x09, 0x00, 0x9D, 0x61, 0xA8, 0x28, 0x36, 0x1E, 0x58, 0xAA, 0x75, 0x74, 0xA1, 0x42,
	0x05, 0x96, 0x40, 0x09, 0x41, 0x02, 0x60, 0x00, 0x1F, 0x11, 0x22, 0xBC, 0x31, 0x52, 0x22, 0x54,
	0x22, 0xA9, 0xC4, 0x00, 0x1D, 0xAD, 0x80, 0x0C, 0x5D, 0xFA, 0x51, 0x92, 0x93, 0x98, 0xA4, 0x04,
	0x22, 0xA9, 0xC4, 0x00, 0x1D, 0xAD, 0x80, 0x0C, 0x5D, 0xFA, 0x51, 0x92, 0x93, 0x98, 0xA4, 0x04,
	0x22, 0xA9, 0xC4, 0x00, 0x1D, 0xAD, 0x80, 0x0C, 0x5D, 0xFA, 0x51, 0x92, 0x93, 0x98, 0xA4, 0x04,
	0x22, 0xA9, 0xC4, 0x00, 0x1D, 0xAD, 0x80, 0x0C, 0x5D, 0xFA, 0x51, 0x92, 0x93, 0x98, 0xA4, 0x04
};

/*
  For an AGB in CGB mode this data is:
	0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
	0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB,
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
	0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD,
	0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
*/

void gbc_video_init( void ) {
	int i;

	gb_video_init();

	for( i = 0; i < sizeof(cgb_oam_extra); i++ ) {
		gb_oam[ 0xa0 + i] = cgb_oam_extra[i];
	}

	gb_chrgen = GBC_VRAMMap[0];
	gbc_chrgen = GBC_VRAMMap[1];
	gb_bgdtab = gb_wndtab = GBC_VRAMMap[0] + 0x1C00;
	gbc_bgdtab = gbc_wndtab = GBC_VRAMMap[1] + 0x1C00;

	/* Override the scanline update function */
	update_scanline = cgb_update_scanline;

	/* HDMA disabled */
	gbc_hdma_enabled = 0;
}

void gbc_hdma(UINT16 length) {
	UINT16 src, dst;

	src = ((UINT16)HDMA1 << 8) | (HDMA2 & 0xF0);
	dst = ((UINT16)(HDMA3 & 0x1F) << 8) | (HDMA4 & 0xF0);
	dst |= 0x8000;
	while( length > 0 ) {
		program_write_byte_8( dst++, program_read_byte_8( src++ ) );
		length--;
	}
	HDMA1 = src >> 8;
	HDMA2 = src & 0xF0;
	HDMA3 = 0x1f & (dst >> 8);
	HDMA4 = dst & 0xF0;
	HDMA5--;
	if( (HDMA5 & 0x7f) == 0 ) {
		HDMA5 = 0xff;
		gbc_hdma_enabled = 0;
	}
}

void gb_increment_scanline( void ) {
	gb_lcd.current_line = ( gb_lcd.current_line + 1 ) % 154;
	if ( LCDCONT & 0x80 ) {
		CURLINE = gb_lcd.current_line;
		if ( CURLINE == CMPLINE ) {
			LCDSTAT |= 0x04;
			/* Generate lcd interrupt if requested */
			if ( LCDSTAT & 0x40 )
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
		} else {
			LCDSTAT &= 0xFB;
		}
	}
	if ( gb_lcd.current_line == 0 ) {
		gb_lcd.window_lines_drawn = 0;
	}
}

static void gb_lcd_timer_proc( int mode ) {
	if ( LCDCONT & 0x80 ) {
		switch( mode ) {
		case 0:		/* Switch to mode 0 */
			/* update current scanline */
			update_scanline();
			/* Increment the number of window lines drawn if enabled */
			if ( gb_lcd.layer[1].enabled ) {
				gb_lcd.window_lines_drawn++;
			}
			gb_lcd.previous_line = gb_lcd.current_line;
			/* Set Mode 0 lcdstate */
			LCDSTAT &= 0xFC;
			/* Generate lcd interrupt if requested */
			if( LCDSTAT & 0x08 ) {
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
			}
			/* Check for HBLANK DMA */
			if( gbc_hdma_enabled )
				gbc_hdma(0x10);
			if ( CURLINE == 143 ) {
				mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(204,0), 1, time_never );
			} else {
				mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(204,0), 2, time_never );
			}
			break;
		case 2:		/* Switch to mode 2 */
			gb_increment_scanline();
		case 5:		/* on scanline 0, don't increment the current line counter */
			/* Set Mode 2 lcdstate */
			LCDSTAT = (LCDSTAT & 0xFC) | 0x02;
			/* Generate lcd interrupt if requested */
			if (LCDSTAT & 0x20) {
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
			}
			/* Mode 2 last for approximately 80 clock cycles */
			mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(80,0), 3, time_never );
			break;
		case 3:		/* Switch to mode 3 */
			gb_select_sprites();
			/* Set Mode 3 lcdstate */
			LCDSTAT = (LCDSTAT & 0xFC) | 0x03;
			/* Mode 3 lasts for approximately 172+#sprites*10 clock cycles */
			mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(172,0), 0, time_never );
			gb_lcd.start_x = 0;
			break;
		case 1:		/* Switch to or stay in mode 1 */
			gb_increment_scanline();
			if ( CURLINE == 144 ) {
				/* Trigger VBlank interrupt */
				cpunum_set_input_line(0, VBL_INT, HOLD_LINE);
				/* Set VBlank lcdstate */
				LCDSTAT = (LCDSTAT & 0xFC) | 0x01;
				/* Trigger LCD interrupt if requested */
				if( LCDSTAT & 0x10 ) {
					cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
				}
			}
			if ( gb_lcd.current_line == 153 ) {
				mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(24,0), 4, time_never );
			} else {
				mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(456,0), 1, time_never );
			}
			break;
		case 4:		/* we stay in VBlank but current line counter should already be incremented */
			gb_increment_scanline();
			mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(456-24,0), 5, time_never );
			break;
		}
	} else {
		gb_increment_scanline();
		if ( gb_lcd.current_line < 144 ) {
			update_scanline();
		}
		mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(456,0), 0, time_never );
	}
}

static void gb_lcd_switch_on( void ) {
	gb_lcd.current_line = 0;
	gb_lcd.previous_line = 153;
	gb_lcd.window_lines_drawn = 0;
	/* Check for LY=LYC coincidence */
	if ( CURLINE == CMPLINE ) {
		LCDSTAT |= 0x04;
		/* Generate lcd interrupt if requested */
		if ( LCDSTAT & 0x40 ) {
			cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
		}
	}
	mame_timer_adjust( gb_lcd.lcd_timer, MAME_TIME_IN_CYCLES(80,0), 3, time_never );
}

READ8_HANDLER( gb_video_r ) {
	return gb_vid_regs[offset];
}

/* Ignore write when LCD is on and STAT is 02 or 03 */
int gb_video_oam_locked( void ) {
	if ( ( LCDCONT & 0x80 ) && ( LCDSTAT & 0x02 ) ) {
		return 1;
	}
	return 0;
}

/* Ignore write when LCD is on and STAT is not 03 */
int gb_video_vram_locked( void ) {
	if ( ( LCDCONT & 0x80 ) && ( ( LCDSTAT & 0x03 ) == 0x03 ) ) {
		return 1;
	}
	return 0;
}

WRITE8_HANDLER ( gb_video_w ) {
	switch (offset) {
	case 0x00:						/* LCDC - LCD Control */
		gb_chrgen = gb_vram + ((data & 0x10) ? 0x0000 : 0x0800);
		gb_tile_no_mod = (data & 0x10) ? 0x00 : 0x80;
		gb_bgdtab = gb_vram + ((data & 0x08) ? 0x1C00 : 0x1800 );
		gb_wndtab = gb_vram + ((data & 0x40) ? 0x1C00 : 0x1800 );
		/* if LCD controller is switched off, set STAT and LY to 00 */
		if ( ! ( data & 0x80 ) ) {
			LCDSTAT &= ~0x03;
			CURLINE = 0;
		}
		/* If LCD is being switched on */
		if ( !( LCDCONT & 0x80 ) && ( data & 0x80 ) ) {
			gb_lcd_switch_on();
		}
		break;
	case 0x01:						/* STAT - LCD Status */
		data = 0x80 | (data & 0x78) | (LCDSTAT & 0x07);
		/*
		   Check for the STAT bug:
		   Writing to STAT when the STAT mode is 1 or 0
                   causes a STAT interrupt to be triggered.
		 */
		if ( LCDCONT & 0x80 ) {
			switch ( LCDSTAT & 0x03 ) {
			case 0x00:
			case 0x01:
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
				break;
			}
		}
		break;
	case 0x04:						/* LY - LCD Y-coordinate */
		return;
	case 0x06:						/* DMA - DMA Transfer and Start Address */
		{
			UINT8 *P = gb_oam;
			offset = (UINT16) data << 8;
			for (data = 0; data < 0xA0; data++)
				*P++ = program_read_byte_8 (offset++);
		}
		return;
	case 0x07:						/* BGP - Background Palette */
		update_scanline();
		gb_bpal[0] = data & 0x3;
		gb_bpal[1] = (data & 0xC) >> 2;
		gb_bpal[2] = (data & 0x30) >> 4;
		gb_bpal[3] = (data & 0xC0) >> 6;
		break;
	case 0x08:						/* OBP0 - Object Palette 0 */
//		update_scanline();
		gb_spal0[0] = data & 0x3;
		gb_spal0[1] = (data & 0xC) >> 2;
		gb_spal0[2] = (data & 0x30) >> 4;
		gb_spal0[3] = (data & 0xC0) >> 6;
		break;
	case 0x09:						/* OBP1 - Object Palette 1 */
//		update_scanline();
		gb_spal1[0] = data & 0x3;
		gb_spal1[1] = (data & 0xC) >> 2;
		gb_spal1[2] = (data & 0x30) >> 4;
		gb_spal1[3] = (data & 0xC0) >> 6;
		break;
	case 0x02:						/* SCY - Scroll Y */
	case 0x03:						/* SCX - Scroll X */
		update_scanline();
	case 0x05:						/* LYC - LCD Y-compare */
	case 0x0A:						/* WY - Window Y position */
	case 0x0B:						/* WX - Window X position */
		break;
	default:						/* Unknown register, no change */
		return;
	}
	gb_vid_regs[ offset ] = data;
}

WRITE8_HANDLER ( gbc_video_w ) {
	static const UINT16 gbc_to_gb_pal[4] = {32767, 21140, 10570, 0};
	static UINT16 BP = 0, OP = 0;

	switch( offset ) {
	case 0x00:      /* LCDC - LCD Control */
		gb_chrgen = GBC_VRAMMap[0] + ((data & 0x10) ? 0x0000 : 0x0800);
		gbc_chrgen = GBC_VRAMMap[1] + ((data & 0x10) ? 0x0000 : 0x0800);
		gb_tile_no_mod = (data & 0x10) ? 0x00 : 0x80;
		gb_bgdtab = GBC_VRAMMap[0] + ((data & 0x08) ? 0x1C00 : 0x1800);
		gbc_bgdtab = GBC_VRAMMap[1] + ((data & 0x08) ? 0x1C00 : 0x1800);
		gb_wndtab = GBC_VRAMMap[0] + ((data & 0x40) ? 0x1C00 : 0x1800);
		gbc_wndtab = GBC_VRAMMap[1] + ((data & 0x40) ? 0x1C00 : 0x1800);
		/* if LCD controller is switched off, set STAT to 00 */
		if ( ! ( data & 0x80 ) ) {
			LCDSTAT &= ~0x03;
			CURLINE = 0;
		}
                /* If LCD is being switched on */
                if ( !( LCDCONT & 0x80 ) && ( data & 0x80 ) ) {
			gb_lcd_switch_on();
                }
		break;
	case 0x01:      /* STAT - LCD Status */
		data = 0x80 | (data & 0x78) | (LCDSTAT & 0x07);
		break;
	case 0x07:      /* BGP - GB background palette */
		/* Some GBC games are lazy and still call this */
		if( gbc_mode == GBC_MODE_MONO ) {
			update_scanline();
			Machine->remapped_colortable[0] = gbc_to_gb_pal[(data & 0x03)];
			Machine->remapped_colortable[1] = gbc_to_gb_pal[(data & 0x0C) >> 2];
			Machine->remapped_colortable[2] = gbc_to_gb_pal[(data & 0x30) >> 4];
			Machine->remapped_colortable[3] = gbc_to_gb_pal[(data & 0xC0) >> 6];
		}
		break;
	case 0x08:      /* OBP0 - GB Object 0 palette */
		if( gbc_mode == GBC_MODE_MONO ) /* Some GBC games are lazy and still call this */
		{
			Machine->remapped_colortable[4] = gbc_to_gb_pal[(data & 0x03)];
			Machine->remapped_colortable[5] = gbc_to_gb_pal[(data & 0x0C) >> 2];
			Machine->remapped_colortable[6] = gbc_to_gb_pal[(data & 0x30) >> 4];
			Machine->remapped_colortable[7] = gbc_to_gb_pal[(data & 0xC0) >> 6];
		}
		break;
	case 0x09:      /* OBP1 - GB Object 1 palette */
		if( gbc_mode == GBC_MODE_MONO ) /* Some GBC games are lazy and still call this */
		{
			Machine->remapped_colortable[8] = gbc_to_gb_pal[(data & 0x03)];
			Machine->remapped_colortable[9] = gbc_to_gb_pal[(data & 0x0C) >> 2];
			Machine->remapped_colortable[10] = gbc_to_gb_pal[(data & 0x30) >> 4];
			Machine->remapped_colortable[11] = gbc_to_gb_pal[(data & 0xC0) >> 6];
		}
		break;
	case 0x11:      /* HDMA1 - HBL General DMA - Source High */
		break;
	case 0x12:      /* HDMA2 - HBL General DMA - Source Low */
		data &= 0xF0;
		break;
	case 0x13:      /* HDMA3 - HBL General DMA - Destination High */
		data &= 0x1F;
		break;
	case 0x14:      /* HDMA4 - HBL General DMA - Destination Low */
		data &= 0xF0;
		break;
	case 0x15:      /* HDMA5 - HBL General DMA - Mode, Length */
		if( !(data & 0x80) )
		{
			if( gbc_hdma_enabled )
			{
				gbc_hdma_enabled = 0;
				data = HDMA5 & 0x80;
			}
			else
			{
				/* General DMA */
				gbc_hdma( ((data & 0x7F) + 1) * 0x10 );
				lcd_time -= ((KEY1 & 0x80)?110:220) + (((data & 0x7F) + 1) * 7.68);
				data = 0xff;
			}
		}
		else
		{
			/* H-Blank DMA */
			gbc_hdma_enabled = 1;
			data &= 0x7f;
		}
		break;
	case 0x28:      /* BCPS - Background palette specification */
		break;
	case 0x29:      /* BCPD - background palette data */
		if( GBCBCPS & 0x1 )
			Machine->remapped_colortable[(GBCBCPS & 0x3e) >> 1] = ((UINT16)(data & 0x7f) << 8) | BP;
		else
			BP = data;
		if( GBCBCPS & 0x80 )
		{
			GBCBCPS++;
			GBCBCPS &= 0xBF;
		}
		break;
	case 0x2A:      /* OCPS - Object palette specification */
		break;
	case 0x2B:      /* OCPD - Object palette data */
		if( GBCOCPS & 0x1 )
			Machine->remapped_colortable[GBC_PAL_OBJ_OFFSET + ((GBCOCPS & 0x3e) >> 1)] = ((UINT16)(data & 0x7f) << 8) | OP;
		else
			OP = data;
		if( GBCOCPS & 0x80 )
		{
			GBCOCPS++;
			GBCOCPS &= 0xBF;
		}
		break;
	/* Undocumented registers */
	case 0x2C:
		/* bit 0 can be read/written */
		logerror( "Write to undocumented register: %X = %X\n", offset, data );
		data = 0xFE | ( data & 0x01 );
		break;
	case 0x32:
	case 0x33:
	case 0x34:
		/* whole byte can be read/written */
		logerror( "Write to undocumented register: %X = %X\n", offset, data );
		break;
	case 0x35:
		/* bit 4-6 can be read/written */
		logerror( "Write to undocumented register: %X = %X\n", offset, data );
		data = 0x8F | ( data & 0x70 );
		break;
	case 0x36:
	case 0x37:
		logerror( "Write to undocumented register: %X = %X\n", offset, data );
		return;
	default:
		/* we didn't handle the write, so pass it to the GB handler */
		gb_video_w( offset, data );
		return;
	}

	gb_vid_regs[offset] = data;
}

