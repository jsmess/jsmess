/*************************************************************************

    Driver for Midway V-Unit games

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "audio/williams.h"
#include "video/poly.h"
#include "midvunit.h"


#define WATCH_RENDER		(0)
#define KEEP_STATISTICS		(0)
#define LOG_DMA				(0)


#define DMA_CLOCK			40000000


#if KEEP_STATISTICS
#define ADD_TO_PIXEL_COUNT(a)	do { if ((a) > 0) pixelcount += (a); } while (0)
#else
#define ADD_TO_PIXEL_COUNT(a)
#endif


/* for when we implement DMA timing */
#define DMA_QUEUE_SIZE		273
#define TIME_PER_PIXEL		41e-9



UINT16 *midvunit_videoram;
UINT32 *midvunit_textureram;

static UINT16 video_regs[16];
static UINT16 dma_data[16];
static UINT8 dma_data_index;
static UINT16 page_control;
static UINT8 video_changed;

static mame_timer *scanline_timer;

static struct poly_vertex vert[4];
static UINT8 topleft, topright, botleft, botright;

#if KEEP_STATISTICS
static int polycount, pixelcount, lastfps, framecount, totalframes;
#endif


/*************************************
 *
 *  Video system start
 *
 *************************************/

static TIMER_CALLBACK( scanline_timer_cb )
{
	int scanline = param;

	cpunum_set_input_line(0, 0, ASSERT_LINE);
	mame_timer_adjust(scanline_timer, video_screen_get_time_until_pos(0, scanline + 1, 0), scanline, time_zero);
}


VIDEO_START( midvunit )
{
	scanline_timer = mame_timer_alloc(scanline_timer_cb);
}



/*************************************
 *
 *  Returns true if the quad is
 *  straight-on
 *
 *************************************/

INLINE int quad_is_straight(void)
{
	INT32 x1, x2, y1, y2, t;

	x1 = vert[0].x;
	t = vert[1].x;

	/* case 1: the first two verts are the two extremes */
	if (t != x1)
	{
		x2 = t;
		t = vert[2].x;
		if (t != x1)
		{
			if (t != x2 || vert[3].x != x1)
				return 0;
		}
		else
		{
			if (vert[3].x != x2)
				return 0;
		}
	}

	/* case 2: the first two verts are the same */
	else
	{
		x2 = vert[2].x;
		if (vert[3].x != x2)
			return 0;
	}

	y1 = vert[0].y;
	t = vert[1].y;

	/* case 1: the first two verts are the two extremes */
	if (t != y1)
	{
		y2 = t;
		t = vert[2].y;
		if (t != y1)
		{
			if (t != y2 || vert[3].y != y1)
				return 0;
		}
		else
		{
			if (vert[3].y != y2)
				return 0;
		}
	}

	/* case 2: the first two verts are the same */
	else
	{
		y2 = vert[2].y;
		if (vert[3].y != y2)
			return 0;
	}

	/* sort into min/max */
	if (x1 > x2) { t = x1; x1 = x2; x2 = t; }
	if (y1 > y2) { t = y1; y1 = y2; y2 = t; }

	/* determine the corners */
	for (t = 0; t < 4; t++)
	{
		if (vert[t].x == x1)
		{
			if (vert[t].y == y1)
				topleft = t;
			else
				botleft = t;
		}
		else
		{
			if (vert[t].y == y1)
				topright = t;
			else
				botright = t;
		}
	}

	return 1;
}



/*************************************
 *
 *  Straight, flat quad renderers
 *
 *************************************/

static void render_straight_flat_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT16 pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
	INT32 sx, sy, ex, ey, x, y;

	/* compute parameters */
	sx = vert[topleft].x;
	ex = vert[topright].x;
	sy = vert[topleft].y;
	ey = vert[botleft].y;

	/* clip */
	if (sx < machine->screen[0].visarea.min_x)
		sx = machine->screen[0].visarea.min_x;
	if (ex > machine->screen[0].visarea.max_x)
		ex = machine->screen[0].visarea.max_x;
	if (sy < machine->screen[0].visarea.min_y)
		sy = machine->screen[0].visarea.min_y;
	if (ey > machine->screen[0].visarea.max_y)
		ey = machine->screen[0].visarea.max_y;

	ADD_TO_PIXEL_COUNT((ey - sy + 1) * (ex - sx + 1));

	/* loop over rows */
	for (y = sy; y <= ey; y++)
	{
		UINT16 *d = dest + y * 512 + sx;
		if (pixdata)
			for (x = sx; x <= ex; x++)
				*d++ = pixdata;
		else
			memset(d, 0, 2 * (ex - sx + 1));
	}
}


