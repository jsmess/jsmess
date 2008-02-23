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

#include "driver.h"
#include "includes/mz700.h"


#ifndef VERBOSE
#define VERBOSE 1
#endif

#define LOG(N,M,A)	\
	if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",attotime_to_double(timer_get_time()),(char*)M ); logerror A; }


PALETTE_INIT( mz700 )
{
	int i;

    for (i = 0; i < 8; i++)
	{
		palette_set_color_rgb(machine, i, (i & 2) ? 0xff : 0x00, (i & 4) ? 0xff : 0x00, (i & 1) ? 0xff : 0x00);
	}
}


VIDEO_UPDATE( mz700 )
{
    int offs;

    fillbitmap(bitmap, get_black_pen(machine), cliprect);

	for(offs = 0; offs < 40*25; offs++)
	{
		int sx, sy, code, color;

        sy = (offs / 40) * 8;
		sx = (offs % 40) * 8;
		code = videoram[offs];
		color = colorram[offs];
		code |= (color & 0x80) << 1;

        drawgfx(bitmap, machine->gfx[0], code, color, 0, 0, sx, sy,
			cliprect, TRANSPARENCY_NONE, 0);
	}

	return 0;
}
