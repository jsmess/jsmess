/***************************************************************************

    tilemap.h

    Generic tilemap management system.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __TILEMAP_H__
#define __TILEMAP_H__

#include "mamecore.h"

typedef struct _tilemap tilemap;

#define ALL_TILEMAPS	0
/* ALL_TILEMAPS may be used with:
    tilemap_set_flip, tilemap_mark_all_tiles_dirty
*/

/* tilemap types */
enum _tilemap_type
{
	TILEMAP_TYPE_OPAQUE,
	TILEMAP_TYPE_TRANSPARENT,
	TILEMAP_TYPE_SPLIT,
	TILEMAP_TYPE_BITMASK,
	TILEMAP_TYPE_TRANSPARENT_COLOR,
	TILEMAP_TYPE_SPLIT_PENBIT
};
typedef enum _tilemap_type tilemap_type;


/* Set transparency_pen to a mask.  pen&mask determines whether each pixel is in front or back half */
/*
    TILEMAP_TYPE_SPLIT should be used if the pixels from a single tile
    can appear in more than one plane.

    TILEMAP_TYPE_BITMASK is used by Namco System1, Namco System2, NamcoNA1/2, Namco NB1
*/

#define TILEMAP_IGNORE_TRANSPARENCY		0x10
#define TILEMAP_BACK					0x20
#define TILEMAP_FRONT					0x40
#define TILEMAP_ALPHA					0x80

/*
    when rendering a split layer, pass TILEMAP_FRONT or TILEMAP_BACK or'd with the
    tile_priority value to specify the part to draw.

    when rendering a layer in alpha mode, the priority parameter
    becomes the alpha parameter (0..255).  Split mode is still
    available in alpha mode, ignore_transparency isn't.
*/

typedef struct _tile_data tile_data;
struct _tile_data
{
	/*
        you must set tile_info.pen_data, tile_info.pal_data and tile_info.pen_usage
        in the callback.  You can use the SET_TILE_INFO() macro below to do this.
        tile_info.flags and tile_info.priority will be automatically preset to 0,
        games that don't need them don't need to explicitly set them to 0
    */
	const UINT8 *pen_data;
	const pen_t *pal_data;
	UINT32 flags;
	UINT32 priority;		/* tile priority */
	UINT32 transpens;		/* additional per-tile transparent pens for TILEMAP_TYPE_SPLIT */
	UINT8 *mask_data;		/* for TILEMAP_TYPE_BITMASK */
	void *user_data;		/* user-supplied tilemap-wide pointer */
};

typedef void (*tile_get_info_fn)(running_machine *machine, tile_data *tileinfo, int tile_index);

#define TILE_GET_INFO( _name ) void _name(running_machine *machine, tile_data *tileinfo, int tile_index)

#define SET_TILE_INFO(GFX,CODE,COLOR,FLAGS) { \
	const gfx_element *gfx = machine->gfx[(GFX)]; \
	int _code = (CODE) % gfx->total_elements; \
	tileinfo->pen_data = gfx->gfxdata + _code*gfx->char_modulo; \
	tileinfo->pal_data = &gfx->colortable[gfx->color_granularity * (COLOR)]; \
	tileinfo->flags = FLAGS; \
	if (gfx->flags & GFX_PACKED) tileinfo->flags |= TILE_4BPP; \
}

/* tile flags, set by get_tile_info callback */
/* TILE_IGNORE_TRANSPARENCY is used if you need an opaque tile in a transparent layer. */
#define TILE_FLIPX					0x01
#define TILE_FLIPY					0x02
#define TILE_IGNORE_TRANSPARENCY	0x08
#define TILE_4BPP					0x10
/*      TILE_SPLIT                  0x60 */

/* TILE_SPLIT is for use with TILEMAP_TYPE_SPLIT layers.  It selects transparency type. */
#define TILE_SPLIT_OFFSET			5
#define TILE_SPLIT(T)				((T)<<TILE_SPLIT_OFFSET)