static void render_straight_flat_dither_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT16 pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
	INT32 sx, sy, ex, ey, x, y;

	/* compute parameters */
	sx = vert[topleft].x;
	ex = vert[topright].x;
	sy = vert[topleft].y;
	ey = vert[botleft].y;

	/* clip */
	if (sx < machine->screen[0].visarea.min_x)
		sx = machine->screen[0].visarea.min_x;
	if (ex > machine->screen[0].visarea.max_x)
		ex = machine->screen[0].visarea.max_x;
	if (sy < machine->screen[0].visarea.min_y)
		sy = machine->screen[0].visarea.min_y;
	if (ey > machine->screen[0].visarea.max_y)
		ey = machine->screen[0].visarea.max_y;

	ADD_TO_PIXEL_COUNT((ey - sy + 1) * (ex - sx + 1));

	/* loop over rows */
	for (y = sy; y <= ey; y++)
	{
		UINT16 *d = dest + y * 512;
		int tsx = sx + ((sx ^ y) & 1);
		for (x = tsx; x <= ex; x += 2)
			d[x] = pixdata;
	}
}



/*************************************
 *
 *  Straight, textured quad renderers
 *
 *************************************/

static void render_straight_tex_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1];
	INT32 sx, sy, ex, ey, su, sv, dudx, dvdx, dudy, dvdy, x, y, u, v;

	/* compute parameters */
	sx = vert[topleft].x;
	ex = vert[topright].x;
	sy = vert[topleft].y;
	ey = vert[botleft].y;
	su = (dma_data[10 + topleft] & 0x00ff) << 16;
	sv = (dma_data[10 + topleft] & 0xff00) << 8;

	/* compute texture deltas */
	if (ex != sx)
	{
		dudx = (((dma_data[10 + topright] & 0x00ff) << 16) - su) / (ex - sx);
		dvdx = (((dma_data[10 + topright] & 0xff00) << 8 ) - sv) / (ex - sx);
	}
	else
		dudx = dvdx = 1;
	if (ey != sy)
	{
		dudy = (((dma_data[10 + botleft] & 0x00ff) << 16) - su) / (ey - sy);
		dvdy = (((dma_data[10 + botleft] & 0xff00) << 8 ) - sv) / (ey - sy);
	}
	else
		dudy = dvdy = 1;

	/* clip */
	if (sx < machine->screen[0].visarea.min_x)
	{
		su += (machine->screen[0].visarea.min_x - sx) * dudx;
		sv += (machine->screen[0].visarea.min_x - sx) * dvdx;
		sx = machine->screen[0].visarea.min_x;
	}
	if (ex > machine->screen[0].visarea.max_x)
		ex = machine->screen[0].visarea.max_x;
	if (sy < machine->screen[0].visarea.min_y)
	{
		su += (machine->screen[0].visarea.min_y - sy) * dudy;
		sv += (machine->screen[0].visarea.min_y - sy) * dvdy;
		sy = machine->screen[0].visarea.min_y;
	}
	if (ey > machine->screen[0].visarea.max_y)
		ey = machine->screen[0].visarea.max_y;

	ADD_TO_PIXEL_COUNT((ey - sy + 1) * (ex - sx + 1));

	/* loop over rows */
	for (y = sy; y <= ey; y++)
	{
		UINT16 *d = dest + y * 512;

		u = su;
		v = sv;
		su += dudy;
		sv += dvdy;

		for (x = sx; x <= ex; x++)
		{
			d[x] = pixdata | texbase[((v >> 8) & 0xff00) + (u >> 16)];
			u += dudx;
			v += dvdx;
		}
	}
}


