#include "driver.h"
#include "includes/pcw16.h"
#include "video/generic.h"

int pcw16_colour_palette[16];
int pcw16_video_control;

/* 16 colours, + 1 for border */
unsigned short pcw16_colour_table[PCW16_NUM_COLOURS] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
	29, 30, 31
};

unsigned char pcw16_palette[PCW16_NUM_COLOURS * 3] =
{
	0x080, 0x080, 0x080,	/* light grey */
	0x080, 0x080, 0x080,	/* light grey */
	0x000, 0x080, 0x080,	/* magenta */
	0x000, 0x080, 0x080,	/* magenta */
	0x080, 0x080, 0x080,	/* light grey */
	0x080, 0x080, 0x080,	/* light grey */
	0x0ff, 0x080, 0x080,	/* pastel green */
	0x0ff, 0x080, 0x080,	/* pastel green */
	0x000, 0x000, 0x080,	/* blue */
	0x000, 0x000, 0x000,	/* black */
	0x000, 0x080, 0x0ff,	/* mauve */
	0x000, 0x000, 0x0ff,	/* bright blue */
	0x000, 0x080, 0x000,	/* red */
	0x000, 0x0ff, 0x000,	/* bright red */
	0x000, 0x0ff, 0x080,	/* purple */
	0x000, 0x0ff, 0x0ff,	/* bright magenta */
	0x0ff, 0x000, 0x080,	/* sea green */
	0x0ff, 0x000, 0x0ff,	/* bright green */
	0x0ff, 0x080, 0x0ff,	/* pastel cyan */
	0x0ff, 0x000, 0x0ff,	/* bright cyan */
	0x0ff, 0x080, 0x000,	/* lime green */
	0x0ff, 0x0ff, 0x000,	/* bright yellow */
	0x0ff, 0x0ff, 0x080,	/* pastel yellow */
	0x0ff, 0x0ff, 0x0ff,	/* bright white */
	0x080, 0x000, 0x080,	/* cyan */
	0x080, 0x000, 0x000,	/* green */
	0x080, 0x080, 0x0ff,	/* pastel blue */
	0x080, 0x000, 0x0ff,	/* sky blue */
	0x080, 0x080, 0x000,	/* yellow */
	0x080, 0x0ff, 0x000,	/* orange */
	0x080, 0x0ff, 0x080,	/* pink */
	0x080, 0x0ff, 0x0ff,	/* pastel magenta */
};


/* Initialise the palette */
PALETTE_INIT( pcw16 )
{
	palette_set_colors(machine, 0, pcw16_palette, sizeof(pcw16_palette) / 3);
	memcpy(colortable, pcw16_colour_table, sizeof (pcw16_colour_table));
}

VIDEO_START( pcw16 )
{
	return 0;
}

/* 640, 1 bit per pixel */
static void pcw16_vh_decode_mode0(mame_bitmap *bitmap, int x, int y, unsigned char byte)
{
	int b;
	int local_byte;
	int cols[2];
	int px;

	local_byte = byte;

	cols[0] = Machine->pens[pcw16_colour_palette[0]];
	cols[1] = Machine->pens[pcw16_colour_palette[1]];

	px = x;
	for (b=0; b<8; b++)
	{
		plot_pixel(bitmap, px, y, cols[(local_byte>>7) & 0x01]);
		px++;

		local_byte = local_byte<<1;
	}
}

/* 320, 2 bits per pixel */
static void pcw16_vh_decode_mode1(mame_bitmap *bitmap, int x, int y, unsigned char byte)
{
	int b;
	int px;
	int local_byte;
	int cols[4];

	for (b=0; b<3; b++)
	{
		cols[b] = Machine->pens[pcw16_colour_palette[b]];
	}

	local_byte = byte;

	px = x;
	for (b=0; b<4; b++)
	{
		int col;

		col = cols[((local_byte>>6) & 0x03)];

		plot_pixel(bitmap, px, y, col);
		px++;
		plot_pixel(bitmap, px, y, col);
		px++;

		local_byte = local_byte<<2;
	}
}

