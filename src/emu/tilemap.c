/***************************************************************************

    tilemap.c

    Generic tilemap management system.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

/*
    When the videoram for a tile changes, call tilemap_mark_tile_dirty
    with the appropriate memory offset.

    In the video driver, follow these steps:

    1)  Set each tilemap's scroll registers.

    2)  Call tilemap_draw to draw the tilemaps to the screen, from back to front.

    Notes:
    -   You can currently configure a tilemap as xscroll + scrolling columns or
        yscroll + scrolling rows, but not both types of scrolling simultaneously.
*/

#if !defined(DECLARE) && !defined(TRANSP)

#include "driver.h"
#include "osinline.h"
#include "tilemap.h"
#include "profiler.h"

#define SWAP(X,Y) { UINT32 temp=X; X=Y; Y=temp; }
#define MAX_TILESIZE 64

#define TILE_FLAG_DIRTY	(0x80)

typedef enum { eWHOLLY_TRANSPARENT, eWHOLLY_OPAQUE, eMASKED } trans_t;

typedef void (*tilemap_draw_func)( tilemap *tmap, int xpos, int ypos, int mask, int value );

struct _tilemap
{
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
	int *memory_offset_to_cached_indx;
	UINT32 *cached_indx_to_memory_offset;
	int logical_flip_to_cached_flip[4];

	/* callback to interpret video RAM for the tilemap */
	void (*tile_get_info)( int memory_offset );
	void *user_data;

	UINT32 max_memory_offset;
	UINT32 num_tiles;
	UINT32 num_pens;

	UINT32 num_logical_rows, num_logical_cols;
	UINT32 num_cached_rows, num_cached_cols;

	UINT32 logical_tile_width, logical_tile_height;
	UINT32 cached_tile_width, cached_tile_height;

	UINT32 cached_width, cached_height;

	INT32 dx, dx_if_flipped;
	INT32 dy, dy_if_flipped;
	INT32 scrollx_delta, scrolly_delta;

	INT32 enable;
	INT32 attributes;

	int type;
	INT32 transparent_pen;
	UINT32 fgmask[4], bgmask[4]; /* for TILEMAP_SPLIT */

	UINT32 *pPenToPixel[4];

	UINT8 (*draw_tile)( tilemap *tmap, UINT32 col, UINT32 row, UINT32 flags );

	INT32 cached_scroll_rows, cached_scroll_cols;
	INT32 *cached_rowscroll, *cached_colscroll;

	INT32 logical_scroll_rows, logical_scroll_cols;
	INT32 *logical_rowscroll, *logical_colscroll;

	INT32 orientation;
	INT32 palette_offset;

	UINT16 tile_depth, tile_granularity;
	UINT8 all_tiles_dirty;
	UINT8 all_tiles_clean;

	/* cached color data */
	mame_bitmap *pixmap;
	UINT32 pixmap_pitch_line;
	UINT32 pixmap_pitch_row;

	mame_bitmap *transparency_bitmap;
	UINT32 transparency_bitmap_pitch_line;
	UINT32 transparency_bitmap_pitch_row;
	UINT8 *transparency_data, **transparency_data_row;

	struct _tilemap *next; /* resource tracking */
};

mame_bitmap *		priority_bitmap;
UINT32					priority_bitmap_pitch_line;
UINT32					priority_bitmap_pitch_row;

static tilemap *	first_tilemap; /* resource tracking */
static UINT32			screen_width, screen_height;
tile_data				tile_info;

typedef void (*blitmask_t)( void *dest, const void *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode );
typedef void (*blitopaque_t)( void *dest, const void *source, int count, UINT8 *pri, UINT32 pcode );

/* the following parameters are constant across tilemap_draw calls */
static struct
{
	blitmask_t draw_masked;
	blitopaque_t draw_opaque;
	int clip_left, clip_top, clip_right, clip_bottom;
	UINT32 tilemap_priority_code;
	mame_bitmap *	screen_bitmap;
	UINT32				screen_bitmap_pitch_line;
	UINT32				screen_bitmap_pitch_row;
} blit;

/***********************************************************************************/

static void tilemap_exit( running_machine *machine );
static void tilemap_dispose( tilemap *tmap );
static void PenToPixel_Init( tilemap *tmap );
static void PenToPixel_Term( tilemap *tmap );
static void mappings_create( tilemap *tmap );
static void mappings_dispose( tilemap *tmap );
static void mappings_update( tilemap *tmap );
static void recalculate_scroll( tilemap *tmap );

static void install_draw_handlers( tilemap *tmap );

static void update_tile_info( tilemap *tmap, UINT32 cached_indx, UINT32 cached_col, UINT32 cached_row );

/***********************************************************************************/

static void PenToPixel_Init( tilemap *tmap )
{
	/*
        Construct a table for all tile orientations in advance.
        This simplifies drawing tiles and masks tremendously.
        If performance is an issue, we can always (re)introduce
        customized code for each case and forgo tables.
    */
	int i,x,y,tx,ty;
	UINT32 *pPenToPixel;

	for( i=0; i<4; i++ )
	{
		pPenToPixel = malloc_or_die( tmap->num_pens*sizeof(UINT32) );

		tmap->pPenToPixel[i] = pPenToPixel;
		for( ty=0; ty<tmap->cached_tile_height; ty++ )
		{
			for( tx=0; tx<tmap->cached_tile_width; tx++ )
			{
				x = tx;
				y = ty;
				if( i&TILE_FLIPX ) x = tmap->cached_tile_width-1-x;
				if( i&TILE_FLIPY ) y = tmap->cached_tile_height-1-y;
				*pPenToPixel++ = x+y*MAX_TILESIZE;
			}
		}
	}
}

static void PenToPixel_Term( tilemap *tmap )
{
	int i;
	for( i=0; i<4; i++ )
	{
		free( tmap->pPenToPixel[i] );
	}
}

void tilemap_set_transparent_pen( tilemap *tmap, int pen )
{
	tmap->transparent_pen = pen;
}

void tilemap_set_transmask( tilemap *tmap, int which, UINT32 fgmask, UINT32 bgmask )
{
	if( tmap->fgmask[which] != fgmask || tmap->bgmask[which] != bgmask )
	{
		tmap->fgmask[which] = fgmask;
		tmap->bgmask[which] = bgmask;
		tilemap_mark_all_tiles_dirty( tmap );
	}
}

void tilemap_set_depth( tilemap *tmap, int tile_depth, int tile_granularity )
{
	tmap->tile_depth = tile_depth;
	tmap->tile_granularity = tile_granularity;
}

/***********************************************************************************/
/* some common mappings */

UINT32 tilemap_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*num_cols + col;
}
UINT32 tilemap_scan_rows_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*num_cols + (num_cols-col-1);
}
UINT32 tilemap_scan_rows_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_rows-row-1)*num_cols + col;
}
UINT32 tilemap_scan_rows_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_rows-row-1)*num_cols + (num_cols-col-1);
}

UINT32 tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*num_rows + row;
}
UINT32 tilemap_scan_cols_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_cols-col-1)*num_rows + row;
}
UINT32 tilemap_scan_cols_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*num_rows + (num_rows-row-1);
}
UINT32 tilemap_scan_cols_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_cols-col-1)*num_rows + (num_rows-row-1);
}

/***********************************************************************************/

static void mappings_create( tilemap *tmap )
{
	int max_memory_offset = 0;
	UINT32 col,row;
	UINT32 num_logical_rows = tmap->num_logical_rows;
	UINT32 num_logical_cols = tmap->num_logical_cols;
	/* count offsets (might be larger than num_tiles) */
	for( row=0; row<num_logical_rows; row++ )
	{
		for( col=0; col<num_logical_cols; col++ )
		{
			UINT32 memory_offset = tmap->get_memory_offset( col, row, num_logical_cols, num_logical_rows );
			if( memory_offset>max_memory_offset ) max_memory_offset = memory_offset;
		}
	}
	max_memory_offset++;
	tmap->max_memory_offset = max_memory_offset;
	/* logical to cached (tmap_mark_dirty) */
	tmap->memory_offset_to_cached_indx = malloc_or_die( sizeof(int)*max_memory_offset );
	/* cached to logical (get_tile_info) */
	tmap->cached_indx_to_memory_offset = malloc_or_die( sizeof(UINT32)*tmap->num_tiles );
}

static void mappings_dispose( tilemap *tmap )
{
	free( tmap->cached_indx_to_memory_offset );
	free( tmap->memory_offset_to_cached_indx );
}