static void render_straight_textrans_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1];
	INT32 sx, sy, ex, ey, su, sv, dudx, dvdx, dudy, dvdy, x, y, u, v;

	/* compute parameters */
	sx = vert[topleft].x;
	ex = vert[topright].x;
	sy = vert[topleft].y;
	ey = vert[botleft].y;
	su = (dma_data[10 + topleft] & 0x00ff) << 16;
	sv = (dma_data[10 + topleft] & 0xff00) << 8;

	/* compute texture deltas */
	if (ex != sx)
	{
		dudx = (((dma_data[10 + topright] & 0x00ff) << 16) - su) / (ex - sx);
		dvdx = (((dma_data[10 + topright] & 0xff00) << 8 ) - sv) / (ex - sx);
	}
	else
		dudx = dvdx = 1;
	if (ey != sy)
	{
		dudy = (((dma_data[10 + botleft] & 0x00ff) << 16) - su) / (ey - sy);
		dvdy = (((dma_data[10 + botleft] & 0xff00) << 8 ) - sv) / (ey - sy);
	}
	else
		dudy = dvdy = 1;

	/* clip */
	if (sx < machine->screen[0].visarea.min_x)
	{
		su += (machine->screen[0].visarea.min_x - sx) * dudx;
		sv += (machine->screen[0].visarea.min_x - sx) * dvdx;
		sx = machine->screen[0].visarea.min_x;
	}
	if (ex > machine->screen[0].visarea.max_x)
		ex = machine->screen[0].visarea.max_x;
	if (sy < machine->screen[0].visarea.min_y)
	{
		su += (machine->screen[0].visarea.min_y - sy) * dudy;
		sv += (machine->screen[0].visarea.min_y - sy) * dvdy;
		sy = machine->screen[0].visarea.min_y;
	}
	if (ey > machine->screen[0].visarea.max_y)
		ey = machine->screen[0].visarea.max_y;

	ADD_TO_PIXEL_COUNT((ey - sy + 1) * (ex - sx + 1));

	/* loop over rows */
	for (y = sy; y <= ey; y++)
	{
		UINT16 *d = dest + y * 512;

		u = su;
		v = sv;
		su += dudy;
		sv += dvdy;

		for (x = sx; x <= ex; x++)
		{
			int pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
			if (pix) d[x] = pixdata | pix;
			u += dudx;
			v += dvdx;
		}
	}
}


static void render_straight_textransmask_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
	INT32 sx, sy, ex, ey, su, sv, dudx, dvdx, dudy, dvdy, x, y, u, v;

	/* compute parameters */
	sx = vert[topleft].x;
	ex = vert[topright].x;
	sy = vert[topleft].y;
	ey = vert[botleft].y;
	su = (dma_data[10 + topleft] & 0x00ff) << 16;
	sv = (dma_data[10 + topleft] & 0xff00) << 8;

	/* compute texture deltas */
	if (ex != sx)
	{
		dudx = (((dma_data[10 + topright] & 0x00ff) << 16) - su) / (ex - sx);
		dvdx = (((dma_data[10 + topright] & 0xff00) << 8 ) - sv) / (ex - sx);
	}
	else
		dudx = dvdx = 1;
	if (ey != sy)
	{
		dudy = (((dma_data[10 + botleft] & 0x00ff) << 16) - su) / (ey - sy);
		dvdy = (((dma_data[10 + botleft] & 0xff00) << 8 ) - sv) / (ey - sy);
	}
	else
		dudy = dvdy = 1;

	/* clip */
	if (sx < machine->screen[0].visarea.min_x)
	{
		su += (machine->screen[0].visarea.min_x - sx) * dudx;
		sv += (machine->screen[0].visarea.min_x - sx) * dvdx;
		sx = machine->screen[0].visarea.min_x;
	}
	if (ex > machine->screen[0].visarea.max_x)
		ex = machine->screen[0].visarea.max_x;
	if (sy < machine->screen[0].visarea.min_y)
	{
		su += (machine->screen[0].visarea.min_y - sy) * dudy;
		sv += (machine->screen[0].visarea.min_y - sy) * dvdy;
		sy = machine->screen[0].visarea.min_y;
	}
	if (ey > machine->screen[0].visarea.max_y)
		ey = machine->screen[0].visarea.max_y;

	ADD_TO_PIXEL_COUNT((ey - sy + 1) * (ex - sx + 1));

	/* loop over rows */
	for (y = sy; y <= ey; y++)
	{
		UINT16 *d = dest + y * 512;

		u = su;
		v = sv;
		su += dudy;
		sv += dvdy;

		for (x = sx; x <= ex; x++)
		{
			int pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
			if (pix) d[x] = pixdata;
			u += dudx;
			v += dvdx;
		}
	}
}



