/******************************************************************************
	Motorola Evaluation Kit 6800 D2
    MEK6800D2

    video driver

	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

******************************************************************************/

#include "driver.h"
#include "video/generic.h"

#include "includes/mekd2.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	logerror(x)
#else
#define LOG(x)	/* x */
#endif

PALETTE_INIT( mekd2 )
{
	int i;

	/* initialize 16 colors with shades of red (orange) */
	for (i = 0; i < 16; i++)
	{
		palette_set_color(machine, i,
			24 + (i + 1) * (i + 1) - 1,
			(i + 1) * (i + 1) / 4,
			0);

		colortable[2 * i + 0] = 1;
		colortable[2 * i + 1] = i;
	}

	palette_set_color(machine, 16, 0, 0, 0);
	palette_set_color(machine, 17, 30, 30, 30);
	palette_set_color(machine, 18, 90, 90, 90);
	palette_set_color(machine, 19, 50, 50, 50);
	palette_set_color(machine, 20, 255, 255, 255);

	colortable[2 * 16 + 0 * 4 + 0] = 17;
	colortable[2 * 16 + 0 * 4 + 1] = 18;
	colortable[2 * 16 + 0 * 4 + 2] = 19;
	colortable[2 * 16 + 0 * 4 + 3] = 20;

	colortable[2 * 16 + 1 * 4 + 0] = 17;
	colortable[2 * 16 + 1 * 4 + 1] = 17;
	colortable[2 * 16 + 1 * 4 + 2] = 19;
	colortable[2 * 16 + 1 * 4 + 3] = 15;
}

VIDEO_START( mekd2 )
{
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)auto_malloc (videoram_size);

#if 0
	{
		char backdrop_name[200];
	    /* try to load a backdrop for the machine */
		sprintf(backdrop_name, "%s.png", Machine->gamedrv->name);
		backdrop_load(backdrop_name, 2);
	}
#endif

	return video_start_generic(machine);
}

VIDEO_UPDATE( mekd2 )
{
    int x, y;

    for (x = 0; x < 6; x++)
    {
        int sy = 408;
        int sx = Machine->screen[0].width - 212 + x * 30 + ((x >= 4) ? 6 : 0);

        drawgfx (bitmap, Machine->gfx[0],
                 videoram[2 * x + 0], videoram[2 * x + 1],
                 0, 0, sx, sy, NULL, TRANSPARENCY_PEN, 0);
    }

    for (y = 0; y < 6; y++)
    {
        int sy = 516 + y * 36;

        for (x = 0; x < 4; x++)
        {
            static int layout[6][4] =
            {
                {22, 19, 21, 23},
                {16, 17, 20, 18},
                {12, 13, 14, 15},
				{ 8,  9, 10, 11},
				{ 4,  5,  6,  7},
				{ 0,  1,  2,  3}
            };
            int sx = Machine->screen[0].width - 182 + x * 37;
            int color, code = layout[y][x];

            color = (readinputport (code / 7) & (0x40 >> (code % 7))) ? 0 : 1;

            videoram[6 * 2 + code] = color;
            drawgfx (bitmap, Machine->gfx[1],
                     layout[y][x], color,
                     0, 0, sx, sy, NULL,
                     TRANSPARENCY_NONE, 0);
        }
    }

	return 0;
}


