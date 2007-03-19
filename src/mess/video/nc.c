/***************************************************************************

  nc.c

  Functions to emulate the video hardware of the Amstrad PCW.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/nc.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

VIDEO_START( nc )
{
	return 0;
}

/* two colours */
static unsigned short nc_colour_table[NC_NUM_COLOURS] =
{
	0, 1,2,3
};

/* black/white */
static unsigned char nc_palette[NC_NUM_COLOURS * 3] =
{
	0x060, 0x060, 0x060,
	0x000, 0x000, 0x000,
	0x080, 0x0a0, 0x060,
    0x000, 0x000, 0x000
};


/* Initialise the palette */
PALETTE_INIT( nc )
{
	palette_set_colors(machine, 0, nc_palette, sizeof(nc_palette) / 3);
	memcpy(colortable, nc_colour_table, sizeof (nc_colour_table));
}

extern int nc_display_memory_start;
extern UINT8 nc_type;

static int nc200_backlight = 0;

void nc200_video_set_backlight(int state)
{
	nc200_backlight = state;
}


/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( nc )
{
	int y;
	int b;
	int x;
	int height, width;
	int pens[2];

    if (nc_type==NC_TYPE_200)
    {
        height = NC200_SCREEN_HEIGHT;
        width = NC200_SCREEN_WIDTH;

		if (nc200_backlight)
		{
			pens[0] = Machine->pens[2];
			pens[1] = Machine->pens[3];
		}
		else
		{
			pens[0] = Machine->pens[0];
			pens[1] = Machine->pens[1];
		}
    }
    else
    {
		height = NC_SCREEN_HEIGHT;
		width = NC_SCREEN_WIDTH;
		pens[0] = Machine->pens[2];
		pens[1] = Machine->pens[3];	
	}


    for (y=0; y<height; y++)
    {
		int by;
		/* 64 bytes per line */
		char *line_ptr = ((char*)mess_ram) + nc_display_memory_start + (y<<6);

		x = 0;
		for (by=0; by<width>>3; by++)
		{
			int px;
			unsigned char byte;

			byte = line_ptr[0];

			px = x;
			for (b=0; b<8; b++)
			{
				plot_pixel(bitmap, px, y, pens[(byte>>7) & 0x01]);
				byte = byte<<1;
				px++;
			}

			x = px;
							
			line_ptr = line_ptr+1;
		}
	}
	return 0;
}

