/*********************************************************************

    drawgfx.h

    Generic graphic functions.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#pragma once

#ifndef __DRAWGFX_H__
#define __DRAWGFX_H__

#include "mamecore.h"

#define MAX_GFX_PLANES		8
#define MAX_GFX_SIZE		32
#define MAX_ABS_GFX_SIZE	1024

#define EXTENDED_XOFFS		{ 0 }
#define EXTENDED_YOFFS		{ 0 }

#define RGN_FRAC(num,den) (0x80000000 | (((num) & 0x0f) << 27) | (((den) & 0x0f) << 23))
#define IS_FRAC(offset) ((offset) & 0x80000000)
#define FRAC_NUM(offset) (((offset) >> 27) & 0x0f)
#define FRAC_DEN(offset) (((offset) >> 23) & 0x0f)
#define FRAC_OFFSET(offset) ((offset) & 0x007fffff)

#define STEP4(START,STEP)  (START),(START)+1*(STEP),(START)+2*(STEP),(START)+3*(STEP)
#define STEP8(START,STEP)  STEP4(START,STEP),STEP4((START)+4*(STEP),STEP)
#define STEP16(START,STEP) STEP8(START,STEP),STEP8((START)+8*(STEP),STEP)


struct _gfx_layout
{
	UINT16 width,height;				/* width and height (in pixels) of chars/sprites */
	UINT32 total;						/* total number of chars/sprites in the rom */
	UINT16 planes;						/* number of bitplanes */
	UINT32 planeoffset[MAX_GFX_PLANES]; /* start of every bitplane (in bits) */
	UINT32 xoffset[MAX_GFX_SIZE];		/* position of the bit corresponding to the pixel */
	UINT32 yoffset[MAX_GFX_SIZE];		/* of the given coordinates */
	UINT32 charincrement;				/* distance between two consecutive characters/sprites (in bits) */
	const UINT32 *extxoffs;				/* extended X offset array for really big layouts */
	const UINT32 *extyoffs;				/* extended Y offset array for really big layouts */
};
typedef struct _gfx_layout gfx_layout;

#define GFX_RAW 0x12345678
/* When planeoffset[0] is set to GFX_RAW, the gfx data is left as-is, with no conversion.
   No buffer is allocated for the decoded data, and gfxdata is set to point to the source
   data; therefore, you must not use ROMREGION_DISPOSE.
   xoffset[0] is an optional displacement (*8) from the beginning of the source data, while
   yoffset[0] is the line modulo (*8) and charincrement the char modulo (*8). They are *8
   for consistency with the usual behaviour, but the bottom 3 bits are not used.
   GFX_PACKED is automatically set if planes is <= 4.

   This special mode can be used to save memory in games that require several different
   handlings of the same ROM data (e.g. metro.c can use both 4bpp and 8bpp tiles, and both
   8x8 and 16x16; cps.c has 8x8, 16x16 and 32x32 tiles all fetched from the same ROMs).
   Note, however, that performance will suffer in rotated games, since the gfx data will
   not be prerotated and will rely on GFX_SWAPXY.
*/

struct _gfx_element
{
	UINT16 width,height;

	UINT32 total_elements;	/* total number of characters/sprites */
	UINT16 color_granularity;	/* number of colors for each color code */
							/* (for example, 4 for 2 bitplanes gfx) */
	UINT32 total_colors;
	pen_t *colortable;	/* map color codes to screen pens */
	UINT32 *pen_usage;	/* an array of total_elements entries. */
						/* It is a table of the pens each character uses */
						/* (bit 0 = pen 0, and so on). This is used by */
						/* drawgfgx() to do optimizations like skipping */
						/* drawing of a totally transparent character */
	UINT8 *gfxdata;		/* pixel data */
	UINT32 line_modulo;	/* amount to add to get to the next line (usually = width) */
	UINT32 char_modulo;	/* = line_modulo * height */
	UINT32 flags;
	gfx_layout layout;	/* references the original layout */
};
/* In mamecore.h: typedef struct _gfx_element gfx_element; */

#define GFX_PACKED				1	/* two 4bpp pixels are packed in one byte of gfxdata */
#define GFX_SWAPXY				2	/* characters are mirrored along the top-left/bottom-right diagonal */
#define GFX_DONT_FREE_GFXDATA	4	/* gfxdata was not malloc()ed, so don't free it on exit */


typedef struct _gfx_decode gfx_decode;
struct _gfx_decode
{
	int memory_region;	/* memory region where the data resides (usually 1) */
						/* -1 marks the end of the array */
	UINT32 start;	/* beginning of data to decode */
	const gfx_layout *gfxlayout;
	UINT16 color_codes_start;	/* offset in the color lookup table where color codes start */
	UINT16 total_color_codes;	/* total number of color codes */
};