/*************************************
 *
 *  Generic flat quad renderer
 *
 *************************************/

static void render_flat_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT16 pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_0(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_0(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			UINT16 *d = dest + y * 512 + curscan->sx;
			int width = curscan->ex - curscan->sx + 1;
			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			for (x = 0; x < width; x++)
				d[x] = pixdata;
		}
	}
}


static void render_flat_dither_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT16 pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_0(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_0(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			int tsx = curscan->sx + ((curscan->sx ^ y) & 1);
			UINT16 *d = dest + y * 512;
			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			for (x = tsx; x <= curscan->ex; x += 2)
				d[x] = pixdata;
		}
	}
}



/*************************************
 *
 *  Generic textured quad renderers
 *
 *************************************/

static void render_tex_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1];
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* fill in the vertex data */
	vert[0].p[0] = dma_data[10] & 0xff;
	vert[0].p[1] = dma_data[10] >> 8;
	vert[1].p[0] = dma_data[11] & 0xff;
	vert[1].p[1] = dma_data[11] >> 8;
	vert[2].p[0] = dma_data[12] & 0xff;
	vert[2].p[1] = dma_data[12] >> 8;
	vert[3].p[0] = dma_data[13] & 0xff;
	vert[3].p[1] = dma_data[13] >> 8;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_2(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			UINT16 *d = dest + y * 512 + curscan->sx;
			int width = curscan->ex - curscan->sx + 1;
			int u = curscan->p[0], dudx = scans->dp[0];
			int v = curscan->p[1], dvdx = scans->dp[1];
			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			for (x = 0; x < width; x++)
			{
				d[x] = pixdata | texbase[((v >> 8) & 0xff00) + (u >> 16)];
				u += dudx;
				v += dvdx;
			}
		}
	}
}


static void render_textrans_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1];
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* fill in the vertex data */
	vert[0].p[0] = dma_data[10] & 0xff;
	vert[0].p[1] = dma_data[10] >> 8;
	vert[1].p[0] = dma_data[11] & 0xff;
	vert[1].p[1] = dma_data[11] >> 8;
	vert[2].p[0] = dma_data[12] & 0xff;
	vert[2].p[1] = dma_data[12] >> 8;
	vert[3].p[0] = dma_data[13] & 0xff;
	vert[3].p[1] = dma_data[13] >> 8;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_2(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			UINT16 *d = dest + y * 512 + curscan->sx;
			int width = curscan->ex - curscan->sx + 1;
			int u = curscan->p[0], dudx = scans->dp[0];
			int v = curscan->p[1], dvdx = scans->dp[1];
			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			for (x = 0; x < width; x++)
			{
				int pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
				if (pix) d[x] = pixdata | pix;
				u += dudx;
				v += dvdx;
			}
		}
	}
}


static void render_textransmask_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* fill in the vertex data */
	vert[0].p[0] = dma_data[10] & 0xff;
	vert[0].p[1] = dma_data[10] >> 8;
	vert[1].p[0] = dma_data[11] & 0xff;
	vert[1].p[1] = dma_data[11] >> 8;
	vert[2].p[0] = dma_data[12] & 0xff;
	vert[2].p[1] = dma_data[12] >> 8;
	vert[3].p[0] = dma_data[13] & 0xff;
	vert[3].p[1] = dma_data[13] >> 8;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_2(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			UINT16 *d = dest + y * 512 + curscan->sx;
			int width = curscan->ex - curscan->sx + 1;
			int u = curscan->p[0], dudx = scans->dp[0];
			int v = curscan->p[1], dvdx = scans->dp[1];
			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			for (x = 0; x < width; x++)
			{
				int pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
				if (pix) d[x] = pixdata;
				u += dudx;
				v += dvdx;
			}
		}
	}
}



/*************************************
 *
 *  Generic dithered textured quad renderers
 *
 *************************************/

