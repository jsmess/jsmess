/***************************************************************************

    VIC Dual Game board

****************************************************************************/

#include "driver.h"
#include "includes/vicdual.h"


static UINT8 palette_bank;


#define RGB32_BLACK		(0x00000000)
#define RGB32_WHITE		(0x00ffffff)

static pen_t colors_from_color_prom[] =
{
	0x00000000,
	0x0000ff00,
	0x000000ff,
	0x0000ffff,
	0x00ff0000,
	0x00ffff00,
	0x00ff00ff,
	0x00ffffff
};


WRITE8_HANDLER( vicdual_palette_bank_w )
{
	video_screen_update_partial(0, video_screen_get_vpos(0));

	palette_bank = data & 3;
}


VIDEO_UPDATE( vicdual_bw )
{
	UINT8 x = 0;
	UINT8 y = cliprect->min_y;
	UINT8 video_data = 0;

	while (1)
	{
		pen_t col;

		if ((x & 0x07) == 0)
		{
			offs_t offs;
			UINT8 char_code;

			/* read the character code */
			offs = (y >> 3 << 5) | (x >> 3);
			char_code = vicdual_videoram_r(offs);

			/* read the appropriate line of the character ram */
			offs = (char_code << 3) | (y & 0x07);
			video_data = vicdual_characterram_r(offs);
		}

		/* plot the current pixel */
		col = (video_data & 0x80) ? RGB32_WHITE : RGB32_BLACK;
		plot_pixel(bitmap, x, y, col);

		/* next pixel */
		video_data = video_data << 1;
		x = x + 1;

		/* end of line? */
		if (x == 0)
		{
			/* end of region to update? */
			if (y == cliprect->max_y)
			{
				break;
			}

			/* next row */
			y = y + 1;
		}
	}

	return 0;
}


VIDEO_UPDATE( vicdual_color )
{
	UINT8 *color_prom = (UINT8 *)memory_region(REGION_PROMS);
	UINT8 x = 0;
	UINT8 y = cliprect->min_y;
	UINT8 video_data = 0;
	pen_t back_color = 0;
	pen_t fore_color = 0;

	while (1)
	{
		pen_t col;

		if ((x & 0x07) == 0)
		{
			offs_t offs;
			UINT8 char_code;

			/* read the character code */
			offs = (y >> 3 << 5) | (x >> 3);
			char_code = vicdual_videoram_r(offs);

			/* read the appropriate line of the character ram */
			offs = (char_code << 3) | (y & 0x07);
			video_data = vicdual_characterram_r(offs);

			/* get the foreground and background colors from the PROM */
			offs = (char_code >> 5) | (palette_bank << 3);
			back_color = colors_from_color_prom[(color_prom[offs] >> 1) & 0x07];
			fore_color = colors_from_color_prom[(color_prom[offs] >> 5) & 0x07];
		}

		/* plot the current pixel */
		col = (video_data & 0x80) ? fore_color : back_color;
		plot_pixel(bitmap, x, y, col);

		/* next pixel */
		video_data = video_data << 1;
		x = x + 1;

		/* end of line? */
		if (x == 0)
		{
			/* end of region to update? */
			if (y == cliprect->max_y)
			{
				break;
			}

			/* next row */
			y = y + 1;
		}
	}

	return 0;
}


VIDEO_UPDATE( vicdual_bw_or_color )
{
	if (vicdual_is_cabinet_color())
		video_update_vicdual_color(machine, screen, bitmap, cliprect);
	else
		video_update_vicdual_bw(machine, screen, bitmap, cliprect);

	return 0;
}