typedef struct _alpha_cache alpha_cache;
struct _alpha_cache
{
	int		alphas;
	int		alphad;
};

extern struct _alpha_cache drawgfx_alpha_cache;

enum
{
	TRANSPARENCY_NONE,			/* opaque with remapping */
	TRANSPARENCY_NONE_RAW,		/* opaque with no remapping */
	TRANSPARENCY_PEN,			/* single pen transparency with remapping */
	TRANSPARENCY_PEN_RAW,		/* single pen transparency with no remapping */
	TRANSPARENCY_PENS,			/* multiple pen transparency with remapping */
	TRANSPARENCY_PENS_RAW,		/* multiple pen transparency with no remapping */
	TRANSPARENCY_COLOR,			/* single remapped pen transparency with remapping */
	TRANSPARENCY_PEN_TABLE,		/* special pen remapping modes (see DRAWMODE_xxx below) with remapping */
	TRANSPARENCY_PEN_TABLE_RAW,	/* special pen remapping modes (see DRAWMODE_xxx below) with no remapping */
	TRANSPARENCY_BLEND,			/* blend two bitmaps, shifting the source and ORing to the dest with remapping */
	TRANSPARENCY_BLEND_RAW,		/* blend two bitmaps, shifting the source and ORing to the dest with no remapping */
	TRANSPARENCY_ALPHA,			/* single pen transparency, other pens alpha */
	TRANSPARENCY_ALPHARANGE,	/* single pen transparency, multiple pens alpha depending on array, see psikyosh.c */

	TRANSPARENCY_MODES			/* total number of modes; must be last */
};

/* drawing mode case TRANSPARENCY_ALPHARANGE */
extern UINT8 gfx_alpharange_table[256];

/* drawing mode case TRANSPARENCY_PEN_TABLE */
extern UINT8 gfx_drawmode_table[256];
enum
{
	DRAWMODE_NONE,
	DRAWMODE_SOURCE,
	DRAWMODE_SHADOW
};

/* By default, when drawing sprites with pdrawgfx, shadows affect the sprites below them. */
/* Set this flag to 1 to make shadows only affect the background, leaving sprites at full brightness. */
extern int pdrawgfx_shadow_lowpri;


void drawgfx_init(running_machine *machine);

void decodechar(gfx_element *gfx,int num,const unsigned char *src,const gfx_layout *gl);
gfx_element *allocgfx(const gfx_layout *gl);
void decodegfx(gfx_element *gfx, const UINT8 *src, UINT32 first, UINT32 count);
void freegfx(gfx_element *gfx);
void drawgfx(mame_bitmap *dest,const gfx_element *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color);
void pdrawgfx(mame_bitmap *dest,const gfx_element *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color,
		UINT32 priority_mask);
void mdrawgfx(mame_bitmap *dest,const gfx_element *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color,
		UINT32 priority_mask);
void copybitmap(mame_bitmap *dest,mame_bitmap *src,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color);
void copybitmap_remap(mame_bitmap *dest,mame_bitmap *src,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color);
void copyscrollbitmap(mame_bitmap *dest,mame_bitmap *src,
		int rows,const int *rowscroll,int cols,const int *colscroll,
		const rectangle *clip,int transparency,int transparent_color);
void copyscrollbitmap_remap(mame_bitmap *dest,mame_bitmap *src,
		int rows,const int *rowscroll,int cols,const int *colscroll,
		const rectangle *clip,int transparency,int transparent_color);
void draw_scanline8(mame_bitmap *bitmap,int x,int y,int length,const UINT8 *src,pen_t *pens,int transparent_pen);
void draw_scanline16(mame_bitmap *bitmap,int x,int y,int length,const UINT16 *src,pen_t *pens,int transparent_pen);
void pdraw_scanline8(mame_bitmap *bitmap,int x,int y,int length,const UINT8 *src,pen_t *pens,int transparent_pen,int pri);
void pdraw_scanline16(mame_bitmap *bitmap,int x,int y,int length,const UINT16 *src,pen_t *pens,int transparent_pen,int pri);
void extract_scanline8(mame_bitmap *bitmap,int x,int y,int length,UINT8 *dst);
void extract_scanline16(mame_bitmap *bitmap,int x,int y,int length,UINT16 *dst);