static void mappings_update( tilemap *tmap )
{
	int logical_flip;
	UINT32 logical_indx, cached_indx;
	UINT32 num_cached_rows = tmap->num_cached_rows;
	UINT32 num_cached_cols = tmap->num_cached_cols;
	UINT32 num_logical_rows = tmap->num_logical_rows;
	UINT32 num_logical_cols = tmap->num_logical_cols;
	for( logical_indx=0; logical_indx<tmap->max_memory_offset; logical_indx++ )
	{
		tmap->memory_offset_to_cached_indx[logical_indx] = -1;
	}

	for( logical_indx=0; logical_indx<tmap->num_tiles; logical_indx++ )
	{
		UINT32 logical_col = logical_indx%num_logical_cols;
		UINT32 logical_row = logical_indx/num_logical_cols;
		int memory_offset = tmap->get_memory_offset( logical_col, logical_row, num_logical_cols, num_logical_rows );
		UINT32 cached_col = logical_col;
		UINT32 cached_row = logical_row;
		if( tmap->orientation & ORIENTATION_FLIP_X ) cached_col = (num_cached_cols-1)-cached_col;
		if( tmap->orientation & ORIENTATION_FLIP_Y ) cached_row = (num_cached_rows-1)-cached_row;
		cached_indx = cached_row*num_cached_cols+cached_col;
		tmap->memory_offset_to_cached_indx[memory_offset] = cached_indx;
		tmap->cached_indx_to_memory_offset[cached_indx] = memory_offset;
	}
	for( logical_flip = 0; logical_flip<4; logical_flip++ )
	{
		int cached_flip = logical_flip;
		if( tmap->attributes&TILEMAP_FLIPX ) cached_flip ^= TILE_FLIPX;
		if( tmap->attributes&TILEMAP_FLIPY ) cached_flip ^= TILE_FLIPY;
		tmap->logical_flip_to_cached_flip[logical_flip] = cached_flip;
	}
}

/***********************************************************************************/

static void pio( void *dest, const void *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	if (pcode)
		for( i=0; i<count; i++ )
		{
			pri[i] = (pri[i] & (pcode >> 8)) | pcode;
		}
}

static void pit( void *dest, const void *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	if (pcode)
		for( i=0; i<count; i++ )
		{
			if( (pMask[i]&mask)==value )
			{
				pri[i] = (pri[i] & (pcode >> 8)) | pcode;
			}
		}
}

/***********************************************************************************/

#ifndef pdo16
static void pdo16( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	memcpy( dest,source,count*sizeof(UINT16) );
	for( i=0; i<count; i++ )
	{
		pri[i] = (pri[i] & (pcode >> 8)) | pcode;
	}
}
#endif

#ifndef pdo16pal
static void pdo16pal( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int pal = pcode >> 16;
	int i;
	for( i=0; i<count; i++ )
	{
		dest[i] = source[i] + pal;
		pri[i] = (pri[i] & (pcode >> 8)) | pcode;
	}
}
#endif

static void pdo16palnp( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int pal = pcode >> 16;
	int i;
	for( i=0; i<count; i++ )
	{
		dest[i] = source[i] + pal;
	}
}

#ifndef pdo16np
static void pdo16np( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	memcpy( dest,source,count*sizeof(UINT16) );
}
#endif

static void pdo15( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = clut[source[i]];
		pri[i] = (pri[i] & (pcode >> 8)) | pcode;
	}
}

static void pdo15np( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = clut[source[i]];
	}
}

#ifndef pdo32
static void pdo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = clut[source[i]];
		pri[i] = (pri[i] & (pcode >> 8)) | pcode;
	}
}
#endif

static void pdo32np( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = clut[source[i]];
	}
}

#ifndef npdo32
static void npdo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<oddcount; i++ )
	{
		dest[i] = clut[source[i]];
	}
	source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		UINT32 eax, ebx;
		eax = source[i  ];
		ebx = source[i+1];
		eax = clut[eax];
		ebx = clut[ebx];
		dest[i  ] = eax;
		eax = source[i+2];
		dest[i+1] = ebx;
		ebx = source[i+3];
		eax = clut[eax];
		ebx = clut[ebx];
		dest[i+2] = eax;
		dest[i+3] = ebx;
	}
}
#endif

/***********************************************************************************/

#ifndef pdt16
static void pdt16( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = source[i];
			pri[i] = (pri[i] & (pcode >> 8)) | pcode;
		}
	}
}
#endif

#ifndef pdt16pal
static void pdt16pal( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int pal = pcode >> 16;
	int i;

	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = source[i] + pal;
			pri[i] = (pri[i] & (pcode >> 8)) | pcode;
		}
	}
}
#endif

#ifndef pdt16np
static void pdt16np( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
			dest[i] = source[i];
	}
}
#endif

static void pdt15( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = clut[source[i]];
			pri[i] = (pri[i] & (pcode >> 8)) | pcode;
		}
	}
}

#ifndef pdt32
static void pdt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = clut[source[i]];
			pri[i] = (pri[i] & (pcode >> 8)) | pcode;
		}
	}
}
#endif

#ifndef npdt32
static void npdt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];

	for( i=0; i<oddcount; i++ )
	{
		if( (pMask[i]&mask)==value ) dest[i] = clut[source[i]];
	}
	pMask += count, source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		if( (pMask[i  ]&mask)==value ) dest[i  ] = clut[source[i  ]];
		if( (pMask[i+1]&mask)==value ) dest[i+1] = clut[source[i+1]];
		if( (pMask[i+2]&mask)==value ) dest[i+2] = clut[source[i+2]];
		if( (pMask[i+3]&mask)==value ) dest[i+3] = clut[source[i+3]];
	}
}
#endif

/***********************************************************************************/

static void pbo15( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = alpha_blend16(dest[i], clut[source[i]]);
		pri[i] = (pri[i] & (pcode >> 8)) | pcode;
	}
}

#ifndef pbo32
static void pbo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = alpha_blend32(dest[i], clut[source[i]]);
		pri[i] = (pri[i] & (pcode >> 8)) | pcode;
	}
}
#endif

#ifndef npbo32
static void npbo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<oddcount; i++ )
	{
		dest[i] = alpha_blend32(dest[i], clut[source[i]]);
	}
	source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		dest[i  ] = alpha_blend32(dest[i  ], clut[source[i  ]]);
		dest[i+1] = alpha_blend32(dest[i+1], clut[source[i+1]]);
		dest[i+2] = alpha_blend32(dest[i+2], clut[source[i+2]]);
		dest[i+3] = alpha_blend32(dest[i+3], clut[source[i+3]]);
	}
}
#endif

/***********************************************************************************/

static void pbt15( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = alpha_blend16(dest[i], clut[source[i]]);
			pri[i] = (pri[i] & (pcode >> 8)) | pcode;
		}
	}
}

#ifndef pbt32
static void pbt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = alpha_blend32(dest[i], clut[source[i]]);
			pri[i] = (pri[i] & (pcode >> 8)) | pcode;
		}
	}
}
#endif

#ifndef npbt32
static void npbt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];

	for( i=0; i<oddcount; i++ )
	{
		if( (pMask[i]&mask)==value ) dest[i] = alpha_blend32(dest[i], clut[source[i]]);
	}
	pMask += count, source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		if( (pMask[i  ]&mask)==value ) dest[i  ] = alpha_blend32(dest[i  ], clut[source[i  ]]);
		if( (pMask[i+1]&mask)==value ) dest[i+1] = alpha_blend32(dest[i+1], clut[source[i+1]]);
		if( (pMask[i+2]&mask)==value ) dest[i+2] = alpha_blend32(dest[i+2], clut[source[i+2]]);
		if( (pMask[i+3]&mask)==value ) dest[i+3] = alpha_blend32(dest[i+3], clut[source[i+3]]);
	}
}
#endif

/***********************************************************************************/

#define DEPTH 16
#define DATA_TYPE UINT16
#define DECLARE(function,args,body) static void function##16BPP args body
#include "tilemap.c"

#define DEPTH 32
#define DATA_TYPE UINT32
#define DECLARE(function,args,body) static void function##32BPP args body
#include "tilemap.c"

#define PAL_INIT const pen_t *pPalData = tile_info.pal_data
#define PAL_GET(pen) pPalData[pen]
#define TRANSP(f) f ## _ind
#include "tilemap.c"

#define PAL_INIT int palBase = tile_info.pal_data - Machine->remapped_colortable
#define PAL_GET(pen) (palBase + (pen))
#define TRANSP(f) f ## _raw
#include "tilemap.c"

/*********************************************************************************/

