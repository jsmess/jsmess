/***************************************************************************

    polynew.c

    Helper routines for polygon rendering.

***************************************************************************/

#include "polynew.h"
#include "eminline.h"
#include "mame.h"
#include <math.h>


/***************************************************************************
    DEBUGGING
***************************************************************************/

/* keep statistics */
#define KEEP_STATISTICS					0

/* turn this on to log the reasons for any long waits */
#define LOG_WAITS						0

/* number of profiling ticks before we consider a wait "long" */
#define LOG_WAIT_THRESHOLD				1000



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define SCANLINES_PER_BUCKET			8
#define CACHE_LINE_SIZE					64			/* this is a general guess */
#define TOTAL_BUCKETS					(512 / SCANLINES_PER_BUCKET)
#define UNITS_PER_POLY					(100 / SCANLINES_PER_BUCKET)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* forward definitions */
typedef struct _polygon_info polygon_info;


/* work_unit describes a single "unit" of work, which is a small number of scanlines */
typedef struct _work_unit work_unit;
struct _work_unit
{
	polygon_info *		polygon;				/* pointer to polygon */
	volatile UINT32		count_next;				/* number of scanlines and index of next item to process */
	INT16				scanline;				/* starting scanline and count */
	UINT16				previtem;				/* index of previous item in the same bucket */
	scanline_extent 	extent[SCANLINES_PER_BUCKET]; /* array of scanline extents */
};


/* polygon_info describes a single polygon, which includes the poly_params */
struct _polygon_info
{
	poly_manager *		poly;					/* pointer back to the poly manager */
	void *				dest;					/* pointer to the destination we are rendering to */
	poly_draw_scanline 	callback;				/* callback to handle a scanline's worth of work */
	poly_params			params;					/* poly_params structure */
};


/* full poly manager description */
struct _poly_manager
{
	/* queue management */
	osd_work_queue *	queue;					/* work queue */

	/* work units */
	work_unit **		unit;					/* array of work unit pointers */
	UINT32				unit_next;				/* index of next unit to allocate */
	UINT32				unit_count;				/* number of work units available */
	size_t				unit_size;				/* size of each work unit, in bytes */

	/* poly data */
	polygon_info **		polygon;				/* array of polygon pointers */
	UINT32				polygon_next;			/* index of next polygon to allocate */
	UINT32				polygon_count;			/* number of polygon items available */
	size_t				polygon_size;			/* size of each polygon, in bytes */

	/* misc data */
	UINT8				flags;					/* flags */

	/* buckets */
	UINT16				unit_bucket[TOTAL_BUCKETS]; /* buckets for tracking unit usage */

