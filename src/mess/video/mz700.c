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
#include "machine/pit8253.h"
#include "includes/mz700.h"


#ifndef VERBOSE
#define VERBOSE 1
#endif

#define LOG(N,M,A)	\
	do { \
		if(VERBOSE>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",attotime_to_double(timer_get_time()),(char*)M ); \
			logerror A; \
		} \
	} while (0)


PALETTE_INIT( mz700 )
{
	int i;

	machine->colortable = colortable_alloc(machine, 8);

	for (i = 0; i < 8; i++)
		colortable_palette_set_color(machine->colortable, i, MAKE_RGB((i & 2) ? 0xff : 0x00, (i & 4) ? 0xff : 0x00, (i & 1) ? 0xff : 0x00));
	
	for (i = 0; i < 256; i++)
	{
		colortable_entry_set_value(machine->colortable, i*2, i & 7);
        	colortable_entry_set_value(machine->colortable, i*2+1, (i >> 4) & 7);
	}
}


VIDEO_UPDATE( mz700 )
{
	int offs;

	fillbitmap(bitmap, get_black_pen(screen->machine), cliprect);

	for(offs = 0; offs < 40*25; offs++)
	{
		int sx, sy, code, color;
	        sy = (offs / 40) * 8;
		sx = (offs % 40) * 8;
		color = colorram[offs];
		code = videoram[offs] | (color & 0x80) << 1;
		drawgfx(bitmap, screen->machine->gfx[0], code, color, 0, 0, sx, sy, cliprect, TRANSPARENCY_NONE, 0);
	}

	return 0;
}