static void install_draw_handlers( tilemap *tmap )
{
	if( Machine->game_colortable )
	{
		if( tmap->type & TILEMAP_BITMASK )
			tmap->draw_tile = HandleTransparencyBitmask_ind;
		else if( tmap->type & TILEMAP_SPLIT_PENBIT )
			tmap->draw_tile = HandleTransparencyPenBit_ind;
		else if( tmap->type & TILEMAP_SPLIT )
			tmap->draw_tile = HandleTransparencyPens_ind;
		else if( tmap->type==TILEMAP_TRANSPARENT )
			tmap->draw_tile = HandleTransparencyPen_ind;
		else if( tmap->type==TILEMAP_TRANSPARENT_COLOR )
			tmap->draw_tile = HandleTransparencyColor_ind;
		else
			tmap->draw_tile = HandleTransparencyNone_ind;
	}
	else
	{
		if( tmap->type & TILEMAP_BITMASK )
			tmap->draw_tile = HandleTransparencyBitmask_raw;
		else if( tmap->type & TILEMAP_SPLIT_PENBIT )
			tmap->draw_tile = HandleTransparencyPenBit_raw;
		else if( tmap->type & TILEMAP_SPLIT )
			tmap->draw_tile = HandleTransparencyPens_raw;
		else if( tmap->type==TILEMAP_TRANSPARENT )
			tmap->draw_tile = HandleTransparencyPen_raw;
		else if( tmap->type==TILEMAP_TRANSPARENT_COLOR )
			tmap->draw_tile = HandleTransparencyColor_raw;
		else
			tmap->draw_tile = HandleTransparencyNone_raw;
	}
}

INLINE tilemap_draw_func pick_draw_func( mame_bitmap *dest )
{
	switch (dest ? dest->bpp : bitmap_format_to_bpp(Machine->screen[0].format))
	{
		case 32:
			return draw32BPP;

		case 16:
			return draw16BPP;
	}
	fatalerror("Illegal destination drawing depth");
	return NULL;
}


/***********************************************************************************/

void tilemap_init( running_machine *machine )
{
	screen_width	= Machine->screen[0].width;
	screen_height	= Machine->screen[0].height;
	first_tilemap	= NULL;

	priority_bitmap = auto_bitmap_alloc( screen_width, screen_height, BITMAP_FORMAT_INDEXED8 );
	priority_bitmap_pitch_line = priority_bitmap->rowpixels;
	add_exit_callback(machine, tilemap_exit);
}

static void tilemap_exit( running_machine *machine )
{
	tilemap *next;

	while( first_tilemap )
	{
		next = first_tilemap->next;
		tilemap_dispose( first_tilemap );
		first_tilemap = next;
	}
}

/***********************************************************************************/

static void tilemap_postload(void *param)
{
	tilemap *tmap = param;
	mappings_update(tmap);
	recalculate_scroll(tmap);
	tilemap_mark_all_tiles_dirty(tmap);
}


tilemap *tilemap_create(
	void (*tile_get_info)( int memory_offset ),
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ),
	int type,
	int tile_width, int tile_height,
	int num_cols, int num_rows )
{
	tilemap *tmap;
	UINT32 row;
	int num_tiles;
	int instance = 0;
	tilemap *tm;

	tmap = malloc_or_die( sizeof( tilemap ) );
	memset(tmap, 0, sizeof(tilemap));

	num_tiles = num_cols*num_rows;
	tmap->num_logical_cols = num_cols;
	tmap->num_logical_rows = num_rows;
	tmap->logical_tile_width = tile_width;
	tmap->logical_tile_height = tile_height;
	tmap->logical_colscroll = malloc_or_die(num_cols*tile_width*sizeof(INT32));
	memset(tmap->logical_colscroll, 0, num_cols*tile_width*sizeof(INT32));
	tmap->logical_rowscroll = malloc_or_die(num_rows*tile_height*sizeof(INT32));
	memset(tmap->logical_rowscroll, 0, num_rows*tile_height*sizeof(INT32));
	tmap->num_cached_cols = num_cols;
	tmap->num_cached_rows = num_rows;
	tmap->num_tiles = num_tiles;
	tmap->num_pens = tile_width*tile_height;
	tmap->cached_tile_width = tile_width;
	tmap->cached_tile_height = tile_height;
	tmap->cached_width = tile_width*num_cols;
	tmap->cached_height = tile_height*num_rows;
	tmap->tile_get_info = tile_get_info;
	tmap->get_memory_offset = get_memory_offset;
	tmap->orientation = ROT0;

	/* various defaults */
	tmap->enable = 1;
	tmap->type = type;
	tmap->logical_scroll_rows = tmap->cached_scroll_rows = 1;
	tmap->logical_scroll_cols = tmap->cached_scroll_cols = 1;
	tmap->transparent_pen = -1;
	tmap->tile_depth = 0;
	tmap->tile_granularity = 0;

	tmap->cached_rowscroll	= malloc_or_die(tmap->cached_height * sizeof(INT32));
	memset(tmap->cached_rowscroll, 0, tmap->cached_height * sizeof(INT32));
	tmap->cached_colscroll	= malloc_or_die(tmap->cached_width * sizeof(INT32));
	memset(tmap->cached_colscroll, 0, tmap->cached_width * sizeof(INT32));

	tmap->transparency_data = malloc_or_die( num_tiles );
	tmap->transparency_data_row = malloc_or_die( sizeof(UINT8 *)*num_rows );

	tmap->pixmap = bitmap_alloc( tmap->cached_width, tmap->cached_height, BITMAP_FORMAT_INDEXED16 );
	tmap->transparency_bitmap = bitmap_alloc( tmap->cached_width, tmap->cached_height, BITMAP_FORMAT_INDEXED8 );

	mappings_create( tmap );

	tmap->pixmap_pitch_line = tmap->pixmap->rowpixels;
	tmap->pixmap_pitch_row = tmap->pixmap_pitch_line*tile_height;

	tmap->transparency_bitmap_pitch_line = tmap->transparency_bitmap->rowpixels;
	tmap->transparency_bitmap_pitch_row = tmap->transparency_bitmap_pitch_line*tile_height;

	for( row=0; row<num_rows; row++ )
	{
		tmap->transparency_data_row[row] = tmap->transparency_data+num_cols*row;
	}
	install_draw_handlers( tmap );
	mappings_update( tmap );
	memset( tmap->transparency_data, TILE_FLAG_DIRTY, num_tiles );
	tmap->next = first_tilemap;
	first_tilemap = tmap;

	PenToPixel_Init( tmap );

	recalculate_scroll(tmap);

	/* compute the index of this entry */
	for (tm = first_tilemap; tm; tm = tm->next)
		instance++;

	/* save relevant state */
	state_save_register_item("tilemap", instance, tmap->enable);
	state_save_register_item("tilemap", instance, tmap->attributes);
	state_save_register_item("tilemap", instance, tmap->transparent_pen);
	state_save_register_item_array("tilemap", instance, tmap->fgmask);
	state_save_register_item_array("tilemap", instance, tmap->bgmask);
	state_save_register_item("tilemap", instance, tmap->dx);
	state_save_register_item("tilemap", instance, tmap->dx_if_flipped);
	state_save_register_item("tilemap", instance, tmap->dy);
	state_save_register_item("tilemap", instance, tmap->dy_if_flipped);
	state_save_register_item("tilemap", instance, tmap->cached_scroll_rows);
	state_save_register_item("tilemap", instance, tmap->cached_scroll_cols);
	state_save_register_item_pointer("tilemap", instance, tmap->cached_rowscroll, tmap->cached_height);
	state_save_register_item_pointer("tilemap", instance, tmap->cached_colscroll, tmap->cached_width);
	state_save_register_item("tilemap", instance, tmap->logical_scroll_rows);
	state_save_register_item("tilemap", instance, tmap->logical_scroll_cols);
	state_save_register_item_pointer("tilemap", instance, tmap->logical_rowscroll, num_rows*tile_height);
	state_save_register_item_pointer("tilemap", instance, tmap->logical_colscroll, num_cols*tile_width);
	state_save_register_item("tilemap", instance, tmap->orientation);
	state_save_register_item("tilemap", instance, tmap->palette_offset);
	state_save_register_item("tilemap", instance, tmap->tile_depth);
	state_save_register_item("tilemap", instance, tmap->tile_granularity);

	/* reset everything after a load */
	state_save_register_func_postload_ptr(tilemap_postload, tmap);
	return tmap;
}

static void tilemap_dispose( tilemap *tmap )
{
	tilemap *prev;

	if( tmap==first_tilemap )
	{
		first_tilemap = tmap->next;
	}
	else
	{
		prev = first_tilemap;
		while( prev && prev->next != tmap ) prev = prev->next;
		if( prev ) prev->next =tmap->next;
	}
	PenToPixel_Term( tmap );
	free( tmap->logical_rowscroll );
	free( tmap->cached_rowscroll );
	free( tmap->logical_colscroll );
	free( tmap->cached_colscroll );
	free( tmap->transparency_data );
	free( tmap->transparency_data_row );
	bitmap_free( tmap->transparency_bitmap );
	bitmap_free( tmap->pixmap );
	mappings_dispose( tmap );
	free( tmap );
}

/***********************************************************************************/

void tilemap_set_enable( tilemap *tmap, int enable )
{
	tmap->enable = enable?1:0;
}


