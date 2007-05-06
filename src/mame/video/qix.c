/***************************************************************************

    Taito Qix hardware

    driver by John Butler, Ed Mueller, Aaron Giles

***************************************************************************/

#include "driver.h"
#include "qix.h"
#include "crtc6845.h"


/* Constants */
#define SCANLINE_INCREMENT	1


/* Globals */
UINT8 *qix_videoaddress;
UINT8 qix_cocktail_flip;


/* Local variables */
static UINT8 vram_mask;
static UINT8 qix_palettebank;
static UINT8 leds;
static mame_timer *scanline_timer;


/* Prototypes */
static void scanline_callback(int scanline);



/*************************************
 *
 *  Video startup
 *
 *************************************/

VIDEO_START( qix )
{
	/* allocate memory for the full video RAM */
	videoram = auto_malloc(256 * 256);

	/* initialize the mask for games that don't use it */
	vram_mask = 0xff;

	/* allocate a timer */
	scanline_timer = mame_timer_alloc(scanline_callback);
	mame_timer_adjust(scanline_timer, video_screen_get_time_until_pos(0, 1, 0), 1, time_zero);

	/* set up save states */
	crtc6845_init();
	state_save_register_global_pointer(videoram, 256 * 256);
	state_save_register_global(qix_cocktail_flip);
	state_save_register_global(vram_mask);
	state_save_register_global(qix_palettebank);
	state_save_register_global(leds);

	return 0;
}



/*************************************
 *
 *  Scanline caching
 *
 *************************************/

static void scanline_callback(int scanline)
{
	/* force a partial update */
	video_screen_update_partial(0, scanline - 1);

	/* set a timer for the next increment */
	scanline += SCANLINE_INCREMENT;
	if (scanline > Machine->screen[0].visarea.max_y)
		scanline = SCANLINE_INCREMENT;
	mame_timer_adjust(scanline_timer, video_screen_get_time_until_pos(0, scanline, 0), scanline, time_zero);
}



/*************************************
 *
 *  Current scanline read
 *
 *************************************/

READ8_HANDLER( qix_scanline_r )
{
	/*
        This doesn't actually latch the scanline; rather, it latches the upper bits
        of the address output by the 6845CRTC; this is based on the starting address
        loaded into the CRTC registers plus the scanline. Furthermore, the transparent
        latch holding the address is latched when the display is disabled; this means
        that during blanking periods it continues to hold the last address output
    */
	int startaddr = (crtc6845.start_addr & 0x3ff) >> 2;
	int scanline = video_screen_get_vpos(0);
	scanline = MIN(scanline, Machine->screen[0].visarea.max_y);
	return (scanline + startaddr) & 0xff;
}



/*************************************
 *
 *  Video controller read/write
 *
 *************************************/

READ8_HANDLER( qix_videocontrol_r )
{
	return crtc6845_register_r(offset);
}


WRITE8_HANDLER( qix_videocontrol_w )
{
	/* even offsets write to the address register */
	if (offset == 0)
		crtc6845_address_w(offset, data);

	/* odd offsets write to the register itself */
	else
	{
		int vtotal, vvisible, vfrac, scans_per_char;
		int htotal, hvisible;
		rectangle visarea;

		crtc6845_register_w(offset, data);

		/* grab the screen parameters */
		htotal = crtc6845.horiz_total + 1;
		hvisible = crtc6845.horiz_disp;
		scans_per_char = crtc6845.max_ras_addr + 1;
		vtotal = crtc6845.vert_total + 1;
		vfrac = crtc6845.vert_total_adj;
		vvisible = crtc6845.vert_disp;

		/* determine the visible area */
		visarea.min_x = visarea.min_y = 0;
		visarea.max_x = hvisible * 8 - 1;
		visarea.max_y = vvisible * 8 - 1;

		/* update the screen if we have valid data */
		if (htotal > 1 && visarea.max_x < htotal * 8 && vtotal > 1 && visarea.max_y < vtotal * scans_per_char + vfrac)
			video_screen_configure(0, htotal * 8, vtotal * scans_per_char + vfrac, &visarea, HZ_TO_SUBSECONDS(QIX_CHARACTER_CLOCK) * htotal * (vtotal * scans_per_char + vfrac));
	}
}



/*************************************
 *
 *  Video RAM mask
 *
 *************************************/

WRITE8_HANDLER( slither_vram_mask_w )
{
	/* Slither appears to extend the basic hardware by providing */
	/* a mask register which controls which data bits get written */
	/* to video RAM */
	vram_mask = data;
}



/*************************************
 *
 *  Direct video RAM read/write
 *
 *  The screen is 256x256 with eight
 *  bit pixels (64K).  The screen is
 *  divided into two halves each half
 *  mapped by the video CPU at
 *  $0000-$7FFF.  The high order bit
 *  of the address latch at $9402
 *  specifies which half of the screen
 *  is being accessed.
 *
 *************************************/