#define TILE_FLIPYX(YX)				(YX)
#define TILE_FLIPXY(XY)				((((XY)>>1)|((XY)<<1))&3)
/*
    TILE_FLIPYX is a shortcut that can be used by approx 80% of games,
    since yflip frequently occurs one bit higher than xflip within a
    tile attributes byte.
*/

#define TILE_LINE_DISABLED 0x80000000

extern mame_bitmap *priority_bitmap;

/* don't call these from drivers - they are called from mame.c */
void tilemap_init( running_machine *machine );

tilemap *tilemap_create(
	tile_get_info_fn tile_get_info,
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ),
	tilemap_type type,
	int tile_width, int tile_height,
	int num_cols, int num_rows );

void tilemap_set_transparent_pen( tilemap *tmap, int pen );
void tilemap_set_transmask( tilemap *tmap, int which, UINT32 fgmask, UINT32 bgmask );
void tilemap_set_depth( tilemap *tmap, int tile_depth, int tile_granularity );

void tilemap_mark_tile_dirty( tilemap *tmap, int memory_offset );
void tilemap_mark_all_tiles_dirty( tilemap *tmap );

void tilemap_set_scroll_rows( tilemap *tmap, int scroll_rows ); /* default: 1 */
void tilemap_set_scrolldx( tilemap *tmap, int dx, int dx_if_flipped );
void tilemap_set_scrollx( tilemap *tmap, int row, int value );
int tilemap_get_scrolldx( tilemap *tmap );

void tilemap_set_scroll_cols( tilemap *tmap, int scroll_cols ); /* default: 1 */
void tilemap_set_scrolldy( tilemap *tmap, int dy, int dy_if_flipped );
void tilemap_set_scrolly( tilemap *tmap, int col, int value );
int tilemap_get_scrolldy( tilemap *tmap );

void tilemap_set_palette_offset( tilemap *tmap, int offset );
void tilemap_set_user_data( tilemap *tmap, void *user_data );

#define TILEMAP_FLIPX 0x1
#define TILEMAP_FLIPY 0x2
void tilemap_set_flip( tilemap *tmap, int attributes );
void tilemap_set_enable( tilemap *tmap, int enable );

void tilemap_draw( mame_bitmap *dest, const rectangle *cliprect, tilemap *tmap, UINT32 flags, UINT32 priority );
void tilemap_draw_primask( mame_bitmap *dest, const rectangle *cliprect, tilemap *tmap, UINT32 flags, UINT32 priority, UINT32 priority_mask );

void tilemap_draw_roz(mame_bitmap *dest,const rectangle *cliprect,tilemap *tmap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,
		int wraparound,
		UINT32 flags, UINT32 priority );

void tilemap_draw_roz_primask(mame_bitmap *dest,const rectangle *cliprect,tilemap *tmap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,
		int wraparound,
		UINT32 flags, UINT32 priority, UINT32 priority_mask );

/* ----xxxx tile priority
 * ---x---- opaque in foreground
 * --x----- opaque in background
 * -x------ reserved
 * x------- tile-is-dirty
 */
#define TILE_FLAG_TILE_PRIORITY	(0x0f)
#define TILE_FLAG_FG_OPAQUE		(0x10)
#define TILE_FLAG_BG_OPAQUE		(0x20)

mame_bitmap *tilemap_get_pixmap( tilemap * tmap );
mame_bitmap *tilemap_get_transparency_bitmap( tilemap * tmap );
UINT8 *tilemap_get_transparency_data( tilemap * tmap );  //*

/*********************************************************************/

UINT32 tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_cols_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_cols_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_cols_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );

UINT32 tilemap_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_rows_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_rows_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_rows_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );

/* For showcharset()'s sake */
UINT32 tilemap_count( void );
void tilemap_nb_size( UINT32 number, UINT32 *width, UINT32 *height );
void tilemap_nb_draw( mame_bitmap *dest, UINT32 number, UINT32 scrollx, UINT32 scrolly );

#endif	/* __TILEMAP_H__ */
