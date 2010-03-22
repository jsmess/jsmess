/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate the video hardware of the coupe.

 At present these are not done using the mame driver standard, they are
 simply plot pixelled.. I will fix this in a future version.

***************************************************************************/

#include "emu.h"
#include "includes/samcoupe.h"


#define BORDER_COLOR(x)	((x & 0x20) >> 2 | (x & 0x07))

static void draw_left_border(running_device *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = (coupe_asic *)screen->machine->driver_data;

	for (int x = 0; x < SAM_BORDER_LEFT; x++)
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, x) = asic->clut[BORDER_COLOR(asic->border)];
}

static void draw_right_border(running_device *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = (coupe_asic *)screen->machine->driver_data;

	for (int x = SAM_BORDER_LEFT + SAM_SCREEN_WIDTH; x < SAM_BORDER_LEFT + SAM_SCREEN_WIDTH + SAM_BORDER_RIGHT; x++)
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, x) = asic->clut[BORDER_COLOR(asic->border)];
}

static void draw_mode4_line(running_device *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = (coupe_asic *)screen->machine->driver_data;

	/* get start address */
	UINT8 *vram = screen->machine->generic.videoram.u8 + (y * 128);

	for (int x = 0; x < SAM_SCREEN_WIDTH/4; x++)
	{
		/* draw 2 pixels (doublewidth) */
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 0) = asic->clut[(*vram >> 4) & 0x0f];
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 1) = asic->clut[(*vram >> 4) & 0x0f];
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 2) = asic->clut[(*vram >> 0) & 0x0f];
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 3) = asic->clut[(*vram >> 0) & 0x0f];

		/* move to next address */
		vram++;
	}
}

static void draw_mode3_line(running_device *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = (coupe_asic *)screen->machine->driver_data;

	/* get start address */
	UINT8 *vram = screen->machine->generic.videoram.u8 + (y * 128);

	for (int x = 0; x < 512/4; x++)
	{
		/* draw 4 pixels */
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 0) = asic->clut[(*vram >> 6) & 0x03];
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 1) = asic->clut[(*vram >> 4) & 0x03];
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 2) = asic->clut[(*vram >> 2) & 0x03];
		*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x * 4 + 3) = asic->clut[(*vram >> 0) & 0x03];

		/* move to next address */
		vram++;
	}
}

static void draw_mode2_line(running_device *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = (coupe_asic *)screen->machine->driver_data;
	int b, scrx = 0;
	UINT16 ink = 127, pap = 0;
	UINT8 *attr = screen->machine->generic.videoram.u8 + 32*192 + y*32;
	int x;

	for (x = 0; x < 256/8; x++)
	{
		UINT8 tmp = screen->machine->generic.videoram.u8[x + (y*32)];

#ifndef MONO
		ink = asic->clut[(*attr) & 0x07];
		pap = asic->clut[((*attr) >> 3) & 0x07];
#endif

		attr++;

		for (b = 0x80; b!= 0; b >>= 1)
		{
			*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + scrx++) = (tmp & b) ? ink : pap;
			*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + scrx++) = (tmp & b) ? ink : pap;
		}
	}
}

static void draw_mode1_line(running_device *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = (coupe_asic *)screen->machine->driver_data;
	int block, x = 0;

	/* get base address of attribute and data values */
	UINT8 *attr = screen->machine->generic.videoram.u8 + 32*192 + ((y & 0xf8) << 2);
	UINT8 *data = screen->machine->generic.videoram.u8 + (((y & 0xc0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2));

	/* loop over all 32 blocks */
	for (block = 0; block < 32; block++)
	{
        int bit;

        /* get current colors from attribute byte */
        UINT8 ink = asic->clut[(*attr & 0x07) + ((*attr >> 3) & 0x08)];
        UINT8 paper = asic->clut[(*attr >> 3) & 0x0f];

        /* draw a block (8 pixels, doubled) */
		for (bit = 0x80; bit != 0; bit >>= 1)
		{
			*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x++) = (*data & bit) ? ink : paper;
			*BITMAP_ADDR16(bitmap, SAM_BORDER_TOP + y, SAM_BORDER_LEFT + x++) = (*data & bit) ? ink : paper;
		}

		/* save current attribute */
		asic->attribute = *attr;

		/* move to the next block of values */
		attr++;
		data++;
	}
}

VIDEO_UPDATE( samcoupe )
{
	int scanline = video_screen_get_vpos(screen);
	coupe_asic *asic = (coupe_asic *)screen->machine->driver_data;

	/* border area? */
	if (scanline < SAM_BORDER_TOP || scanline >= SAM_SCREEN_HEIGHT + SAM_BORDER_BOTTOM)
	{
		plot_box(bitmap, 0, scanline, SAM_BORDER_LEFT + SAM_SCREEN_WIDTH + SAM_BORDER_RIGHT, 1, asic->clut[BORDER_COLOR(asic->border)]);
	}
	else
	{
		/* adjust for border */
		scanline -= SAM_BORDER_TOP;

		/* line interrupt? */
		if (asic->line_int == scanline)
			samcoupe_irq(screen->machine->firstcpu, SAM_LINE_INT);

		/* display disabled? (only in mode 3 or 4) */
		if ((asic->vmpr & 0x40) && (asic->border & 0x80))
		{
			/* display is disabled, draw a black screen */
			bitmap_fill(bitmap, cliprect, 0);
		}
		else
		{
			/* display enabled, start drawing border */
			draw_left_border(screen, bitmap, scanline);

			/* draw line in the right screen mode */
			switch ((asic->vmpr & 0x60) >> 5)
			{
			case 0:	draw_mode1_line(screen, bitmap, scanline); break;
			case 1:	draw_mode2_line(screen, bitmap, scanline); break;
			case 2:	draw_mode3_line(screen, bitmap, scanline); break;
			case 3:	draw_mode4_line(screen, bitmap, scanline); break;
			}

			/* rest of the border */
			draw_right_border(screen, bitmap, scanline);
		}
	//printf("videomode: %u\n", (asic->vmpr & 0x60) >> 5);
	}

	return 0;
}