void tilemap_set_flip( tilemap *tmap, int attributes )
{
	if( tmap==ALL_TILEMAPS )
	{
		tmap = first_tilemap;
		while( tmap )
		{
			tilemap_set_flip( tmap, attributes );
			tmap = tmap->next;
		}
	}
	else if( tmap->attributes!=attributes )
	{
		tmap->attributes = attributes;
		tmap->orientation = ROT0;
		if( attributes&TILEMAP_FLIPY )
		{
			tmap->orientation ^= ORIENTATION_FLIP_Y;
		}

		if( attributes&TILEMAP_FLIPX )
		{
			tmap->orientation ^= ORIENTATION_FLIP_X;
		}

		mappings_update( tmap );
		recalculate_scroll( tmap );
		tilemap_mark_all_tiles_dirty( tmap );
	}
}

/***********************************************************************************/

void tilemap_set_scroll_cols( tilemap *tmap, int n )
{
	tmap->logical_scroll_cols = n;
	tmap->cached_scroll_cols = n;
}

void tilemap_set_scroll_rows( tilemap *tmap, int n )
{
	tmap->logical_scroll_rows = n;
	tmap->cached_scroll_rows = n;
}

/***********************************************************************************/

void tilemap_mark_tile_dirty( tilemap *tmap, int memory_offset )
{
	if( memory_offset<tmap->max_memory_offset )
	{
		int cached_indx = tmap->memory_offset_to_cached_indx[memory_offset];
		if( cached_indx>=0 )
		{
			tmap->transparency_data[cached_indx] = TILE_FLAG_DIRTY;
			tmap->all_tiles_clean = 0;
		}
	}
}

void tilemap_mark_all_tiles_dirty( tilemap *tmap )
{
	if( tmap==ALL_TILEMAPS )
	{
		tmap = first_tilemap;
		while( tmap )
		{
			tilemap_mark_all_tiles_dirty( tmap );
			tmap = tmap->next;
		}
	}
	else
	{
		tmap->all_tiles_dirty = 1;
		tmap->all_tiles_clean = 0;
	}
}

/***********************************************************************************/

static void update_tile_info( tilemap *tmap, UINT32 cached_indx, UINT32 col, UINT32 row )
{
	UINT32 x0;
	UINT32 y0;
	UINT32 memory_offset;
	UINT32 flags;

profiler_mark(PROFILER_TILEMAP_UPDATE);

	memory_offset = tmap->cached_indx_to_memory_offset[cached_indx];
	tmap->tile_get_info( memory_offset );
	flags = tile_info.flags;
	flags = (flags&0xfc)|tmap->logical_flip_to_cached_flip[flags&0x3];
	x0 = tmap->cached_tile_width*col;
	y0 = tmap->cached_tile_height*row;

	tmap->transparency_data[cached_indx] = tmap->draw_tile(tmap,x0,y0,flags );

profiler_mark(PROFILER_END);
}

mame_bitmap *tilemap_get_pixmap( tilemap * tmap )
{
	UINT32 cached_indx = 0;
	UINT32 row,col;

	if (!tmap)
		return 0;

	if (tmap->all_tiles_clean == 0)
	{
profiler_mark(PROFILER_TILEMAP_DRAW);

		/* if the whole map is dirty, mark it as such */
		if (tmap->all_tiles_dirty)
		{
			memset( tmap->transparency_data, TILE_FLAG_DIRTY, tmap->num_tiles );
			tmap->all_tiles_dirty = 0;
		}

		memset( &tile_info, 0x00, sizeof(tile_info) ); /* initialize defaults */
		tile_info.user_data = tmap->user_data;

		/* walk over cached rows/cols (better to walk screen coords) */
		for( row=0; row<tmap->num_cached_rows; row++ )
		{
			for( col=0; col<tmap->num_cached_cols; col++ )
			{
				if( tmap->transparency_data[cached_indx] == TILE_FLAG_DIRTY )
				{
					update_tile_info( tmap, cached_indx, col, row );
				}
				cached_indx++;
			} /* next col */
		} /* next row */

		tmap->all_tiles_clean = 1;

profiler_mark(PROFILER_END);
	}

	return tmap->pixmap;
}

mame_bitmap *tilemap_get_transparency_bitmap( tilemap * tmap )
{
	return tmap->transparency_bitmap;
}

UINT8 *tilemap_get_transparency_data( tilemap * tmap ) //*
{
	return tmap->transparency_data;
}

/***********************************************************************************/

static void
recalculate_scroll( tilemap *tmap )
{
	int i;

	tmap->scrollx_delta = (tmap->attributes & TILEMAP_FLIPX )?tmap->dx_if_flipped:tmap->dx;
	tmap->scrolly_delta = (tmap->attributes & TILEMAP_FLIPY )?tmap->dy_if_flipped:tmap->dy;

	for( i=0; i<tmap->logical_scroll_rows; i++ )
	{
		tilemap_set_scrollx( tmap, i, tmap->logical_rowscroll[i] );
	}
	for( i=0; i<tmap->logical_scroll_cols; i++ )
	{
		tilemap_set_scrolly( tmap, i, tmap->logical_colscroll[i] );
	}
}

void
tilemap_set_scrolldx( tilemap *tmap, int dx, int dx_if_flipped )
{
	tmap->dx = dx;
	tmap->dx_if_flipped = dx_if_flipped;
	recalculate_scroll( tmap );
}

void
tilemap_set_scrolldy( tilemap *tmap, int dy, int dy_if_flipped )
{
	tmap->dy = dy;
	tmap->dy_if_flipped = dy_if_flipped;
	recalculate_scroll( tmap );
}

int
tilemap_get_scrolldx( tilemap *tmap )
{
	return (tmap->attributes & TILEMAP_FLIPX) ? tmap->dx_if_flipped : tmap->dx;
}

int
tilemap_get_scrolldy( tilemap *tmap )
{
	return (tmap->attributes & TILEMAP_FLIPY) ? tmap->dy_if_flipped : tmap->dy;
}

void tilemap_set_scrollx( tilemap *tmap, int which, int value )
{
	tmap->logical_rowscroll[which] = value;
	value = tmap->scrollx_delta-value; /* adjust */

	if( tmap->orientation & ORIENTATION_FLIP_Y )
	{
		/* adjust affected row */
		which = tmap->cached_scroll_rows-1 - which;
	}
	if( tmap->orientation & ORIENTATION_FLIP_X )
	{
		/* adjust scroll amount */
		value = screen_width-tmap->cached_width-value;
	}
	tmap->cached_rowscroll[which] = value;
}

void tilemap_set_scrolly( tilemap *tmap, int which, int value )
{
	tmap->logical_colscroll[which] = value;
	value = tmap->scrolly_delta - value; /* adjust */

	if( tmap->orientation & ORIENTATION_FLIP_X )
	{
		/* adjust affected col */
		which = tmap->cached_scroll_cols-1 - which;
	}
	if( tmap->orientation & ORIENTATION_FLIP_Y )
	{
		/* adjust scroll amount */
		value = screen_height-tmap->cached_height-value;
	}
	tmap->cached_colscroll[which] = value;
}

/***********************************************************************************/

void tilemap_set_palette_offset( tilemap *tmap, int offset )
{
	tmap->palette_offset = offset;
}

/***********************************************************************************/

void tilemap_set_user_data( tilemap *tmap, void *user_data )
{
	tmap->user_data = user_data;
}

/***********************************************************************************/

void tilemap_draw( mame_bitmap *dest, const rectangle *cliprect, tilemap *tmap, UINT32 flags, UINT32 priority )
{
	tilemap_draw_primask( dest, cliprect, tmap, flags, priority, 0xff );
}

