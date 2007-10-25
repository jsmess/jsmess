/***************************************************************************

    polynew.h

    New polygon helper routines.

****************************************************************************

    Pixel model:

    (0.0,0.0)       (1.0,0.0)       (2.0,0.0)       (3.0,0.0)
        +---------------+---------------+---------------+
        |               |               |               |
        |               |               |               |
        |   (0.5,0.5)   |   (1.5,0.5)   |   (2.5,0.5)   |
        |       *       |       *       |       *       |
        |               |               |               |
        |               |               |               |
    (0.0,1.0)       (1.0,1.0)       (2.0,1.0)       (3.0,1.0)
        +---------------+---------------+---------------+
        |               |               |               |
        |               |               |               |
        |   (0.5,1.5)   |   (1.5,1.5)   |   (2.5,1.5)   |
        |       *       |       *       |       *       |
        |               |               |               |
        |               |               |               |
        |               |               |               |
        +---------------+---------------+---------------+
    (0.0,2.0)       (1.0,2.0)       (2.0,2.0)       (3.0,2.0)

***************************************************************************/

#pragma once

#ifndef __POLYNEW_H__
#define __POLYNEW_H__

#include "mamecore.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_VERTEX_PARAMS		6

#define POLYFLAG_INCLUDE_BOTTOM_EDGE		0x01
#define POLYFLAG_INCLUDE_RIGHT_EDGE			0x02
#define POLYFLAG_NO_WORK_QUEUE				0x04



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* opaque reference to the poly manager */
typedef struct _poly_manager poly_manager;


/* scanline_extent describes start/end points for a scanline */
typedef struct _scanline_extent scanline_extent;
struct _scanline_extent
{
	INT16		startx;
	INT16		stopx;
};


/* input vertex data */
typedef struct _poly_vertex poly_vertex;
struct _poly_vertex
{
	float		x;							/* screen X coordinate */
	float		y;							/* screen Y coordinate */
	float		p[MAX_VERTEX_PARAMS];		/* interpolated parameter values */
};


/* single set of polygon per-parameter data */
typedef struct _poly_param poly_param;
struct _poly_param
{
	float		start;						/* parameter value at starting X,Y */
	float		dpdx;						/* dp/dx relative to starting X */
	float		dpdy;						/* dp/dy relative to starting Y */
};


/* polygon constant data */
typedef struct _poly_params poly_params;
struct _poly_params
{
	void *		extra;						/* pointer to the extra data */
	INT32		xorigin;					/* X origin for all parameters */
	INT32		yorigin;					/* Y origin for all parameters */
	poly_param	param[MAX_VERTEX_PARAMS];	/* array of parameter data */
};


/* callback routine to process a batch of scanlines */
typedef void (*poly_draw_scanline)(void *dest, INT32 scanline, INT32 startx, INT32 stopx, const poly_params *poly, int threadid);



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/


/* ----- initialization/teardown ----- */

/* allocate a new poly manager */
poly_manager *poly_alloc(int max_queued_polygons, size_t extra_data_size, UINT8 flags);

/* free a poly manager */
void poly_free(poly_manager *poly);



/* ----- synchronization ----- */

/* wait until all polygons in the queue have been rendered */
void poly_wait(poly_manager *poly, const char *debug_reason);



/* ----- core rendering ----- */

/* get a pointer to the extra data for the next polygon */
void *poly_get_extra_data(poly_manager *poly);

/* render a single triangle given 3 vertexes */
UINT32 poly_render_triangle(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_scanline callback, int paramcount, const poly_vertex *v1, const poly_vertex *v2, const poly_vertex *v3);

/* perform a custom render of an object, given specific extents */
UINT32 poly_render_custom(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_scanline callback, int startscanline, int numscanlines, const scanline_extent *extents);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    poly_param_value - return parameter value
    at specified X,Y coordinate
-------------------------------------------------*/

INLINE float poly_param_value(INT32 x, INT32 y, int whichparam, const poly_params *data)
{
	return data->param[whichparam].start + (float)(x - data->xorigin) * data->param[whichparam].dpdx + (float)(y - data->yorigin) * data->param[whichparam].dpdy;
}


#endif	/* __POLY_H__ */
