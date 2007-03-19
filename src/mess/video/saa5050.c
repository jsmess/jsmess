/***************************************************************************

  saa5050.c

  Functions to emulate the video hardware of the saa5050.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/saa5050.h"

static INT8 frame_count;

#define SAA5050_DBLHI	0x0001
#define SAA5050_SEPGR	0x0002
#define SAA5050_FLASH	0x0004
#define SAA5050_BOX		0x0008
#define SAA5050_GRAPH	0x0010
#define SAA5050_CONCEAL	0x0020
#define SAA5050_HOLDGR	0x0040

#define SAA5050_BLACK   0
#define SAA5050_WHITE   7

struct	{
	UINT16	saa5050_flags;
	UINT8	saa5050_forecol;
	UINT8	saa5050_backcol;
	UINT8	saa5050_prvcol;
	UINT8	saa5050_prvchr;
} saa5050_state;

static gfx_layout saa5050_charlayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8,
	  5*8, 6*8, 7*8, 8*8, 9*8 },
	8 * 10
};

static gfx_layout saa5050_hilayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8, 2*8,
	  2*8, 3*8, 3*8, 4*8, 4*8 },
	8 * 10
};

static gfx_layout saa5050_lolayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 5*8, 5*8, 6*8, 6*8, 7*8,
	  7*8, 8*8, 8*8, 9*8, 9*8 },
	8 * 10
};

static gfx_decode saa5050_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &saa5050_charlayout, 0, 128},
	{ REGION_GFX1, 0x0000, &saa5050_hilayout, 0, 128},
	{ REGION_GFX1, 0x0000, &saa5050_lolayout, 0, 128},
	{-1}
};

static unsigned char saa5050_palette[8 * 3] =
{
	0x00, 0x00, 0x00,	/* black */
	0xff, 0x00, 0x00,	/* red */
	0x00, 0xff, 0x00,	/* green */
	0xff, 0xff, 0x00,	/* yellow */
	0x00, 0x00, 0xff,	/* blue */
	0xff, 0x00, 0xff,	/* magenta */
	0x00, 0xff, 0xff,	/* cyan */
	0xff, 0xff, 0xff	/* white */
};

static	unsigned	short	saa5050_colortable[64 * 2] =	/* bgnd, fgnd */
{
	0,1, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0, 2,1, 2,2, 2,3, 2,4, 2,5, 2,6, 2,7,
	3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
	4,0, 4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7,
	5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
	6,0, 6,1, 6,2, 6,3, 6,4, 6,5, 6,6, 6,7,
	7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7
};

static PALETTE_INIT( saa5050 )
{
	palette_set_colors(machine, 0, saa5050_palette, sizeof(saa5050_palette) / 3);
	memcpy(colortable, saa5050_colortable, sizeof (saa5050_colortable));
}

static VIDEO_START( saa5050 )
{
	frame_count = 0;
	if( video_start_generic(machine) )
		return 1;
    return 0;
}

static void saa5050_vh_callback (void)
{
	if (frame_count++ > 49)
		frame_count = 0;
}

/* || */

/* BOX and dirtybuffer not implemented */