void tilemap_draw_primask( mame_bitmap *dest, const rectangle *cliprect, tilemap *tmap, UINT32 flags, UINT32 priority, UINT32 priority_mask )
{
	tilemap_draw_func drawfunc = pick_draw_func(dest);
	int xpos,ypos,mask,value;
	int rows, cols;
	const int *rowscroll, *colscroll;
	int left, right, top, bottom;

profiler_mark(PROFILER_TILEMAP_DRAW);
	if( tmap->enable )
	{
		/* scroll registers */
		rows		= tmap->cached_scroll_rows;
		cols		= tmap->cached_scroll_cols;
		rowscroll	= tmap->cached_rowscroll;
		colscroll	= tmap->cached_colscroll;

		/* clipping */
		if( cliprect )
		{
			left	= cliprect->min_x;
			top		= cliprect->min_y;
			right	= cliprect->max_x+1;
			bottom	= cliprect->max_y+1;
		}
		else
		{
			left	= 0;
			top		= 0;
			right	= tmap->cached_width;
			bottom	= tmap->cached_height;
		}

		/* tile priority */
		mask		= TILE_FLAG_TILE_PRIORITY;
		value		= TILE_FLAG_TILE_PRIORITY&flags;

		/* initialize defaults */
		memset( &tile_info, 0x00, sizeof(tile_info) );
		tile_info.user_data = tmap->user_data;

		/* if the whole map is dirty, mark it as such */
		if (tmap->all_tiles_dirty)
		{
			memset( tmap->transparency_data, TILE_FLAG_DIRTY, tmap->num_tiles );
			tmap->all_tiles_dirty = 0;
		}

		/* priority_bitmap_pitch_row is tmap-specific */
		priority_bitmap_pitch_row = priority_bitmap_pitch_line*tmap->cached_tile_height;

		blit.screen_bitmap = dest;
		if( dest == NULL )
		{
			blit.draw_masked = (blitmask_t)pit;
			blit.draw_opaque = (blitopaque_t)pio;
		}
		else
		{
			blit.screen_bitmap_pitch_line = dest->rowpixels;
			switch( dest->format )
			{
			case BITMAP_FORMAT_RGB32:
				if (priority)
				{
					if( flags&TILEMAP_ALPHA )
					{
						blit.draw_masked = (blitmask_t)pbt32;
						blit.draw_opaque = (blitopaque_t)pbo32;
					}
					else
					{
						blit.draw_masked = (blitmask_t)pdt32;
						blit.draw_opaque = (blitopaque_t)pdo32;
					}
				}
				else
				{
					//* AAT APR2003: added 32-bit no-priority counterpart
					if( flags&TILEMAP_ALPHA )
					{
						blit.draw_masked = (blitmask_t)npbt32;
						blit.draw_opaque = (blitopaque_t)npbo32;
					}
					else
					{
						blit.draw_masked = (blitmask_t)npdt32;
						blit.draw_opaque = (blitopaque_t)npdo32;
					}
				}
				break;
			case BITMAP_FORMAT_RGB15:
				if( flags&TILEMAP_ALPHA )
				{
					blit.draw_masked = (blitmask_t)pbt15;
					blit.draw_opaque = (blitopaque_t)pbo15;
				}
				else
				{
					blit.draw_masked = (blitmask_t)pdt15;
					blit.draw_opaque = (blitopaque_t)pdo15;
				}
				break;
			case BITMAP_FORMAT_INDEXED16:
				if (tmap->palette_offset)
				{
					blit.draw_masked = (blitmask_t)pdt16pal;
					blit.draw_opaque = (blitopaque_t)pdo16pal;
				}
				else if (priority)
				{
					blit.draw_masked = (blitmask_t)pdt16;
					blit.draw_opaque = (blitopaque_t)pdo16;
				}
				else
				{
					blit.draw_masked = (blitmask_t)pdt16np;
					blit.draw_opaque = (blitopaque_t)pdo16np;
				}
				break;

			default:
				fatalerror("tilemap_draw_primask: Invalid bitmap format");
				break;
			}
			blit.screen_bitmap_pitch_row = blit.screen_bitmap_pitch_line*tmap->cached_tile_height;
		} /* dest == bitmap */

		if( !(tmap->type==TILEMAP_OPAQUE || (flags&TILEMAP_IGNORE_TRANSPARENCY)) )
		{
			if( flags&TILEMAP_BACK )
			{
				mask	|= TILE_FLAG_BG_OPAQUE;
				value	|= TILE_FLAG_BG_OPAQUE;
			}
			else
			{
				mask	|= TILE_FLAG_FG_OPAQUE;
				value	|= TILE_FLAG_FG_OPAQUE;
			}
		}

		blit.tilemap_priority_code = (priority & 0xff) | ((priority_mask & 0xff) << 8) | (tmap->palette_offset << 16);

		if( rows == 1 && cols == 1 )
		{ /* XY scrolling playfield */
			int scrollx = rowscroll[0];
			int scrolly = colscroll[0];

			if( scrollx < 0 )
			{
				scrollx = tmap->cached_width - (-scrollx) % tmap->cached_width;
			}
			else
			{
				scrollx = scrollx % tmap->cached_width;
			}

			if( scrolly < 0 )
			{
				scrolly = tmap->cached_height - (-scrolly) % tmap->cached_height;
			}
			else
			{
				scrolly = scrolly % tmap->cached_height;
			}

	 		blit.clip_left		= left;
	 		blit.clip_top		= top;
	 		blit.clip_right		= right;
	 		blit.clip_bottom	= bottom;

			for(
				ypos = scrolly - tmap->cached_height;
				ypos < blit.clip_bottom;
				ypos += tmap->cached_height )
			{
				for(
					xpos = scrollx - tmap->cached_width;
					xpos < blit.clip_right;
					xpos += tmap->cached_width )
				{
					drawfunc( tmap, xpos, ypos, mask, value );
				}
			}
		}
		else if( rows == 1 )
		{ /* scrolling columns + horizontal scroll */
			int col = 0;
			int colwidth = tmap->cached_width / cols;
			int scrollx = rowscroll[0];

			if( scrollx < 0 )
			{
				scrollx = tmap->cached_width - (-scrollx) % tmap->cached_width;
			}
			else
			{
				scrollx = scrollx % tmap->cached_width;
			}

			blit.clip_top		= top;
			blit.clip_bottom	= bottom;

			while( col < cols )
			{
				int cons	= 1;
				int scrolly	= colscroll[col];

	 			/* count consecutive columns scrolled by the same amount */
				if( scrolly != TILE_LINE_DISABLED )
				{
					while( col + cons < cols &&	colscroll[col + cons] == scrolly ) cons++;

					if( scrolly < 0 )
					{
						scrolly = tmap->cached_height - (-scrolly) % tmap->cached_height;
					}
					else
					{
						scrolly %= tmap->cached_height;
					}

					blit.clip_left = col * colwidth + scrollx;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - tmap->cached_height;
						ypos < blit.clip_bottom;
						ypos += tmap->cached_height )
					{
						drawfunc( tmap, scrollx, ypos, mask, value );
					}

					blit.clip_left = col * colwidth + scrollx - tmap->cached_width;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx - tmap->cached_width;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - tmap->cached_height;
						ypos < blit.clip_bottom;
						ypos += tmap->cached_height )
					{
						drawfunc( tmap, scrollx - tmap->cached_width, ypos, mask, value );
					}
				}
				col += cons;
			}
		}
		else if( cols == 1 )
		{ /* scrolling rows + vertical scroll */
			int row = 0;
			int rowheight = tmap->cached_height / rows;
			int scrolly = colscroll[0];
			if( scrolly < 0 )
			{
				scrolly = tmap->cached_height - (-scrolly) % tmap->cached_height;
			}
			else
			{
				scrolly = scrolly % tmap->cached_height;
			}
			blit.clip_left = left;
			blit.clip_right = right;
			while( row < rows )
			{
				int cons = 1;
				int scrollx = rowscroll[row];
				/* count consecutive rows scrolled by the same amount */
				if( scrollx != TILE_LINE_DISABLED )
				{
					while( row + cons < rows &&	rowscroll[row + cons] == scrollx ) cons++;
					if( scrollx < 0)
					{
						scrollx = tmap->cached_width - (-scrollx) % tmap->cached_width;
					}
					else
					{
						scrollx %= tmap->cached_width;
					}
					blit.clip_top = row * rowheight + scrolly;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - tmap->cached_width;
						xpos < blit.clip_right;
						xpos += tmap->cached_width )
					{
						drawfunc( tmap, xpos, scrolly, mask, value );
					}
					blit.clip_top = row * rowheight + scrolly - tmap->cached_height;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly - tmap->cached_height;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - tmap->cached_width;
						xpos < blit.clip_right;
						xpos += tmap->cached_width )
					{
						drawfunc( tmap, xpos, scrolly - tmap->cached_height, mask, value );
					}
				}
				row += cons;
			}
		}
	}
profiler_mark(PROFILER_END);
}

/* notes:
   - startx and starty MUST be UINT32 for calculations to work correctly
   - srcbitmap->width and height are assumed to be a power of 2 to speed up wraparound
   */
void tilemap_draw_roz( mame_bitmap *dest,const rectangle *cliprect,tilemap *tmap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,
		int wraparound,
		UINT32 flags, UINT32 priority )
{
	tilemap_draw_roz_primask( dest,cliprect,tmap,startx,starty,incxx,incxy,incyx,incyy,wraparound,flags,priority, 0xff );
}