READ8_HANDLER( qix_videoram_r )
{
	/* add in the upper bit of the address latch */
	offset += (qix_videoaddress[0] & 0x80) << 8;
	return videoram[offset];
}


WRITE8_HANDLER( qix_videoram_w )
{
	/* add in the upper bit of the address latch */
	offset += (qix_videoaddress[0] & 0x80) << 8;

	/* blend the data */
	videoram[offset] = (videoram[offset] & ~vram_mask) | (data & vram_mask);
}



/*************************************
 *
 *  Latched video RAM read/write
 *
 *  The address latch works as follows.
 *  When the video CPU accesses $9400,
 *  the screen address is computed by
 *  using the values at $9402 (high
 *  byte) and $9403 (low byte) to get
 *  a value between $0000-$FFFF.  The
 *  value at that location is either
 *  returned or written.
 *
 *************************************/

READ8_HANDLER( qix_addresslatch_r )
{
	/* compute the value at the address latch */
	offset = (qix_videoaddress[0] << 8) | qix_videoaddress[1];
	return videoram[offset];
}



WRITE8_HANDLER( qix_addresslatch_w )
{
	/* compute the value at the address latch */
	offset = (qix_videoaddress[0] << 8) | qix_videoaddress[1];

	/* blend the data */
	videoram[offset] = (videoram[offset] & ~vram_mask) | (data & vram_mask);
}



/*************************************
 *
 *  Palette RAM
 *
 *************************************/

WRITE8_HANDLER( qix_paletteram_w )
{
	/* this conversion table should be about right. It gives a reasonable */
	/* gray scale in the test screen, and the red, green and blue squares */
	/* in the same screen are barely visible, as the manual requires. */
	static UINT8 table[16] =
	{
		0x00,	/* value = 0, intensity = 0 */
		0x12,	/* value = 0, intensity = 1 */
		0x24,	/* value = 0, intensity = 2 */
		0x49,	/* value = 0, intensity = 3 */
		0x12,	/* value = 1, intensity = 0 */
		0x24,	/* value = 1, intensity = 1 */
		0x49,	/* value = 1, intensity = 2 */
		0x92,	/* value = 1, intensity = 3 */
		0x5b,	/* value = 2, intensity = 0 */
		0x6d,	/* value = 2, intensity = 1 */
		0x92,	/* value = 2, intensity = 2 */
		0xdb,	/* value = 2, intensity = 3 */
		0x7f,	/* value = 3, intensity = 0 */
		0x91,	/* value = 3, intensity = 1 */
		0xb6,	/* value = 3, intensity = 2 */
		0xff	/* value = 3, intensity = 3 */
	};
	int bits, intensity, red, green, blue;

	/* set the palette RAM value */
	paletteram[offset] = data;

	/* compute R, G, B from the table */
	intensity = (data >> 0) & 0x03;
	bits = (data >> 6) & 0x03;
	red = table[(bits << 2) | intensity];
	bits = (data >> 4) & 0x03;
	green = table[(bits << 2) | intensity];
	bits = (data >> 2) & 0x03;
	blue = table[(bits << 2) | intensity];

	/* update the palette */
	palette_set_color(Machine, offset, red, green, blue);
}


WRITE8_HANDLER( qix_palettebank_w )
{
	/* set the bank value */
	if (qix_palettebank != (data & 3))
	{
		video_screen_update_partial(0, video_screen_get_vpos(0) - 1);
		qix_palettebank = data & 3;
	}

	/* LEDs are in the upper 6 bits */
	leds = ~data & 0xfc;
}



/*************************************
 *
 *  Core video refresh
 *
 *************************************/

VIDEO_UPDATE( qix )
{
	int startaddr = (crtc6845.start_addr & 0x3ff) << 6;
	pen_t *pens = &machine->pens[qix_palettebank * 256];
	UINT8 scanline[256];
	int offset;
	int x,y;

	/* draw the bitmap */
	if (qix_cocktail_flip)
	{
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			offset = startaddr + (y ^ 0xff) * 256;
			for (x = 0;x < 256;x++)
				scanline[x] = videoram[(offset + (x ^ 0xff)) & 0xffff];
			draw_scanline8(bitmap, 0, y, 256, scanline, pens, -1);
		}
	}
	else
	{
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			offset = startaddr + y * 256;
			for (x = 0;x < 256;x++)
				scanline[x] = videoram[(offset + x) & 0xffff];
			draw_scanline8(bitmap, 0, y, 256, scanline, pens, -1);
		}
	}
#if 0
	// note the confusing bit order!
	popmessage("self test leds: %d%d %d%d%d%d",BIT(leds,7),BIT(leds,5),BIT(leds,6),BIT(leds,4),BIT(leds,2),BIT(leds,3));
#endif
	return 0;
}
