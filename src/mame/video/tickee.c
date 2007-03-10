/***************************************************************************

    Raster Elite Tickee Tickats hardware

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "tickee.h"


UINT16 *tickee_vram;


/* local variables */
static void *setup_gun_timer;



/*************************************
 *
 *  Compute X/Y coordinates
 *
 *************************************/

INLINE void get_crosshair_xy(int player, int *x, int *y)
{
	*x = (((readinputport(4 + player * 2) & 0xff) * (Machine->screen[0].visarea.max_x - Machine->screen[0].visarea.min_x)) >> 8) + Machine->screen[0].visarea.min_x;
	*y = (((readinputport(5 + player * 2) & 0xff) * (Machine->screen[0].visarea.max_y - Machine->screen[0].visarea.min_y)) >> 8) + Machine->screen[0].visarea.min_y;
}



/*************************************
 *
 *  Light gun interrupts
 *
 *************************************/

static void trigger_gun_interrupt(int which)
{
	/* fire the IRQ at the correct moment */
	cpunum_set_input_line(0, which, ASSERT_LINE);
}


static void clear_gun_interrupt(int which)
{
	/* clear the IRQ on the next scanline? */
	cpunum_set_input_line(0, which, CLEAR_LINE);
}


static void setup_gun_interrupts(int param)
{
	int beamx, beamy;
	mame_time time;

	/* set a timer to do this again next frame */
	mame_timer_adjust(setup_gun_timer, video_screen_get_time_until_pos(0, 0, 0), 0, time_zero);

	/* only do work if the palette is flashed */
	if (!tickee_control[2])
		return;

	/* generate interrupts for player 1's gun */
	get_crosshair_xy(0, &beamx, &beamy);
	time = video_screen_get_time_until_pos(0, beamy, 0);
	time = add_mame_times(time, double_to_mame_time(mame_time_to_double(video_screen_get_scan_period(0)) * (double)(beamx + 175) / (double)450));
	mame_timer_set(time, 0, trigger_gun_interrupt);
	mame_timer_set(add_mame_times(time, video_screen_get_scan_period(0)), 0, clear_gun_interrupt);

	/* generate interrupts for player 2's gun */
	get_crosshair_xy(1, &beamx, &beamy);
	time = video_screen_get_time_until_pos(0, beamy, 0);
	time = add_mame_times(time, double_to_mame_time(mame_time_to_double(video_screen_get_scan_period(0)) * (double)(beamx + 175) / (double)450));
	mame_timer_set(time, 1, trigger_gun_interrupt);
	mame_timer_set(add_mame_times(time, video_screen_get_scan_period(0)), 1, clear_gun_interrupt);
}



/*************************************
 *
 *  Video startup
 *
 *************************************/

VIDEO_START( tickee )
{
	/* start a timer going on the first scanline of every frame */
	setup_gun_timer = mame_timer_alloc(setup_gun_interrupts);
	mame_timer_adjust(setup_gun_timer, video_screen_get_time_until_pos(0, 0, 0), 0, time_zero);

	return 0;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( tickee )
{
	int v, h, width, xoffs;
	UINT8 *base1 = (UINT8 *)tickee_vram;
	pen_t pen_lookup[256];
	UINT32 offset;

	/* fill out the pen array based on the palette bank */
	for (h = 0; h < 256; h++)
		pen_lookup[h] = tickee_control[2] ? 255 : h;

	/* determine the base of the videoram */
	offset = (~tms34010_get_DPYSTRT(0) & 0xfff0) << 5;
	offset += TOBYTE(0x1000) * (cliprect->min_y - Machine->screen[0].visarea.min_y);

	/* determine how many pixels to copy */
	xoffs = cliprect->min_x;
	width = cliprect->max_x - xoffs + 1;
	offset += xoffs;

	/* loop over rows */
	for (v = cliprect->min_y; v <= cliprect->max_y; v++)
	{
		UINT8 scanline[512];

		/* extract the scanline to account for endianness */
		for (h = 0; h < width; h++)
			scanline[h] = base1[BYTE_XOR_LE(offset + h) & 0x7ffff];

		/* draw it and advance */
		draw_scanline8(bitmap, xoffs, v, width, scanline, pen_lookup, -1);
		offset += TOBYTE(0x1000);
	}

	return 0;
}