void tilemap_draw_roz_primask( mame_bitmap *dest,const rectangle *cliprect,tilemap *tmap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,
		int wraparound,
		UINT32 flags, UINT32 priority, UINT32 priority_mask )
{
	if( (incxx == 1<<16) && !incxy & !incyx && (incyy == 1<<16) && wraparound )
	{
		tilemap_set_scrollx( tmap, 0, startx >> 16 );
		tilemap_set_scrolly( tmap, 0, starty >> 16 );
		tilemap_draw( dest, cliprect, tmap, flags, priority );
	}
	else
	{
		int mask,value;

profiler_mark(PROFILER_TILEMAP_DRAW_ROZ);
		if( tmap->enable )
		{
			/* tile priority */
			mask		= TILE_FLAG_TILE_PRIORITY;
			value		= TILE_FLAG_TILE_PRIORITY&flags;

			tilemap_get_pixmap( tmap ); /* force update */

			if( !(tmap->type==TILEMAP_OPAQUE || (flags&TILEMAP_IGNORE_TRANSPARENCY)) )
			{
				if( flags&TILEMAP_BACK )
				{
					mask	|= TILE_FLAG_BG_OPAQUE;
					value	|= TILE_FLAG_BG_OPAQUE;
				}
				else
				{
					mask	|= TILE_FLAG_FG_OPAQUE;
					value	|= TILE_FLAG_FG_OPAQUE;
				}
			}

			switch( dest->format )
			{
			case BITMAP_FORMAT_RGB32:

				/* Opaque drawing routines not present due to difficulty with
                optimization using current ROZ methods
                */
				if (priority)
				{
					if( flags&TILEMAP_ALPHA )
						blit.draw_masked = (blitmask_t)pbt32;
					else
						blit.draw_masked = (blitmask_t)pdt32;
				}
				else
				{
					if( flags&TILEMAP_ALPHA )
						blit.draw_masked = (blitmask_t)npbt32;
					else
						blit.draw_masked = (blitmask_t)npdt32;
				}

				copyroz_core32BPP(dest,tmap,startx,starty,incxx,incxy,incyx,incyy,
					wraparound,cliprect,mask,value,priority,priority_mask,tmap->palette_offset);
				break;

			case BITMAP_FORMAT_RGB15:
				if( flags&TILEMAP_ALPHA )
					blit.draw_masked = (blitmask_t)pbt15;
				else
					blit.draw_masked = (blitmask_t)pdt15;

				copyroz_core16BPP(dest,tmap,startx,starty,incxx,incxy,incyx,incyy,
					wraparound,cliprect,mask,value,priority,priority_mask,tmap->palette_offset);
				break;

			case BITMAP_FORMAT_INDEXED16:
				if (tmap->palette_offset)
					blit.draw_masked = (blitmask_t)pdt16pal;
				else if (priority)
					blit.draw_masked = (blitmask_t)pdt16;
				else
					blit.draw_masked = (blitmask_t)pdt16np;

				copyroz_core16BPP(dest,tmap,startx,starty,incxx,incxy,incyx,incyy,
					wraparound,cliprect,mask,value,priority,priority_mask,tmap->palette_offset);
				break;

			default:
				fatalerror("tilemap_draw_roz: Invalid bitmap format\n");
				break;
			}
		} /* tmap->enable */
profiler_mark(PROFILER_END);
	}
}

UINT32 tilemap_count( void )
{
	UINT32 count = 0;
	tilemap *tmap = first_tilemap;
	while( tmap )
	{
		count++;
		tmap = tmap->next;
	}
	return count;
}

static tilemap *tilemap_nb_find( int number )
{
	int count = 0;
	tilemap *tmap;

	tmap = first_tilemap;
	while( tmap )
	{
		count++;
		tmap = tmap->next;
	}

	number = (count-1)-number;

	tmap = first_tilemap;
	while( number-- )
	{
		tmap = tmap->next;
	}
	return tmap;
}

void tilemap_nb_size( UINT32 number, UINT32 *width, UINT32 *height )
{
	tilemap *tmap = tilemap_nb_find( number );
	*width  = tmap->cached_width;
	*height = tmap->cached_height;
}

void tilemap_nb_draw( mame_bitmap *dest, UINT32 number, UINT32 scrollx, UINT32 scrolly )
{
	tilemap_draw_func drawfunc = pick_draw_func(dest);
	int xpos,ypos;
	tilemap *tmap = tilemap_nb_find( number );

	blit.screen_bitmap = dest;
	blit.screen_bitmap_pitch_line = dest->rowpixels;
	switch( dest->format )
	{
	case BITMAP_FORMAT_RGB32:
	case BITMAP_FORMAT_ARGB32:
		blit.draw_opaque = (blitopaque_t)pdo32np;
		break;

	case BITMAP_FORMAT_RGB15:
		blit.draw_opaque = (blitopaque_t)pdo15np;
		break;

	case BITMAP_FORMAT_INDEXED16:
		blit.draw_opaque = (blitopaque_t)pdo16palnp;
		break;

	default:
		fatalerror("tilemap_nb_draw: Invalid bitmap format");
		break;
	}
	priority_bitmap_pitch_row = priority_bitmap_pitch_line*tmap->cached_tile_height;
	blit.screen_bitmap_pitch_row = blit.screen_bitmap_pitch_line*tmap->cached_tile_height;
	blit.tilemap_priority_code = (tmap->palette_offset << 16);
	scrollx = tmap->cached_width  - scrollx % tmap->cached_width;
	scrolly = tmap->cached_height - scrolly % tmap->cached_height;

	blit.clip_left		= 0;
	blit.clip_top		= 0;
	blit.clip_right		= (dest->width < tmap->cached_width) ? dest->width : tmap->cached_width;
	blit.clip_bottom	= (dest->height < tmap->cached_height) ? dest->height : tmap->cached_height;

	for(
		ypos = scrolly - tmap->cached_height;
		ypos < blit.clip_bottom;
		ypos += tmap->cached_height )
	{
		for(
			xpos = scrollx - tmap->cached_width;
			xpos < blit.clip_right;
			xpos += tmap->cached_width )
		{
			drawfunc( tmap, xpos, ypos, 0, 0 );
		}
	}
}


/***********************************************************************************/

#endif // !DECLARE && !TRANSP

#define ROZ_PLOT_PIXEL(INPUT_VAL)										\
	if (blit.draw_masked == (blitmask_t)pbt32)							\
	{																	\
		clut = &Machine->remapped_colortable[priority >> 16] ;			\
		*dest = alpha_blend32(*dest, clut[INPUT_VAL]);					\
		*pri = (*pri & priority_mask) | priority;						\
	}																	\
	else if (blit.draw_masked == (blitmask_t)pdt32)						\
	{																	\
		clut = &Machine->remapped_colortable[priority >> 16] ;			\
		*dest = clut[INPUT_VAL] ;										\
		*pri = (*pri & priority_mask) | priority;						\
	}																	\
	else if (blit.draw_masked == (blitmask_t)npbt32)					\
	{																	\
		clut = &Machine->remapped_colortable[priority >> 16] ;			\
		*dest = alpha_blend32(*dest, clut[INPUT_VAL]) ;					\
/*      logerror("PARTIALLY IMPLEMENTED ROZ VIDEO MODE - npbt32\n") ;*/	\
	}																	\
	else if (blit.draw_masked == (blitmask_t)npdt32)					\
	{																	\
		clut = &Machine->remapped_colortable[priority >> 16] ;			\
		*dest = clut[INPUT_VAL] ;										\
/*      logerror("PARTIALLY IMPLEMENTED ROZ VIDEO MODE - npbt32\n") ;*/	\
	}																	\
	else if (blit.draw_masked == (blitmask_t)pbt15)						\
	{																	\
		clut = &Machine->remapped_colortable[priority >> 16] ;			\
		*dest = alpha_blend16(*dest, clut[INPUT_VAL]) ;					\
		*pri = (*pri & priority_mask) | priority;						\
	}																	\
	else if (blit.draw_masked == (blitmask_t)pdt15)						\
	{																	\
		clut = &Machine->remapped_colortable[priority >> 16] ;			\
		*dest = clut[INPUT_VAL] ;										\
		*pri = (*pri & priority_mask) | priority;						\
	}																	\
	else if (blit.draw_masked == (blitmask_t)pdt16pal)					\
	{																	\
		*dest = (INPUT_VAL) + (priority >> 16) ;						\
		*pri = (*pri & priority_mask) | priority;						\
	}																	\
	else if (blit.draw_masked == (blitmask_t)pdt16)						\
	{																	\
		*dest = INPUT_VAL ;												\
		*pri = (*pri & priority_mask) | priority;						\
	}																	\
	else if (blit.draw_masked == (blitmask_t)pdt16np)					\
	{																	\
		*dest = INPUT_VAL ;												\
	}

#ifdef DECLARE

