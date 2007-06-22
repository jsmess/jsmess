/***************************************************************************

    Raster Elite Tickee Tickats hardware

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "tickee.h"


UINT16 *tickee_vram;


/* local variables */
static mame_timer *setup_gun_timer;



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
}



/*************************************
 *
 *  Video update
 *
 *************************************/

void tickee_scanline_update(running_machine *machine, int screen, mame_bitmap *bitmap, int scanline, const tms34010_display_params *params)
{
	UINT16 *src = &tickee_vram[(params->rowaddr << 8) & 0x3ff00];
	UINT16 *dest = BITMAP_ADDR16(bitmap, scanline, 0);
	int coladdr = params->coladdr << 1;
	int x;

	/* blank palette: fill with pen 255 */
	if (tickee_control[2])
	{
		for (x = params->heblnk; x < params->hsblnk; x++)
			dest[x] = 0xff;
		return;
	}

	/* copy the non-blanked portions of this scanline */
	for (x = params->heblnk; x < params->hsblnk; x += 2)
	{
		UINT16 pixels = src[coladdr++ & 0xff];
		dest[x + 0] = pixels & 0xff;
		dest[x + 1] = pixels >> 8;
	}
}