static void render_tex_dither_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1];
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* fill in the vertex data */
	vert[0].p[0] = dma_data[10] & 0xff;
	vert[0].p[1] = dma_data[10] >> 8;
	vert[1].p[0] = dma_data[11] & 0xff;
	vert[1].p[1] = dma_data[11] >> 8;
	vert[2].p[0] = dma_data[12] & 0xff;
	vert[2].p[1] = dma_data[12] >> 8;
	vert[3].p[0] = dma_data[13] & 0xff;
	vert[3].p[1] = dma_data[13] >> 8;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_2(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			int u = curscan->p[0], dudx = scans->dp[0];
			int v = curscan->p[1], dvdx = scans->dp[1];
			UINT16 *d = dest + y * 512;
			int tsx = curscan->sx;

			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			if ((tsx ^ y) & 1)
			{
				tsx++;
				u += dudx;
				v += dvdx;
			}

			dudx *= 2;
			dvdx *= 2;

			for (x = tsx; x <= curscan->ex; x += 2)
			{
				d[x] = pixdata | texbase[((v >> 8) & 0xff00) + (u >> 16)];
				u += dudx;
				v += dvdx;
			}
		}
	}
}


static void render_textrans_dither_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1];
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* fill in the vertex data */
	vert[0].p[0] = dma_data[10] & 0xff;
	vert[0].p[1] = dma_data[10] >> 8;
	vert[1].p[0] = dma_data[11] & 0xff;
	vert[1].p[1] = dma_data[11] >> 8;
	vert[2].p[0] = dma_data[12] & 0xff;
	vert[2].p[1] = dma_data[12] >> 8;
	vert[3].p[0] = dma_data[13] & 0xff;
	vert[3].p[1] = dma_data[13] >> 8;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_2(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			int u = curscan->p[0], dudx = scans->dp[0];
			int v = curscan->p[1], dvdx = scans->dp[1];
			UINT16 *d = dest + y * 512;
			int tsx = curscan->sx;

			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			if ((tsx ^ y) & 1)
			{
				tsx++;
				u += dudx;
				v += dvdx;
			}

			dudx *= 2;
			dvdx *= 2;

			for (x = tsx; x <= curscan->ex; x += 2)
			{
				int pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
				if (pix) d[x] = pixdata | pix;
				u += dudx;
				v += dvdx;
			}
		}
	}
}


static void render_textransmask_dither_quad(running_machine *machine)
{
	UINT16 *dest = &midvunit_videoram[(page_control & 4) ? 0x40000 : 0x00000];
	UINT8 *texbase = (UINT8 *)midvunit_textureram + (dma_data[14] * 256);
	UINT16 pixdata = dma_data[1] | (dma_data[0] & 0x00ff);
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	int x, y, i;

	/* fill in the vertex data */
	vert[0].p[0] = dma_data[10] & 0xff;
	vert[0].p[1] = dma_data[10] >> 8;
	vert[1].p[0] = dma_data[11] & 0xff;
	vert[1].p[1] = dma_data[11] >> 8;
	vert[2].p[0] = dma_data[12] & 0xff;
	vert[2].p[1] = dma_data[12] >> 8;
	vert[3].p[0] = dma_data[13] & 0xff;
	vert[3].p[1] = dma_data[13] >> 8;

	/* loop over two tris */
	for (i = 0; i < 2; i++)
	{
		/* first tri is 0,1,2; second is 0,3,2 */
		if (i == 0)
			scans = setup_triangle_2(&vert[0], &vert[1], &vert[2], &machine->screen[0].visarea);
		else
			scans = setup_triangle_2(&vert[0], &vert[3], &vert[2], &machine->screen[0].visarea);

		/* skip if we're clipped out */
		if (!scans)
			continue;

		/* loop over scanlines */
		curscan = scans->scanline;
		for (y = scans->sy; y <= scans->ey; y++, curscan++)
		{
			int u = curscan->p[0], dudx = scans->dp[0];
			int v = curscan->p[1], dvdx = scans->dp[1];
			UINT16 *d = dest + y * 512;
			int tsx = curscan->sx;

			ADD_TO_PIXEL_COUNT(curscan->ex - curscan->sx + 1);
			if ((tsx ^ y) & 1)
			{
				tsx++;
				u += dudx;
				v += dvdx;
			}

			dudx *= 2;
			dvdx *= 2;

			for (x = tsx; x <= curscan->ex; x += 2)
			{
				int pix = texbase[((v >> 8) & 0xff00) + (u >> 16)];
				if (pix) d[x] = pixdata;
				u += dudx;
				v += dvdx;
			}
		}
	}
}



/*************************************
 *
 *  DMA queue processor
 *
 *************************************/

