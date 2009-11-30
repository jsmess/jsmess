/******************************************************************************
    Motorola Evaluation Kit 6800 D2
    MEK6800D2

    video driver

    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

******************************************************************************/

#include "driver.h"
#include "includes/mekd2.h"

PALETTE_INIT( mekd2 )
{
	int i;

	machine->colortable = colortable_alloc(machine, 21);

	for (i = 0; i < 16; i++)
		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(24 + (i + 1) * (i + 1) - 1, (i + 1) * (i + 1) / 4, 0));

	colortable_palette_set_color(machine->colortable, 16, MAKE_RGB(0, 0, 0));
	colortable_palette_set_color(machine->colortable, 17, MAKE_RGB(30, 30, 30));
	colortable_palette_set_color(machine->colortable, 18, MAKE_RGB(90, 90, 90));
	colortable_palette_set_color(machine->colortable, 19, MAKE_RGB(50, 50, 50));
	colortable_palette_set_color(machine->colortable, 20, MAKE_RGB(255, 255, 255));

	for (i = 0; i < 16; i++)
	{
		colortable_entry_set_value(machine->colortable, i*2, 1);
		colortable_entry_set_value(machine->colortable, i*2+1, i);
	}

	colortable_entry_set_value(machine->colortable, 32, 17);
	colortable_entry_set_value(machine->colortable, 33, 18);
	colortable_entry_set_value(machine->colortable, 34, 19);
	colortable_entry_set_value(machine->colortable, 35, 20);
	colortable_entry_set_value(machine->colortable, 36, 17);
	colortable_entry_set_value(machine->colortable, 37, 17);
	colortable_entry_set_value(machine->colortable, 38, 19);
	colortable_entry_set_value(machine->colortable, 39, 15);
}


VIDEO_START( mekd2 )
{
    machine->generic.videoram_size = 6 * 2 + 24;
    machine->generic.videoram.u8 = auto_alloc_array(machine, UINT8, machine->generic.videoram_size);
}


VIDEO_UPDATE( mekd2 )
{
	int width = video_screen_get_width(screen);
    int x, y;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3" };

    for (x = 0; x < 6; x++)
    {
        int sy = 408;
        int sx = width - 212 + x * 30 + ((x >= 4) ? 6 : 0);

        drawgfx_transpen (bitmap, NULL, screen->machine->gfx[0],
                 screen->machine->generic.videoram.u8[2 * x + 0], screen->machine->generic.videoram.u8[2 * x + 1],
                 0, 0, sx, sy, 0);
    }

    for (y = 0; y < 6; y++)
    {
        int sy = 516 + y * 36;

        for (x = 0; x < 4; x++)
        {
            static const int layout[6][4] =
            {
                {22, 19, 21, 23},
                {16, 17, 20, 18},
                {12, 13, 14, 15},
				{ 8,  9, 10, 11},
				{ 4,  5,  6,  7},
				{ 0,  1,  2,  3}
            };
            int sx = width - 182 + x * 37;
            int color, code = layout[y][x];

            color = (input_port_read(screen->machine, keynames[code / 7]) & (0x40 >> (code % 7))) ? 0 : 1;

            screen->machine->generic.videoram.u8[6 * 2 + code] = color;
            drawgfx_opaque (bitmap, NULL,
                     screen->machine->gfx[1],
                     layout[y][x], color,
                     0, 0, sx, sy);
        }
    }

	return 0;
}