static VIDEO_UPDATE( saa5050 )
{
	int code, colour;
	int sx, sy;

	for (sy = 0; sy < 24; sy++)
	{
		/* Set start of line state */
		saa5050_state.saa5050_flags = 0;
		saa5050_state.saa5050_prvchr = 32;
		saa5050_state.saa5050_forecol = SAA5050_WHITE;
		saa5050_state.saa5050_prvcol = SAA5050_WHITE;

		for (sx = 0; sx < 40; sx++)
		{
			code = videoram[sy * 80 + sx];
			if (code < 32)
			{
				switch (code) {
				case 0x01: case 0x02: case 0x03: case 0x04:
				case 0x05: case 0x06: case 0x07:
					saa5050_state.saa5050_prvcol = saa5050_state.saa5050_forecol = code;
					saa5050_state.saa5050_flags &= ~(SAA5050_GRAPH | SAA5050_CONCEAL);
					break;
				case 0x11: case 0x12: case 0x13: case 0x14:
				case 0x15: case 0x16: case 0x17:
					saa5050_state.saa5050_prvcol = (saa5050_state.saa5050_forecol =
						(code & 0x07));
					saa5050_state.saa5050_flags &= ~SAA5050_CONCEAL;
					saa5050_state.saa5050_flags |= SAA5050_GRAPH;
					break;
				case 0x08:
					saa5050_state.saa5050_flags |= SAA5050_FLASH;
					break;
				case 0x09:
					saa5050_state.saa5050_flags &= ~SAA5050_FLASH;
					break;
				case 0x0a:
					saa5050_state.saa5050_flags |= SAA5050_BOX;
					break;
				case 0x0b:
					saa5050_state.saa5050_flags &= ~SAA5050_BOX;
					break;
				case 0x0c:
					saa5050_state.saa5050_flags &= ~SAA5050_DBLHI;
					break;
				case 0x0d:
					saa5050_state.saa5050_flags |= SAA5050_DBLHI;
					break;
				case 0x18:
					saa5050_state.saa5050_flags |= SAA5050_CONCEAL;
					break;
				case 0x19:
					saa5050_state.saa5050_flags |= SAA5050_SEPGR;
					break;
				case 0x1a:
					saa5050_state.saa5050_flags &= ~SAA5050_SEPGR;
					break;
				case 0x1c:
					saa5050_state.saa5050_backcol = SAA5050_BLACK;
					break;
				case 0x1d:
					saa5050_state.saa5050_backcol = saa5050_state.saa5050_prvcol;
					break;
				case 0x1e:
					saa5050_state.saa5050_flags |= SAA5050_HOLDGR;
					break;
				case 0x1f:
					saa5050_state.saa5050_flags &= ~SAA5050_HOLDGR;
					break;
				}
				if (saa5050_state.saa5050_flags & SAA5050_HOLDGR)
	  				code = saa5050_state.saa5050_prvchr;
				else
					code = 32;
			}
			if (saa5050_state.saa5050_flags & SAA5050_CONCEAL)
				code = 32;
			else if ((saa5050_state.saa5050_flags & SAA5050_FLASH) && (frame_count > 38))
				code = 32;
			else
			{
				saa5050_state.saa5050_prvchr = code;
				if ((saa5050_state.saa5050_flags & SAA5050_GRAPH) && (code & 0x20))
				{
					code += (code & 0x40) ? 64 : 96;
					if (saa5050_state.saa5050_flags & SAA5050_SEPGR)
						code += 64;
				}
			}
			if (code & 0x80)
				colour = (saa5050_state.saa5050_forecol << 3) | saa5050_state.saa5050_backcol;
			else
				colour = saa5050_state.saa5050_forecol | (saa5050_state.saa5050_backcol << 3);
			if (saa5050_state.saa5050_flags & SAA5050_DBLHI)
			{
				drawgfx (bitmap, Machine->gfx[1], code, colour, 0, 0,
					sx * 6, sy * 10, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
				drawgfx (bitmap, Machine->gfx[2], code, colour, 0, 0,
					sx * 6, (sy + 1) * 10, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
			}
			else
			{
				drawgfx (bitmap, Machine->gfx[0], code, colour, 0, 0,
					sx * 6, sy * 10, &Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
			}
		}
		if (saa5050_state.saa5050_flags & SAA5050_DBLHI)
		{
			sy++;
			saa5050_state.saa5050_flags &= ~SAA5050_DBLHI;
		}
	}
	return 0;
}

MACHINE_DRIVER_START( vh_saa5050 )
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(SAA5050_VBLANK))
	MDRV_INTERLEAVE(1)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40 * 6, 24 * 10)
	MDRV_SCREEN_VISIBLE_AREA(0, 40 * 6 - 1, 0, 24 * 10 - 1)
	MDRV_GFXDECODE( saa5050_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(128)
	MDRV_PALETTE_INIT(saa5050)

	MDRV_VIDEO_START(saa5050)
	MDRV_VIDEO_UPDATE(saa5050)
MACHINE_DRIVER_END