static void process_dma_queue(running_machine *machine)
{
	int textured = ((dma_data[0] & 0x300) == 0x100);
	int dithered = dma_data[0] & 0x2000;
	int straight;

#if KEEP_STATISTICS
	polycount++;
#endif

	/* if we're rendering to the same page we're viewing, it has changed */
	if ((((page_control >> 2) ^ page_control) & 1) == 0)
		video_changed = TRUE;

	/* fill in the vertex data */
	vert[0].x = (INT16)dma_data[2];
	vert[0].y = (INT16)dma_data[3];
	vert[1].x = (INT16)dma_data[4];
	vert[1].y = (INT16)dma_data[5];
	vert[2].x = (INT16)dma_data[6];
	vert[2].y = (INT16)dma_data[7];
	vert[3].x = (INT16)dma_data[8];
	vert[3].y = (INT16)dma_data[9];

	/* determine if it's straight-on or arbitrary */
	straight = quad_is_straight();

	/* handle flat-shaded quads here */
	if (!textured)
	{
		/* two cases: straight on and arbitrary */
		if (straight)
		{
			if (!dithered)
				render_straight_flat_quad(machine);
			else
				render_straight_flat_dither_quad(machine);
		}
		else
		{
			if (!dithered)
				render_flat_quad(machine);
			else
				render_flat_dither_quad(machine);
		}
	}

	/* handle textured quads here */
	else
	{
		/* three cases: straight on, non-dithered arbitrary, and dithered arbitrary */
		if (straight && !dithered)
		{
			/* handle non-masked, non-transparent quads */
			if ((dma_data[0] & 0xc00) == 0x000)
				render_straight_tex_quad(machine);

			/* handle non-masked, transparent quads */
			else if ((dma_data[0] & 0xc00) == 0x800)
				render_straight_textrans_quad(machine);

			/* handle masked, transparent quads */
			else if ((dma_data[0] & 0xc00) == 0xc00)
				render_straight_textransmask_quad(machine);

			/* handle masked, non-transparent quads */
			else
				render_straight_flat_quad(machine);
		}
		else if (!dithered)
		{
			/* handle non-masked, non-transparent quads */
			if ((dma_data[0] & 0xc00) == 0x000)
				render_tex_quad(machine);

			/* handle non-masked, transparent quads */
			else if ((dma_data[0] & 0xc00) == 0x800)
				render_textrans_quad(machine);

			/* handle masked, transparent quads */
			else if ((dma_data[0] & 0xc00) == 0xc00)
				render_textransmask_quad(machine);

			/* handle masked, non-transparent quads */
			else
				render_flat_quad(machine);
		}
		else
		{
			/* handle non-masked, non-transparent quads */
			if ((dma_data[0] & 0xc00) == 0x000)
				render_tex_dither_quad(machine);

			/* handle non-masked, transparent quads */
			else if ((dma_data[0] & 0xc00) == 0x800)
				render_textrans_dither_quad(machine);

			/* handle masked, transparent quads */
			else if ((dma_data[0] & 0xc00) == 0xc00)
				render_textransmask_dither_quad(machine);

			/* handle masked, non-transparent quads */
			else
				render_flat_dither_quad(machine);
		}
	}
}



/*************************************
 *
 *  DMA pipe control control
 *
 *************************************/

WRITE32_HANDLER( midvunit_dma_queue_w )
{
	if (LOG_DMA && input_code_pressed(KEYCODE_L))
		logerror("%06X:queue(%X) = %08X\n", activecpu_get_pc(), dma_data_index, data);
	if (dma_data_index < 16)
		dma_data[dma_data_index++] = data;
}


READ32_HANDLER( midvunit_dma_queue_entries_r )
{
	/* always return 0 entries */
	return 0;
}


READ32_HANDLER( midvunit_dma_trigger_r )
{
	if (offset)
	{
		if (LOG_DMA && input_code_pressed(KEYCODE_L))
			logerror("%06X:trigger\n", activecpu_get_pc());
		process_dma_queue(Machine);
		dma_data_index = 0;
	}
	return 0;
}



/*************************************
 *
 *  Paging control
 *
 *************************************/

