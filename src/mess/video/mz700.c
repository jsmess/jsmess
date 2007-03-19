/***************************************************************************
 *	Sharp MZ700
 *
 *	video hardware
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *	Reference: http://sharpmz.computingmuseum.com
 *
 ***************************************************************************/

#include "includes/mz700.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(N,M,A)	\
	if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define LOG(N,M,A)
#endif

int mz700_frame_time = 0;

//void mz700_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
PALETTE_INIT(mz700)
{
	int i;

    for (i = 0; i < 8; i++)
	{
		palette_set_color(machine, i, (i & 2) ? 0xff : 0x00, (i & 4) ? 0xff : 0x00, (i & 1) ? 0xff : 0x00);
	}

	for (i = 0; i < 256; i++)
	{
		colortable[i*2+0] = i & 7;
        colortable[i*2+1] = (i >> 4) & 7;
	}
}

VIDEO_START(mz700)
{
	if (video_start_generic(machine))
		return 1;
    return 0;
}

//void mz700_vh_screenrefresh(mame_bitmap *bitmap, int full_refresh)
VIDEO_UPDATE(mz700)
{
    int offs;
    int full_refresh = 1;

    if( full_refresh )
	{
		fillbitmap(bitmap, Machine->pens[0], &Machine->screen[0].visarea);
		memset(dirtybuffer, 1, videoram_size);
    }

	for( offs = 0; offs < 40*25; offs++ )
	{
		if( dirtybuffer[offs] )
		{
			int sx, sy, code, color;

            dirtybuffer[offs] = 0;

            sy = (offs / 40) * 8;
			sx = (offs % 40) * 8;
			code = videoram[offs];
			color = colorram[offs];
			code |= (color & 0x80) << 1;

            drawgfx(bitmap,Machine->gfx[0],code,color,0,0,sx,sy,
				&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);
		}
	}
	return 0;
}


