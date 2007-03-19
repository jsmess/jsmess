/*********************************************************************

	vidhrdw/apple2gs.c

	Apple IIgs video code

*********************************************************************/

#include "mame.h"
#include "includes/apple2.h"
#include "includes/apple2gs.h"



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
			scanline = BITMAP_ADDR16(bitmap, row, 0);

			if (scb & 0x80)
			{
				for (col = 0; col < 160; col++)
				{
					b = vram[col];
					scanline[col * 4 + 0] = palette +  0 + ((b >> 6) & 0x03);
					scanline[col * 4 + 1] = palette +  4 + ((b >> 4) & 0x03);
					scanline[col * 4 + 2] = palette +  8 + ((b >> 2) & 0x03);
					scanline[col * 4 + 3] = palette + 12 + ((b >> 0) & 0x03);
				}
			}
			else
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
					scanline[col * 4 + 0] = pixel;
					scanline[col * 4 + 1] = pixel;

					b = vram[col];
					pixel = (b >> 0) & 0x0f;

					if ((scb & 0x20) && !pixel)
						pixel = last_pixel;
					else
						last_pixel = pixel;
					pixel += palette;
					scanline[col * 4 + 2] = pixel;
					scanline[col * 4 + 3] = pixel;
				}
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
		
		for (i = 0; i < 192; i++)
		{
			scanline = BITMAP_ADDR16(bitmap, i + 4, 0);

			for (j = 0; j < 40; j++)
			{
				scanline[j + 0] = apple2gs_bordercolor;
				scanline[j + 40 + 560] = apple2gs_bordercolor;
			}
			memcpy(scanline + 40, BITMAP_ADDR16(apple2gs_legacy_gfx, i, 0), 560 * sizeof(UINT16));
		}

		for (i = 0; i < 4; i++)
		{
			memset16(BITMAP_ADDR16(bitmap, i + 0, 0), apple2gs_bordercolor, 640);
			memset16(BITMAP_ADDR16(bitmap, i + 4 + 192, 0), apple2gs_bordercolor, 640);
		}
	}
	return 0;
}

