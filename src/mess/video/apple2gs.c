/*********************************************************************

	vidhrdw/apple2gs.c

	Apple IIgs video code

*********************************************************************/

#include "mame.h"
#include "includes/apple2.h"
#include "includes/apple2gs.h"

#define BORDER_LEFT	(32)
#define BORDER_RIGHT	(32)
#define BORDER_TOP	(16)	// (plus bottom)

static mame_bitmap *apple2gs_legacy_gfx;
UINT16 apple2gs_bordercolor;

VIDEO_START( apple2gs )
{
	apple2gs_bordercolor = 0;

	if (apple2_video_start(apple2gs_slowmem, 0x20000, 0, 8))
		return 1;

	apple2gs_legacy_gfx = auto_bitmap_alloc(560, 192, BITMAP_FORMAT_INDEXED16);
	if (!apple2gs_legacy_gfx)
		return 1;

	return 0;
}



VIDEO_UPDATE( apple2gs )
{
	const UINT8 *vram;
	UINT16 *scanline;
	UINT8 scb, b;
	int i, j, row, col, palette;
	UINT16 last_pixel = 0, pixel;
	rectangle new_cliprect;

	if (apple2gs_newvideo & 0x80)
	{
		/* draw the top border */
		for (row = 0; row < BORDER_TOP; row++)
		{
			scanline = BITMAP_ADDR16(bitmap, row, 0);
			for (col = 0; col < BORDER_LEFT+BORDER_RIGHT+640; col++)
			{
				scanline[col] = apple2gs_bordercolor;
			}
		}

		/* use Apple IIgs super hires video */
		for (i = 0; i < 256; i++)
		{
			palette_set_color(machine, i + 16,
				((apple2gs_slowmem[0x19E00 + (i * 2) + 1] >> 0) & 0x0F) * 17,
				((apple2gs_slowmem[0x19E00 + (i * 2) + 0] >> 4) & 0x0F) * 17,
				((apple2gs_slowmem[0x19E00 + (i * 2) + 0] >> 0) & 0x0F) * 17);
		}

		for (row = 0; row < 200; row++)
		{
			scb = apple2gs_slowmem[0x19D00 + row];
			palette = ((scb & 0x0f) << 4) + 16;

			vram = &apple2gs_slowmem[0x12000 + (row * 160)];
			scanline = BITMAP_ADDR16(bitmap, row+BORDER_TOP, 0);

			// draw left and right borders
			for (col = 0; col < BORDER_LEFT; col++)
			{
				scanline[col] = apple2gs_bordercolor;
				scanline[col+BORDER_LEFT+640] = apple2gs_bordercolor;
			}

			if (scb & 0x80)	// 640 mode
			{
				for (col = 0; col < 160; col++)
				{
					b = vram[col];
					scanline[col * 4 + 0 + BORDER_LEFT] = palette +  0 + ((b >> 6) & 0x03);
					scanline[col * 4 + 1 + BORDER_LEFT] = palette +  4 + ((b >> 4) & 0x03);
					scanline[col * 4 + 2 + BORDER_LEFT] = palette +  8 + ((b >> 2) & 0x03);
					scanline[col * 4 + 3 + BORDER_LEFT] = palette + 12 + ((b >> 0) & 0x03);
				}
			}
			else		// 320 mode
			{
				for (col = 0; col < 160; col++)
				{
					b = vram[col];
					pixel = (b >> 4) & 0x0f;

					if ((scb & 0x20) && !pixel)
						pixel = last_pixel;
					else
						last_pixel = pixel;
					pixel += palette;
					scanline[col * 4 + 0 + BORDER_LEFT] = pixel;
					scanline[col * 4 + 1 + BORDER_LEFT] = pixel;

					b = vram[col];
					pixel = (b >> 0) & 0x0f;

					if ((scb & 0x20) && !pixel)
						pixel = last_pixel;
					else
						last_pixel = pixel;
					pixel += palette;
					scanline[col * 4 + 2 + BORDER_LEFT] = pixel;
					scanline[col * 4 + 3 + BORDER_LEFT] = pixel;
				}
			}
		}

		/* draw the bottom border */
		for (row = 200+BORDER_TOP; row < 231+BORDER_TOP; row++)
		{
			scanline = BITMAP_ADDR16(bitmap, row, 0);
			for (col = 0; col < BORDER_LEFT+BORDER_RIGHT+640; col++)
			{
				scanline[col] = apple2gs_bordercolor;
			}
		}
	}
	else
	{
		/* call legacy Apple II video rendering */
		new_cliprect.min_x = MAX(cliprect->min_x - 40, 0);
		new_cliprect.min_y = MAX(cliprect->min_y - 4, 0);
		new_cliprect.max_x = MIN(cliprect->max_x - 40, 559);
		new_cliprect.max_y = MIN(cliprect->max_y - 4, 191);
		if ((new_cliprect.max_x > new_cliprect.min_x) && (new_cliprect.max_y > new_cliprect.min_y))
			video_update_apple2(machine, screen, apple2gs_legacy_gfx, &new_cliprect);

		/* draw the top border */
		for (row = 0; row < BORDER_TOP + 4; row++)
		{
			scanline = BITMAP_ADDR16(bitmap, row, 0);
			for (col = 0; col < BORDER_LEFT+BORDER_RIGHT+640; col++)
			{
				scanline[col] = apple2gs_bordercolor;
			}
		}
		
		for (i = 0; i < 192; i++)
		{
			scanline = BITMAP_ADDR16(bitmap, i + 4 + BORDER_TOP, 0);

			// draw left and right borders
			for (col = 0; col < BORDER_LEFT + 40; col++)
			{
				scanline[col] = apple2gs_bordercolor;
				scanline[col+BORDER_LEFT+600] = apple2gs_bordercolor;
			}

			memcpy(scanline + 40 + BORDER_LEFT, BITMAP_ADDR16(apple2gs_legacy_gfx, i, 0), 560 * sizeof(UINT16));
		}

		/* draw the bottom border */
		for (row = 192+4+BORDER_TOP; row < 231+BORDER_TOP; row++)
		{
			scanline = BITMAP_ADDR16(bitmap, row, 0);
			for (col = 0; col < BORDER_LEFT+BORDER_RIGHT+640; col++)
			{
				scanline[col] = apple2gs_bordercolor;
			}
		}
	}
	return 0;
}