DECLARE(copyroz_core,(mame_bitmap *bitmap,tilemap *tmap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,int wraparound,
		const rectangle *clip,
		int mask,int value,
		UINT32 priority,UINT32 priority_mask,UINT32 palette_offset),
{
	UINT32 cx;
	UINT32 cy;
	int x;
	int sx;
	int sy;
	int ex;
	int ey;
	mame_bitmap *srcbitmap = tmap->pixmap;
	mame_bitmap *transparency_bitmap = tmap->transparency_bitmap;
	const int xmask = srcbitmap->width-1;
	const int ymask = srcbitmap->height-1;
	const int widthshifted = srcbitmap->width << 16;
	const int heightshifted = srcbitmap->height << 16;
	DATA_TYPE *dest;
	UINT8 *pri;
	const UINT16 *src;
	const UINT8 *pMask;

	pen_t *clut ;

	if (clip)
	{
		startx += clip->min_x * incxx + clip->min_y * incyx;
		starty += clip->min_x * incxy + clip->min_y * incyy;

		sx = clip->min_x;
		sy = clip->min_y;
		ex = clip->max_x;
		ey = clip->max_y;
	}
	else
	{
		sx = 0;
		sy = 0;
		ex = bitmap->width-1;
		ey = bitmap->height-1;
	}


	if (incxy == 0 && incyx == 0 && !wraparound)
	{
		/* optimized loop for the not rotated case */

		if (incxx == 0x10000)
		{
			/* optimized loop for the not zoomed case */

			/* startx is unsigned */
			startx = ((INT32)startx) >> 16;

			if (startx >= srcbitmap->width)
			{
				sx += -startx;
				startx = 0;
			}

			if (sx <= ex)
			{
				while (sy <= ey)
				{
					if (starty < heightshifted)
					{
						x = sx;
						cx = startx;
						cy = starty >> 16;
						dest = BITMAP_ADDR(bitmap, DATA_TYPE, sy, sx);

						pri = BITMAP_ADDR8(priority_bitmap, sy, sx);
						src = BITMAP_ADDR16(srcbitmap, cy, 0);
						pMask = BITMAP_ADDR8(transparency_bitmap, cy, 0);

						while (x <= ex && cx < srcbitmap->width)
						{
							if ( (pMask[cx]&mask) == value )
							{
								ROZ_PLOT_PIXEL((src[cx]+palette_offset)) ;
							}
							cx++;
							x++;
							dest++;
							pri++;
						}
					}
					starty += incyy;
					sy++;
				}
			}
		}
		else
		{
			while (startx >= widthshifted && sx <= ex)
			{
				startx += incxx;
				sx++;
			}

			if (sx <= ex)
			{
				while (sy <= ey)
				{
					if (starty < heightshifted)
					{
						x = sx;
						cx = startx;
						cy = starty >> 16;
						dest = BITMAP_ADDR(bitmap, DATA_TYPE, sy, sx);

						pri = BITMAP_ADDR8(priority_bitmap, sy, sx);
						src = BITMAP_ADDR16(srcbitmap, cy, 0);
						pMask = BITMAP_ADDR8(transparency_bitmap, cy, 0);

						while (x <= ex && cx < widthshifted)
						{
							if ( (pMask[cx>>16]&mask) == value )
							{
								ROZ_PLOT_PIXEL((src[cx >> 16]+palette_offset)) ;
							}
							cx += incxx;
							x++;
							dest++;
							pri++;
						}
					}
					starty += incyy;
					sy++;
				}
			}
		}
	}
	else
	{
		if (wraparound)
		{
			/* plot with wraparound */
			while (sy <= ey)
			{
				x = sx;
				cx = startx;
				cy = starty;
				dest = BITMAP_ADDR(bitmap, DATA_TYPE, sy, sx);
				pri = BITMAP_ADDR8(priority_bitmap, sy, sx);
				while (x <= ex)
				{
					if( (*BITMAP_ADDR8(transparency_bitmap, (cy>>16)&ymask, (cx>>16)&xmask) & mask) == value )
					{
						ROZ_PLOT_PIXEL((*BITMAP_ADDR16(srcbitmap, (cy >> 16) & ymask, (cx >> 16) & xmask) + palette_offset));
					}
					cx += incxx;
					cy += incxy;
					x++;
					dest++;
					pri++;
				}
				startx += incyx;
				starty += incyy;
				sy++;
			}
		}
		else
		{
			while (sy <= ey)
			{
				x = sx;
				cx = startx;
				cy = starty;
				dest = BITMAP_ADDR(bitmap, DATA_TYPE, sy, sx);
				pri = BITMAP_ADDR8(priority_bitmap, sy, sx);
				while (x <= ex)
				{
					if (cx < widthshifted && cy < heightshifted)
					{
						if( (*BITMAP_ADDR8(transparency_bitmap, cy>>16, cx>>16) & mask)==value )
						{
							ROZ_PLOT_PIXEL((*BITMAP_ADDR16(srcbitmap, cy >> 16, cx >> 16) + palette_offset));
						}
					}
					cx += incxx;
					cy += incxy;
					x++;
					dest++;
					pri++;
				}
				startx += incyx;
				starty += incyy;
				sy++;
			}
		}
	}
})

#ifndef osd_pend
#define osd_pend() do { } while (0)
#endif

DECLARE( draw, (tilemap *tmap, int xpos, int ypos, int mask, int value ),
{
	trans_t transPrev;
	trans_t transCur;
	const UINT8 *pTrans;
	UINT32 cached_indx;
	mame_bitmap *screen = blit.screen_bitmap;
	int tilemap_priority_code = blit.tilemap_priority_code;
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+tmap->cached_width;
	int y2 = ypos+tmap->cached_height;
	DATA_TYPE *dest_baseaddr = NULL;
	DATA_TYPE *dest_next;
	int dy;
	int count;
	const UINT16 *source0;
	DATA_TYPE *dest0;
	UINT8 *pmap0;
	int i;
	int row;
	int x_start;
	int x_end;
	int column;
	int c1; /* leftmost visible column in source tilemap */
	int c2; /* rightmost visible column in source tilemap */
	int y; /* current screen line to render */
	int y_next;
	UINT8 *priority_bitmap_baseaddr;
	UINT8 *priority_bitmap_next;
	const UINT16 *source_baseaddr;
	const UINT16 *source_next;
	const UINT8 *mask0;
	const UINT8 *mask_baseaddr;
	const UINT8 *mask_next;

	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 ) /* do nothing if totally clipped */
	{
		priority_bitmap_baseaddr = BITMAP_ADDR8(priority_bitmap, y1, xpos);
		if( screen )
		{
			dest_baseaddr = BITMAP_ADDR(screen, DATA_TYPE, y1, xpos);
		}

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = BITMAP_ADDR16(tmap->pixmap, y1, 0);
		mask_baseaddr = BITMAP_ADDR8(tmap->transparency_bitmap, y1, 0);

		c1 = x1/tmap->cached_tile_width; /* round down */
		c2 = (x2+tmap->cached_tile_width-1)/tmap->cached_tile_width; /* round up */

		y = y1;
		y_next = tmap->cached_tile_height*(y1/tmap->cached_tile_height) + tmap->cached_tile_height;
		if( y_next>y2 ) y_next = y2;

		dy = y_next-y;
		dest_next = dest_baseaddr + dy*blit.screen_bitmap_pitch_line;
		priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_pitch_line;
		source_next = source_baseaddr + dy*tmap->pixmap_pitch_line;
		mask_next = mask_baseaddr + dy*tmap->transparency_bitmap_pitch_line;
		for(;;)
		{
			row = y/tmap->cached_tile_height;
			x_start = x1;

			transPrev = eWHOLLY_TRANSPARENT;
			pTrans = mask_baseaddr + x_start;

			cached_indx = row*tmap->num_cached_cols + c1;
			for( column=c1; column<=c2; column++ )
			{
				if( column == c2 )
				{
					transCur = eWHOLLY_TRANSPARENT;
					goto L_Skip;
				}

				if( tmap->transparency_data[cached_indx]==TILE_FLAG_DIRTY )
				{
					update_tile_info( tmap, cached_indx, column, row );
				}

				if( (tmap->transparency_data[cached_indx]&mask)!=0 )
				{
					transCur = eMASKED;
				}
				else
				{
					transCur = (((*pTrans)&mask) == value)?eWHOLLY_OPAQUE:eWHOLLY_TRANSPARENT;
				}
				pTrans += tmap->cached_tile_width;

			L_Skip:
				if( transCur!=transPrev )
				{
					x_end = column*tmap->cached_tile_width;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( transPrev != eWHOLLY_TRANSPARENT )
					{
						count = x_end - x_start;
						source0 = source_baseaddr + x_start;
						dest0 = dest_baseaddr + x_start;
						pmap0 = priority_bitmap_baseaddr + x_start;

						if( transPrev == eWHOLLY_OPAQUE )
						{
							i = y;
							for(;;)
							{
								blit.draw_opaque( dest0, source0, count, pmap0, tilemap_priority_code );
								if( ++i == y_next ) break;

								dest0 += blit.screen_bitmap_pitch_line;
								source0 += tmap->pixmap_pitch_line;
								pmap0 += priority_bitmap_pitch_line;
							}
						} /* transPrev == eWHOLLY_OPAQUE */
						else /* transPrev == eMASKED */
						{
							mask0 = mask_baseaddr + x_start;
							i = y;
							for(;;)
							{
								blit.draw_masked( dest0, source0, mask0, mask, value, count, pmap0, tilemap_priority_code );
								if( ++i == y_next ) break;

								dest0 += blit.screen_bitmap_pitch_line;
								source0 += tmap->pixmap_pitch_line;
								mask0 += tmap->transparency_bitmap_pitch_line;
								pmap0 += priority_bitmap_pitch_line;
							}
						} /* transPrev == eMASKED */
					} /* transPrev != eWHOLLY_TRANSPARENT */
					x_start = x_end;
					transPrev = transCur;
				}
				cached_indx++;
			}
			if( y_next==y2 ) break; /* we are done! */

			priority_bitmap_baseaddr = priority_bitmap_next;
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
			mask_baseaddr = mask_next;
			y = y_next;
			y_next += tmap->cached_tile_height;

			if( y_next>=y2 )
			{
				y_next = y2;
			}
			else
			{
				dest_next += blit.screen_bitmap_pitch_row;
				priority_bitmap_next += priority_bitmap_pitch_row;
				source_next += tmap->pixmap_pitch_row;
				mask_next += tmap->transparency_bitmap_pitch_row;
			}
		} /* process next row */
	} /* not totally clipped */

	osd_pend();
})