WRITE32_HANDLER( midvunit_page_control_w )
{
	/* watch for the display page to change */
	if ((page_control ^ data) & 1)
	{
		video_changed = TRUE;
		if (LOG_DMA && input_code_pressed(KEYCODE_L))
			logerror("##########################################################\n");
#if KEEP_STATISTICS
		popmessage("Polys:%d  Render:%d%%  FPS:%d",
				polycount, pixelcount / (512*4), lastfps);
		polycount = pixelcount = 0;
		framecount++;
#endif
		video_screen_update_partial(0, video_screen_get_vpos(0) - 1);
	}
	page_control = data;
}


READ32_HANDLER( midvunit_page_control_r )
{
	return page_control;
}



/*************************************
 *
 *  Video control
 *
 *************************************/

WRITE32_HANDLER( midvunit_video_control_w )
{
	UINT16 old = video_regs[offset];

	/* update the data */
	COMBINE_DATA(&video_regs[offset]);

	/* update the scanline timer */
	if (offset == 0)
		mame_timer_adjust(scanline_timer, video_screen_get_time_until_pos(0, (data & 0x1ff) + 1, 0), data & 0x1ff, time_zero);

	/* if something changed, update our parameters */
	if (old != video_regs[offset] && video_regs[6] != 0 && video_regs[11] != 0)
	{
		rectangle visarea;

		/* derive visible area from blanking */
		visarea.min_x = 0;
		visarea.max_x = (video_regs[6] + video_regs[2] - video_regs[5]) % video_regs[6];
		visarea.min_y = 0;
		visarea.max_y = (video_regs[11] + video_regs[7] - video_regs[10]) % video_regs[11];
		video_screen_configure(0, video_regs[6], video_regs[11], &visarea, HZ_TO_SUBSECONDS(MIDVUNIT_VIDEO_CLOCK / 2) * video_regs[6] * video_regs[11]);
	}
}


READ32_HANDLER( midvunit_scanline_r )
{
	return video_screen_get_vpos(0);
}



/*************************************
 *
 *  Video RAM access
 *
 *************************************/

WRITE32_HANDLER( midvunit_videoram_w )
{
	if (!video_changed)
	{
		int visbase = (page_control & 1) ? 0x40000 : 0x00000;
		if ((offset & 0x40000) == visbase)
			video_changed = TRUE;
	}
	COMBINE_DATA(&midvunit_videoram[offset]);
}


READ32_HANDLER( midvunit_videoram_r )
{
	return midvunit_videoram[offset];
}



/*************************************
 *
 *  Palette RAM access
 *
 *************************************/

WRITE32_HANDLER( midvunit_paletteram_w )
{
	int newword;

	COMBINE_DATA(&paletteram32[offset]);
	newword = paletteram32[offset];
	palette_set_color_rgb(Machine, offset, pal5bit(newword >> 10), pal5bit(newword >> 5), pal5bit(newword >> 0));
}



/*************************************
 *
 *  Texture RAM access
 *
 *************************************/

WRITE32_HANDLER( midvunit_textureram_w )
{
	UINT8 *base = (UINT8 *)midvunit_textureram;
	base[offset * 2] = data;
	base[offset * 2 + 1] = data >> 8;
}


READ32_HANDLER( midvunit_textureram_r )
{
	UINT8 *base = (UINT8 *)midvunit_textureram;
	return (base[offset * 2 + 1] << 8) | base[offset * 2];
}




/*************************************
 *
 *  Video system update
 *
 *************************************/

VIDEO_UPDATE( midvunit )
{
	int x, y, width, xoffs;
	UINT32 offset;

	/* if the video didn't change, indicate as much */
	if (!video_changed)
		return UPDATE_HAS_NOT_CHANGED;
	video_changed = FALSE;

#if KEEP_STATISTICS
	totalframes++;
	if (totalframes == 57)
	{
		lastfps = framecount;
		framecount = totalframes = 0;
	}
#endif

	/* determine the base of the videoram */
#if WATCH_RENDER
	offset = (page_control & 4) ? 0x40000 : 0x00000;
#else
	offset = (page_control & 1) ? 0x40000 : 0x00000;
#endif

	/* determine how many pixels to copy */
	xoffs = cliprect->min_x;
	width = cliprect->max_x - xoffs + 1;

	/* adjust the offset */
	offset += xoffs;
	offset += 512 * (cliprect->min_y - machine->screen[0].visarea.min_y);

	/* loop over rows */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels + cliprect->min_x;
		for (x = 0; x < width; x++)
			*dest++ = midvunit_videoram[offset + x] & 0x7fff;
		offset += 512;
	}
	return 0;
}