	/* statistics */
	UINT32				polygons;				/* number of polygons queued */
	UINT64				pixels;					/* number of pixels rendered */
#if KEEP_STATISTICS
	UINT32				conflicts[WORK_MAX_THREADS]; /* number of conflicts found, per thread */
	UINT32				resolved[WORK_MAX_THREADS];	/* number of conflicts resolved, per thread */
#endif
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void *poly_item_callback(void *param, int threadid);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    round_coordinate - round a coordinate to
    an integer, following rules that 0.5 rounds
    down
-------------------------------------------------*/

INLINE INT32 round_coordinate(float value)
{
	INT32 result = floor(value);
	return result + (value - (float)result > 0.5f);
}



/***************************************************************************
    INITIALIZATION/TEARDOWN
***************************************************************************/

/*-------------------------------------------------
    poly_alloc - initialize a new polygon
    manager
-------------------------------------------------*/

poly_manager *poly_alloc(int max_queued_polygons, size_t extra_data_size, UINT8 flags)
{
	int polynum, unitnum;
	poly_manager *poly;

	/* allocate the manager itself */
	poly = malloc_or_die(sizeof(*poly));
	memset(poly, 0, sizeof(*poly));
	poly->flags = flags;

	/* allocate polygons */
	poly->polygon_size = sizeof(polygon_info) + extra_data_size;
	poly->polygon_count = MAX(max_queued_polygons, 1);
	poly->polygon = malloc_or_die(sizeof(*poly->polygon) * poly->polygon_count);
	memset(poly->polygon, 0, sizeof(*poly->polygon) * poly->polygon_count);
	poly->polygon[0] = malloc_or_die(sizeof(*poly->polygon[0]) * poly->polygon_count * poly->polygon_size);
	memset(poly->polygon[0], 0, sizeof(*poly->polygon[0]) * poly->polygon_count * poly->polygon_size);

	/* initialize the pointers */
	for (polynum = 0; polynum < poly->polygon_count; polynum++)
	{
		poly->polygon[polynum] = (void *)((UINT8 *)poly->polygon[0] + polynum * poly->polygon_size);
		poly->polygon[polynum]->params.extra = (void *)((UINT8 *)poly->polygon[polynum] + sizeof(polygon_info));
	}

	/* allocate work units */
	poly->unit_size = ((sizeof(work_unit) + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE) * CACHE_LINE_SIZE;
	poly->unit_count = MIN(poly->polygon_count * UNITS_PER_POLY, 65535);
	poly->unit = malloc_or_die(sizeof(*poly->unit) * poly->unit_count);
	memset(poly->unit, 0, sizeof(*poly->unit) * poly->unit_count);
	poly->unit[0] = malloc_or_die(sizeof(*poly->unit[0]) * poly->unit_count * poly->unit_size);
	memset(poly->unit[0], 0, sizeof(*poly->unit[0]) * poly->unit_count * poly->unit_size);

	/* initialize the work units */
	for (unitnum = 0; unitnum < poly->unit_count; unitnum++)
		poly->unit[unitnum] = (void *)((UINT8 *)poly->unit[0] + unitnum * poly->unit_size);

	/* create the work queue */
	if (!(flags & POLYFLAG_NO_WORK_QUEUE))
		poly->queue = osd_work_queue_alloc(WORK_QUEUE_FLAG_MULTI | WORK_QUEUE_FLAG_HIGH_FREQ);
	return poly;
}


/*-------------------------------------------------
    poly_free - free a polygon manager
-------------------------------------------------*/

void poly_free(poly_manager *poly)
{
#if KEEP_STATISTICS
{
	int i, conflicts = 0, resolved = 0;
	for (i = 0; i < ARRAY_LENGTH(poly->conflicts); i++)
	{
		conflicts += poly->conflicts[i];
		resolved += poly->resolved[i];
	}
	printf("Total polygons = %d\n", poly->polygons);
	if (poly->pixels > 1000000000)
		printf("Total pixels   = %d%09d\n", (UINT32)(poly->pixels / 1000000000), (UINT32)(poly->pixels % 1000000000));
	else
		printf("Total pixels   = %d\n", (UINT32)poly->pixels);
	printf("Resolved conflicts = %d/%d\n", resolved, conflicts);
}
#endif

	/* free the work queue */
	if (poly->queue != NULL)
		osd_work_queue_free(poly->queue);

	/* free the work units */
	if (poly->unit != NULL)
	{
		if (poly->unit[0] != NULL)
			free(poly->unit[0]);
		free(poly->unit);
	}

	/* free the polygon */
	if (poly->polygon != NULL)
	{
		if (poly->polygon[0] != NULL)
			free(poly->polygon[0]);
		free(poly->polygon);
	}

	/* free the manager itself */
	free(poly);
}



/***************************************************************************
    SYNCHRONIZATION
***************************************************************************/

/*-------------------------------------------------
    poly_wait - wait for all pending rendering
    to complete
-------------------------------------------------*/

void poly_wait(poly_manager *poly, const char *debug_reason)
{
	osd_ticks_t time;

	/* remember the start time if we're logging */
	if (LOG_WAITS)
		time = osd_profiling_ticks();

	/* wait for all pending work items to complete */
	if (poly->queue != NULL)
		osd_work_queue_wait(poly->queue, osd_ticks_per_second() * 100);

	/* if we don't have a queue, just run the whole list now */
	else
	{
		int unitnum;
		for (unitnum = 0; unitnum < poly->unit_next; unitnum++)
			poly_item_callback(poly->unit[unitnum], 0);
	}

	/* log any long waits */
	if (LOG_WAITS)
	{
		time = osd_profiling_ticks() - time;
		if (time > LOG_WAIT_THRESHOLD)
			logerror("Poly:Waited %d cycles for %s\n", (int)time, debug_reason);
	}

	/* reset the state */
	poly->polygon_next = poly->unit_next = 0;
	memset(poly->unit_bucket, 0xff, sizeof(poly->unit_bucket));
}



/***************************************************************************
    CORE RENDERING
***************************************************************************/

/*-------------------------------------------------
    poly_get_extra_data - get a pointer to the
    extra data for the next polygon
-------------------------------------------------*/

void *poly_get_extra_data(poly_manager *poly)
{
	/* wait for a work item if we have to */
	if (poly->polygon_next + 1 > poly->polygon_count)
		poly_wait(poly, "Out of polygons");

	/* return a pointer to the extra data for the next item */
	return poly->polygon[poly->polygon_next]->params.extra;
}



/***************************************************************************
    CORE RENDERING
***************************************************************************/

/*-------------------------------------------------
    poly_render_triangle - render a single
    triangle given 3 vertexes
-------------------------------------------------*/

UINT32 poly_render_triangle(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_scanline callback, int paramcount, const poly_vertex *v1, const poly_vertex *v2, const poly_vertex *v3)
{
	float dxdy_minmid, dxdy_minmax, dxdy_midmax;
	const poly_vertex *tv;
	INT32 curscan, scaninc;
	polygon_info *polygon;
	INT32 v1yclip, v3yclip;
	INT32 v1y, v3y, v1x;
	INT32 pixels = 0;
	UINT32 startunit;

	/* first sort by Y */
	if (v2->y < v1->y)
	{
		tv = v1;
		v1 = v2;
		v2 = tv;
	}
	if (v3->y < v2->y)
	{
		tv = v2;
		v2 = v3;
		v3 = tv;
		if (v2->y < v1->y)
		{
			tv = v1;
			v1 = v2;
			v2 = tv;
		}
	}

	/* compute some integral X/Y vertex values */
	v1x = round_coordinate(v1->x);
	v1y = round_coordinate(v1->y);
	v3y = round_coordinate(v3->y);

	/* clip coordinates */
	v1yclip = v1y;
	v3yclip = v3y + ((poly->flags & POLYFLAG_INCLUDE_BOTTOM_EDGE) ? 1 : 0);
	if (cliprect != NULL)
	{
		v1yclip = MAX(v1yclip, cliprect->min_y);
		v3yclip = MIN(v3yclip, cliprect->max_y + 1);
	}
	if (v3yclip - v1yclip <= 0)
		return 0;

	/* wait for a work item if we have to */
	if (poly->polygon_next + 1 > poly->polygon_count)
		poly_wait(poly, "Out of polygons");
	else if (poly->unit_next + (v3yclip - v1yclip) / SCANLINES_PER_BUCKET + 2 > poly->unit_count)
		poly_wait(poly, "Out of work units");
	polygon = poly->polygon[poly->polygon_next++];

	/* fill in the polygon information */
	polygon->poly = poly;
	polygon->dest = dest;
	polygon->callback = callback;

	/* set the start X/Y coordinates */
	polygon->params.xorigin = v1x;
	polygon->params.yorigin = v1y;

	/* compute the slopes for each portion of the triangle */
	dxdy_minmid = (v2->y == v1->y) ? 0.0f : (v2->x - v1->x) / (v2->y - v1->y);
	dxdy_minmax = (v3->y == v1->y) ? 0.0f : (v3->x - v1->x) / (v3->y - v1->y);
	dxdy_midmax = (v3->y == v2->y) ? 0.0f : (v3->x - v2->x) / (v3->y - v2->y);

	/* compute the X extents for each scanline */
	startunit = poly->unit_next;
	for (curscan = v1yclip; curscan < v3yclip; curscan += scaninc)
	{
		UINT32 bucketnum = ((UINT32)curscan / SCANLINES_PER_BUCKET) % TOTAL_BUCKETS;
		UINT32 unit_index = poly->unit_next++;
		work_unit *unit = poly->unit[unit_index];
		int extnum;

		/* determine how much to advance to hit the next bucket */
		scaninc = SCANLINES_PER_BUCKET - (UINT32)curscan % SCANLINES_PER_BUCKET;

		/* fill in the work unit basics */
		unit->polygon = polygon;
		unit->count_next = MIN(v3yclip - curscan, scaninc);
		unit->scanline = curscan;
		unit->previtem = poly->unit_bucket[bucketnum];
		poly->unit_bucket[bucketnum] = unit_index;

		/* iterate over extents */
		for (extnum = 0; extnum < unit->count_next; extnum++)
		{
			float fully = (float)(curscan + extnum) + 0.5f;
			float startx = v1->x + (fully - v1->y) * dxdy_minmax;
			float stopx;
			INT32 istartx, istopx;

			/* compute the ending X based on which part of the triangle we're in */
			if (fully < v2->y)
				stopx = v1->x + (fully - v1->y) * dxdy_minmid;
			else
				stopx = v2->x + (fully - v2->y) * dxdy_midmax;

			/* clamp to full pixels */
			istartx = round_coordinate(startx);
			istopx = round_coordinate(stopx);

			/* force start < stop */
			if (istartx > istopx)
			{
				INT32 temp = istartx;
				istartx = istopx;
				istopx = temp;
			}

			/* include the right edge if requested */
			if (poly->flags & POLYFLAG_INCLUDE_RIGHT_EDGE)
				istopx++;

			/* apply left/right clipping */
			if (cliprect != NULL)
			{
				if (istartx < cliprect->min_x)
					istartx = cliprect->min_x;
				if (istopx > cliprect->max_x)
					istopx = cliprect->max_x + 1;
			}

			/* set the extent and update the total pixel count */
			if (istartx >= istopx)
				istartx = istopx = 0;
			unit->extent[extnum].startx = istartx;
			unit->extent[extnum].stopx = istopx;
			pixels += istopx - istartx;
		}
	}

	/* compute parameter starting points and deltas */
	if (paramcount > 0)
	{
		float divisor, dx1, dx2, dy1, dy2, xoffset, yoffset;
		int paramnum;

		/* compute the divisor */
		divisor = 1.0f / ((v1->x - v2->x) * (v1->y - v3->y) - (v1->x - v3->x) * (v1->y - v2->y));

		/* compute the dx/dy values */
		dx1 = v1->y - v3->y;
		dx2 = v1->y - v2->y;
		dy1 = v1->x - v2->x;
		dy2 = v1->x - v3->x;

		/* determine x/y offset for subpixel correction so that the starting values are
           relative to the integer coordinates */
		xoffset = v1->x - ((float)polygon->params.xorigin + 0.5f);
		yoffset = v1->y - ((float)polygon->params.yorigin + 0.5f);

		/* iterate over parameters */
		for (paramnum = 0; paramnum < paramcount; paramnum++)
		{
			poly_param *params = &polygon->params.param[paramnum];
			params->dpdx = ((v1->p[paramnum] - v2->p[paramnum]) * dx1 - (v1->p[paramnum] - v3->p[paramnum]) * dx2) * divisor;
			params->dpdy = ((v1->p[paramnum] - v3->p[paramnum]) * dy1 - (v1->p[paramnum] - v2->p[paramnum]) * dy2) * divisor;
			params->start = v1->p[paramnum] - xoffset * params->dpdx - yoffset * params->dpdy;
		}
	}

	/* enqueue the work items */
	if (poly->queue != NULL)
		osd_work_item_queue_multiple(poly->queue, poly_item_callback, poly->unit_next - startunit, poly->unit[startunit], poly->unit_size, WORK_ITEM_FLAG_AUTO_RELEASE);

	/* return the total number of pixels in the triangle */
	poly->polygons++;
	poly->pixels += pixels;
	return pixels;
}


/*-------------------------------------------------
    poly_render_custom - perform a custom render
    of an object, given specific extents
-------------------------------------------------*/

UINT32 poly_render_custom(poly_manager *poly, void *dest, const rectangle *cliprect, poly_draw_scanline callback, int startscanline, int numscanlines, const scanline_extent *extents)
{
	INT32 curscan, scaninc;
	polygon_info *polygon;
	INT32 v1yclip, v3yclip;
	INT32 pixels = 0;
	UINT32 startunit;

	/* clip coordinates */
	if (cliprect != NULL)
	{
		v1yclip = MAX(startscanline, cliprect->min_y);
		v3yclip = MIN(startscanline + numscanlines, cliprect->max_y + 1);
	}
	else
	{
		v1yclip = startscanline;
		v3yclip = startscanline + numscanlines;
	}
	if (v3yclip - v1yclip <= 0)
		return 0;

	/* wait for a work item if we have to */
	if (poly->polygon_next + 1 > poly->polygon_count)
		poly_wait(poly, "Out of polygons");
	else if (poly->unit_next + (v3yclip - v1yclip) / SCANLINES_PER_BUCKET + 2 > poly->unit_count)
		poly_wait(poly, "Out of work units");
	polygon = poly->polygon[poly->polygon_next++];

	/* fill in the polygon information */
	polygon->poly = poly;
	polygon->dest = dest;
	polygon->callback = callback;

	/* compute the X extents for each scanline */
	startunit = poly->unit_next;
	for (curscan = v1yclip; curscan < v3yclip; curscan += scaninc)
	{
		UINT32 bucketnum = ((UINT32)curscan / SCANLINES_PER_BUCKET) % TOTAL_BUCKETS;
		UINT32 unit_index = poly->unit_next++;
		work_unit *unit = poly->unit[unit_index];
		int extnum;

		/* determine how much to advance to hit the next bucket */
		scaninc = SCANLINES_PER_BUCKET - (UINT32)curscan % SCANLINES_PER_BUCKET;

		/* fill in the work unit basics */
		unit->polygon = polygon;
		unit->count_next = MIN(v3yclip - curscan, scaninc);
		unit->scanline = curscan;
		unit->previtem = poly->unit_bucket[bucketnum];
		poly->unit_bucket[bucketnum] = unit_index;

		/* iterate over extents */
		for (extnum = 0; extnum < unit->count_next; extnum++)
		{
			const scanline_extent *extent = &extents[(curscan + extnum) - startscanline];
			INT32 istartx = extent->startx, istopx = extent->stopx;

			/* force start < stop */
			if (istartx > istopx)
			{
				INT32 temp = istartx;
				istartx = istopx;
				istopx = temp;
			}

			/* apply left/right clipping */
			if (cliprect != NULL)
			{
				if (istartx < cliprect->min_x)
					istartx = cliprect->min_x;
				if (istopx > cliprect->max_x)
					istopx = cliprect->max_x + 1;
			}

			/* set the extent and update the total pixel count */
			unit->extent[extnum].startx = istartx;
			unit->extent[extnum].stopx = istopx;
			if (istartx < istopx)
				pixels += istopx - istartx;
		}
	}

	/* enqueue the work items */
	if (poly->queue != NULL)
		osd_work_item_queue_multiple(poly->queue, poly_item_callback, poly->unit_next - startunit, poly->unit[startunit], poly->unit_size, WORK_ITEM_FLAG_AUTO_RELEASE);

	/* return the total number of pixels in the object */
	poly->polygons++;
	poly->pixels += pixels;
	return pixels;
}



/***************************************************************************
    WORK ITEM CALLBACK
***************************************************************************/

/*-------------------------------------------------
    poly_item_callback - callback for each poly
    item
-------------------------------------------------*/

static void *poly_item_callback(void *param, int threadid)
{
	while (1)
	{
		work_unit *unit = param;
		polygon_info *polygon = unit->polygon;
		int count = unit->count_next & 0xffff;
		UINT32 orig_count_next;
		int extnum;

		/* if our previous item isn't done yet, enqueue this item to the end and proceed */
		if (unit->previtem != 0xffff)
		{
			work_unit *prevunit = polygon->poly->unit[unit->previtem];
			if (prevunit->count_next != 0)
			{
				UINT32 unitnum = ((UINT8 *)unit - (UINT8 *)polygon->poly->unit[0]) / polygon->poly->unit_size;
				UINT32 new_count_next;

				/* attempt to atomically swap in this new value */
				do
				{
					orig_count_next = prevunit->count_next;
					new_count_next = orig_count_next | (unitnum << 16);
				} while (compare_exchange32((volatile INT32 *)&prevunit->count_next, orig_count_next, new_count_next) != orig_count_next);

#if KEEP_STATISTICS
				/* track resolved conflicts */
				polygon->poly->conflicts[threadid]++;
				if (orig_count_next != 0)
					polygon->poly->resolved[threadid]++;
#endif
				/* if we succeeded, skip out early so we can do other work */
				if (orig_count_next != 0)
					break;
			}
		}

		/* iterate over extents */
		for (extnum = 0; extnum < count; extnum++)
			(*polygon->callback)(polygon->dest, unit->scanline + extnum, unit->extent[extnum].startx, unit->extent[extnum].stopx, &polygon->params, threadid);

		/* set our count to 0 and re-fetch the original count value */
		do
		{
			orig_count_next = unit->count_next;
		} while (compare_exchange32((volatile INT32 *)&unit->count_next, orig_count_next, 0) != orig_count_next);

		/* if we have no more work to do, do nothing */
		orig_count_next >>= 16;
		if (orig_count_next == 0)
			break;
		param = polygon->poly->unit[orig_count_next];
	}
	return NULL;
}