#undef DATA_TYPE
#undef DEPTH
#undef DECLARE
#endif /* DECLARE */

#ifdef TRANSP
/*************************************************************************************************/

/* Each of the following routines draws pixmap and transarency data for a single tile.
 *
 * This function returns a per-tile code.  Each bit of this code is 0 if the corresponding
 * bit is zero in every byte of transparency data in the tile, or 1 if that bit is not
 * consistant within the tile.
 *
 * This precomputed value allows us for any particular tile and mask, to determine if all pixels
 * in that tile have the same masked transparency value.
 */

static UINT8 TRANSP(HandleTransparencyBitmask)(tilemap *tmap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tmap->cached_tile_width;
	UINT32 tile_height = tmap->cached_tile_height;
	mame_bitmap *pixmap = tmap->pixmap;
	mame_bitmap *transparency_bitmap = tmap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel;
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 code_opaque = code_transparent | TILE_FLAG_FG_OPAQUE;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT8 *pBitmask = tile_info.mask_data;
	UINT32 bitoffs;
	int bWhollyOpaque;
	int bWhollyTransparent;
	int bDontIgnoreTransparency = !(flags&TILE_IGNORE_TRANSPARENCY);

	bWhollyOpaque = 1;
	bWhollyTransparent = 1;

	pPenToPixel = tmap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
			}
			pPenData += pitch;
		}
	}

	pPenToPixel = tmap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	bitoffs = 0;
	for( ty=tile_height; ty!=0; ty-- )
	{
		for( tx=tile_width; tx!=0; tx-- )
		{
			yx = *pPenToPixel++;
			x = x0+(yx%MAX_TILESIZE);
			y = y0+(yx/MAX_TILESIZE);
			if( bDontIgnoreTransparency && (pBitmask[bitoffs/8]&(0x80>>(bitoffs&7))) == 0 )
			{
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code_transparent;
				bWhollyOpaque = 0;
			}
			else
			{
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
				bWhollyTransparent = 0;
			}
			bitoffs++;
		}
	}

	return (bWhollyOpaque || bWhollyTransparent)?0:TILE_FLAG_FG_OPAQUE;
}

static UINT8 TRANSP(HandleTransparencyColor)(tilemap *tmap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tmap->cached_tile_width;
	UINT32 tile_height = tmap->cached_tile_height;
	mame_bitmap *pixmap = tmap->pixmap;
	mame_bitmap *transparency_bitmap = tmap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tmap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 code_opaque = code_transparent | TILE_FLAG_FG_OPAQUE;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 transparent_color = tmap->transparent_pen;
	int bWhollyOpaque;
	int bWhollyTransparent;

	bWhollyOpaque = 1;
	bWhollyTransparent = 1;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				if( PAL_GET(pen)==transparent_color )
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
					bWhollyTransparent = 0;
				}

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				if( PAL_GET(pen)==transparent_color )
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
					bWhollyTransparent = 0;
				}
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				if( PAL_GET(pen)==transparent_color )
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
					bWhollyTransparent = 0;
				}
			}
			pPenData += pitch;
		}
	}
	return (bWhollyOpaque || bWhollyTransparent)?0:TILE_FLAG_FG_OPAQUE;
}

static UINT8 TRANSP(HandleTransparencyPen)(tilemap *tmap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tmap->cached_tile_width;
	UINT32 tile_height = tmap->cached_tile_height;
	mame_bitmap *pixmap = tmap->pixmap;
	mame_bitmap *transparency_bitmap = tmap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tmap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 code_opaque = code_transparent | TILE_FLAG_FG_OPAQUE;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 transparent_pen = tmap->transparent_pen;
	int bWhollyOpaque;
	int bWhollyTransparent;

	bWhollyOpaque = 1;
	bWhollyTransparent = 1;

	if( flags&TILE_IGNORE_TRANSPARENCY )
	{
		transparent_pen = ~0;
	}

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				if( pen==transparent_pen )
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
					bWhollyTransparent = 0;
				}

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				*BITMAP_ADDR8(transparency_bitmap, y, x) = (pen==transparent_pen)?code_transparent:code_opaque;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				if( pen==transparent_pen )
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_transparent;
					bWhollyOpaque = 0;

				}
				else
				{
					*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
					bWhollyTransparent = 0;
				}
			}
			pPenData += pitch;
		}
	}

	return (bWhollyOpaque || bWhollyTransparent)?0:TILE_FLAG_FG_OPAQUE;
}

static UINT8 TRANSP(HandleTransparencyPenBit)(tilemap *tmap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tmap->cached_tile_width;
	UINT32 tile_height = tmap->cached_tile_height;
	mame_bitmap *pixmap = tmap->pixmap;
	mame_bitmap *transparency_bitmap = tmap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tmap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 penbit = tmap->transparent_pen;
	UINT32 code_front = tile_info.priority | TILE_FLAG_FG_OPAQUE;
	UINT32 code_back = tile_info.priority | TILE_FLAG_BG_OPAQUE;
	int code;
	int and_flags = ~0;
	int or_flags = 0;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				code = ((pen&penbit)==penbit)?code_front:code_back;
				and_flags &= code;
				or_flags |= code;
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code;

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				code = ((pen&penbit)==penbit)?code_front:code_back;
				and_flags &= code;
				or_flags |= code;
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				code = ((pen&penbit)==penbit)?code_front:code_back;
				and_flags &= code;
				or_flags |= code;
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code;
			}
			pPenData += pitch;
		}
	}
	return or_flags ^ and_flags;
}

static UINT8 TRANSP(HandleTransparencyPens)(tilemap *tmap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tmap->cached_tile_width;
	UINT32 tile_height = tmap->cached_tile_height;
	mame_bitmap *pixmap = tmap->pixmap;
	mame_bitmap *transparency_bitmap = tmap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tmap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 fgmask = tmap->fgmask[(flags>>TILE_SPLIT_OFFSET)&3];
	UINT32 bgmask = tmap->bgmask[(flags>>TILE_SPLIT_OFFSET)&3];
	UINT32 code;
	int and_flags = ~0;
	int or_flags = 0;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				code = code_transparent;
				if( !((1<<pen)&fgmask) ) code |= TILE_FLAG_FG_OPAQUE;
				if( !((1<<pen)&bgmask) ) code |= TILE_FLAG_BG_OPAQUE;
				and_flags &= code;
				or_flags |= code;
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code;

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				code = code_transparent;
				if( !((1<<pen)&fgmask) ) code |= TILE_FLAG_FG_OPAQUE;
				if( !((1<<pen)&bgmask) ) code |= TILE_FLAG_BG_OPAQUE;
				and_flags &= code;
				or_flags |= code;
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				code = code_transparent;
				if( !((1<<pen)&fgmask) ) code |= TILE_FLAG_FG_OPAQUE;
				if( !((1<<pen)&bgmask) ) code |= TILE_FLAG_BG_OPAQUE;
				and_flags &= code;
				or_flags |= code;
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code;
			}
			pPenData += pitch;
		}
	}
	return and_flags ^ or_flags;
}

static UINT8 TRANSP(HandleTransparencyNone)(tilemap *tmap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tmap->cached_tile_width;
	UINT32 tile_height = tmap->cached_tile_height;
	mame_bitmap *pixmap = tmap->pixmap;
	mame_bitmap *transparency_bitmap = tmap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tmap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_opaque = tile_info.priority;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*BITMAP_ADDR16(pixmap, y, x) = PAL_GET(pen);
				*BITMAP_ADDR8(transparency_bitmap, y, x) = code_opaque;
			}
			pPenData += pitch;
		}
	}
	return 0;
}

#undef TRANSP
#undef PAL_INIT
#undef PAL_GET
#endif // TRANSP