/* 160, 4 bits per pixel */
static void pcw16_vh_decode_mode2(mame_bitmap *bitmap, int x, int y, unsigned char byte)
{
	int px;
	int b;
	int local_byte;
	int cols[2];

	cols[0] = Machine->pens[pcw16_colour_palette[0]];
	cols[1] = Machine->pens[pcw16_colour_palette[1]];
	local_byte = byte;

	px = x;
	for (b=0; b<2; b++)
	{
		int col;

		col = cols[((local_byte>>4)&0x0f)];

		plot_pixel(bitmap, px, y, col);
		px++;
		plot_pixel(bitmap, px, y, col);
		px++;
		plot_pixel(bitmap, px, y, col);
		px++;
		plot_pixel(bitmap, px, y, col);
		px++;

		local_byte = local_byte<<4;
	}
}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( pcw16 )
{
	unsigned char *pScanLine = (unsigned char *)mess_ram + 0x0fc00;	//0x03c00;	//0x020FC00;

	int y;
	int x;

	int border_colour;

	border_colour = pcw16_video_control & 31;

	/* reverse video? */
	if (pcw16_video_control & (1<<7))
	{
		/* colour 0 and colour 1 need to be inverted? - what happens in mode 1 and 2 - ignored? or is bit 1 toggled,
		or is whole lot toggled? */

		/* force border to be colour 1 */
		border_colour = pcw16_colour_palette[1];
	}

	border_colour = Machine->pens[border_colour];

	if ((pcw16_video_control & (1<<6))==0)
	{
		/* blank */
		rectangle rect;

		rect.min_x = 0;
		rect.min_y = 0;
		rect.max_x = PCW16_SCREEN_WIDTH;
		rect.max_y = PCW16_SCREEN_HEIGHT;

		fillbitmap(bitmap, border_colour, &rect);
	}
	else
	{
		/* no blank */

		rectangle rect;

		/* render top border */
		rect.min_x = 0;
		rect.min_y = 0;
		rect.max_x = PCW16_SCREEN_WIDTH;
		rect.max_y = PCW16_BORDER_HEIGHT;
		fillbitmap(bitmap, border_colour, &rect);

		/* render bottom border */
		rect.min_x = 0;
		rect.min_y = PCW16_BORDER_HEIGHT + PCW16_DISPLAY_HEIGHT;
		rect.max_x = PCW16_SCREEN_WIDTH;
		rect.max_y = rect.min_y + PCW16_BORDER_HEIGHT;
		fillbitmap(bitmap, border_colour, &rect);

		/* render border on either side of display */
		plot_box(bitmap, 0,											PCW16_BORDER_HEIGHT, 8, PCW16_DISPLAY_HEIGHT, border_colour);
		plot_box(bitmap, PCW16_DISPLAY_WIDTH + PCW16_BORDER_WIDTH,	PCW16_BORDER_HEIGHT, 8, PCW16_DISPLAY_HEIGHT, border_colour);

		/* render display */
		for (y=0; y<PCW16_DISPLAY_HEIGHT; y++)
		{
			int b;
			int ScanLineAddr;
			int Addr;
			int AddrUpper;
			int mode;

			/* get line address */
			ScanLineAddr = (pScanLine[0] & 0x0ff) | ((pScanLine[1] & 0x0ff)<<8);

			/* generate address */
			Addr = (ScanLineAddr & 0x03fff)<<4;

			/* get upper bits of addr */
			AddrUpper = Addr & (~0x0ffff);

			/* get mode */
			mode = ((ScanLineAddr>>14) & 0x03);

			/* set initial x position */
			x = PCW16_BORDER_WIDTH;

			for (b=0; b<80; b++)
			{
				int byte;

				byte = mess_ram[Addr];

				switch (mode)
				{
					case 0:
					{
						pcw16_vh_decode_mode0(bitmap,x,y+PCW16_BORDER_HEIGHT,byte);
					}
					break;

					case 1:
					{
						pcw16_vh_decode_mode1(bitmap, x,y+PCW16_BORDER_HEIGHT, byte);
					}
					break;

					case 3:
					case 2:
					{
						pcw16_vh_decode_mode2(bitmap, x, y+PCW16_BORDER_HEIGHT, byte);
					}
					break;
				}

				/* only lowest 16 bits are incremented between fetches */
				Addr = ((Addr+1) & 0x0ffff) | AddrUpper;

				x=x+8;
			}

			pScanLine+=2;
		}
	}
	return 0;
}
