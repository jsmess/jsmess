#include "driver.h"
#include "video/generic.h"

#include "includes/pocketc.h"

/* PC126x
   24x2 5x7 space between char
   2000 .. 203b, 2800 .. 283b
   2040 .. 207b, 2840 .. 287b
  203d: 0 BUSY, 1 PRINT, 3 JAPAN, 4 SMALL, 5 SHIFT, 6 DEF
  207c: 1 DEF 1 RAD 2 GRAD 5 ERROR 6 FLAG */

static unsigned char pocketc_palette[] =
{
	99,107,99,
	94,111,103,
	255,255,255,
	255,255,255,
	60, 66, 60,
	0, 0, 0
};

unsigned short pocketc_colortable[8][2] = {
	{ 0, 4 },
	{ 0, 4 },
	{ 0, 4 },
	{ 0, 4 },
	{ 1, 5 },
	{ 1, 5 },
	{ 1, 5 },
	{ 1, 5 }
};

PALETTE_INIT( pocketc )
{
	palette_set_colors(machine, 0, pocketc_palette, sizeof(pocketc_palette) / 3);
	memcpy(colortable,pocketc_colortable,sizeof(pocketc_colortable));
}

VIDEO_START( pocketc )
{
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)auto_malloc (videoram_size);

#if 0
	{
		char backdrop_name[200];
	    /* try to load a backdrop for the machine */
		sprintf(backdrop_name, "%s.png", Machine->gamedrv->name);
		backdrop_load(backdrop_name, 8);
	}
#endif

	return video_start_generic(machine);
}

void pocketc_draw_special(mame_bitmap *bitmap,
						  int x, int y, const POCKETC_FIGURE fig, int color)
{
	int i,j;
	for (i=0;fig[i];i++,y++) {
		for (j=0;fig[i][j]!=0;j++) {
			switch(fig[i][j]) {
			case '1':
				plot_pixel(bitmap, x+j, y, color);
				break;
			case 'e': return;
			}
		}
	}
}

