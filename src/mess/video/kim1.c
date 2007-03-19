/******************************************************************************
	KIM-1

	video driver

	Juergen Buchmueller, Jan 2000

******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/kim1.h"

PALETTE_INIT( kim1 )
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

	palette_set_color(machine, 16,   0,   0,   0);
	palette_set_color(machine, 17,  30,  30,  30);
	palette_set_color(machine, 18,  90,  90,  90);
	palette_set_color(machine, 19,  50,  50,  50);
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

VIDEO_START( kim1 )
{
    videoram_size = 6 * 2 + 24;
    videoram = auto_malloc (videoram_size);

	return video_start_generic(machine);
}

VIDEO_UPDATE( kim1 )
{
    int x, y;

	fillbitmap(bitmap, get_black_pen(machine), NULL);

    for (x = 0; x < 6; x++)
    {
        int sy = 408;
        int sx = machine->screen[0].width - 212 + x * 30 + ((x >= 4) ? 6 : 0);

		drawgfx (bitmap, machine->gfx[0], videoram[2 * x + 0], videoram[2 * x + 1],
			0, 0, sx, sy, &machine->screen[0].visarea, TRANSPARENCY_PEN, 0);
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
            int sx = machine->screen[0].width - 182 + x * 37;
            int color, code = layout[y][x];

            color = (readinputport (code / 7) & (0x40 >> (code % 7))) ? 0 : 1;

            videoram[6 * 2 + code] = color;
			drawgfx (bitmap, machine->gfx[1], layout[y][x], color,
				0, 0, sx, sy, &machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
        }
    }
	return 0;
}


