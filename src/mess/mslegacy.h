/*********************************************************************

	mslegacy.h

	Defines since removed from MAME, but kept in MESS for legacy
	reasons

*********************************************************************/

#ifndef MSLEGACY_H
#define MSLEGACY_H

#define TIME_IN_USEC(us)		((double)(us) * (1.0 / 1000000.0))

INLINE void palette_set_colors_rgb(running_machine *machine, pen_t color_base, const UINT8 *colors, int color_count)
{
	while (color_count--)
	{
		UINT8 r = *colors++;
		UINT8 g = *colors++;
		UINT8 b = *colors++;
		palette_set_color_rgb(machine, color_base++, r, g, b);
	}
}


#endif /* MSLEGACY_H */