/*
  Copy a bitmap applying rotation, zooming, and arbitrary distortion.
  This function works in a way that mimics some real hardware like the Konami
  051316, so it requires little or no further processing on the caller side.

  Two 16.16 fixed point counters are used to keep track of the position on
  the source bitmap. startx and starty are the initial values of those counters,
  indicating the source pixel that will be drawn at coordinates (0,0) in the
  destination bitmap. The destination bitmap is scanned left to right, top to
  bottom; every time the cursor moves one pixel to the right, incxx is added
  to startx and incxy is added to starty. Every time the cursor moves to the
  next line, incyx is added to startx and incyy is added to startyy.

  What this means is that if incxy and incyx are both 0, the bitmap will be
  copied with only zoom and no rotation. If e.g. incxx and incyy are both 0x8000,
  the source bitmap will be doubled.

  Rotation is performed this way:
  incxx = 0x10000 * cos(theta)
  incxy = 0x10000 * -sin(theta)
  incyx = 0x10000 * sin(theta)
  incyy = 0x10000 * cos(theta)
  this will perform a rotation around (0,0), you'll have to adjust startx and
  starty to move the center of rotation elsewhere.

  Optionally the bitmap can be tiled across the screen instead of doing a single
  copy. This is obtained by setting the wraparound parameter to true.
 */
void copyrozbitmap(mame_bitmap *dest,mame_bitmap *src,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,int wraparound,
		const rectangle *clip,int transparency,int transparent_color,UINT32 priority);

#define fillbitmap(dest, pen, clip) bitmap_fill(dest, clip, pen)

void fillbitmap(mame_bitmap *dest,pen_t pen,const rectangle *clip);
void drawgfxzoom( mame_bitmap *dest_bmp,const gfx_element *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color,int scalex,int scaley);
void pdrawgfxzoom( mame_bitmap *dest_bmp,const gfx_element *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color,int scalex,int scaley,
		UINT32 priority_mask);
void mdrawgfxzoom( mame_bitmap *dest_bmp,const gfx_element *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const rectangle *clip,int transparency,int transparent_color,int scalex,int scaley,
		UINT32 priority_mask);



/* Alpha blending functions */
INLINE void alpha_set_level(int level)
{
	assert(level >= 0 && level <= 256);
	drawgfx_alpha_cache.alphas = level;
	drawgfx_alpha_cache.alphad = 256 - level;
}


INLINE UINT32 alpha_blend16(UINT32 d, UINT32 s)
{
	return ((((s & 0x001f) * drawgfx_alpha_cache.alphas + (d & 0x001f) * drawgfx_alpha_cache.alphad) >> 8)) |
		   ((((s & 0x03e0) * drawgfx_alpha_cache.alphas + (d & 0x03e0) * drawgfx_alpha_cache.alphad) >> 8) & 0x03e0) |
		   ((((s & 0x7c00) * drawgfx_alpha_cache.alphas + (d & 0x7c00) * drawgfx_alpha_cache.alphad) >> 8) & 0x7c00);
}


INLINE UINT32 alpha_blend32(UINT32 d, UINT32 s)
{
	return ((((s & 0x0000ff) * drawgfx_alpha_cache.alphas + (d & 0x0000ff) * drawgfx_alpha_cache.alphad) >> 8)) |
		   ((((s & 0x00ff00) * drawgfx_alpha_cache.alphas + (d & 0x00ff00) * drawgfx_alpha_cache.alphad) >> 8) & 0x00ff00) |
		   ((((s & 0xff0000) * drawgfx_alpha_cache.alphas + (d & 0xff0000) * drawgfx_alpha_cache.alphad) >> 8) & 0xff0000);
}


INLINE UINT32 alpha_blend_r16(UINT32 d, UINT32 s, UINT8 level)
{
	int alphad = 256 - level;
	return ((((s & 0x001f) * level + (d & 0x001f) * alphad) >> 8)) |
		   ((((s & 0x03e0) * level + (d & 0x03e0) * alphad) >> 8) & 0x03e0) |
		   ((((s & 0x7c00) * level + (d & 0x7c00) * alphad) >> 8) & 0x7c00);
}


INLINE UINT32 alpha_blend_r32( UINT32 d, UINT32 s, UINT8 level )
{
	int alphad = 256 - level;
	return ((((s & 0x0000ff) * level + (d & 0x0000ff) * alphad) >> 8)) |
		   ((((s & 0x00ff00) * level + (d & 0x00ff00) * alphad) >> 8) & 0x00ff00) |
		   ((((s & 0xff0000) * level + (d & 0xff0000) * alphad) >> 8) & 0xff0000);
}


#endif	/* __DRAWGFX_H__ */
