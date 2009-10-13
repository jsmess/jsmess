/*********************************************************************

	mscommon.h

	MESS specific generic functions

	TODO: This code needs to be removed, update the drivers
	using it.

*********************************************************************/

#ifndef __MSCOMMON_H__
#define __MSCOMMON_H__

#include "driver.h"


/***************************************************************************

	LED code

***************************************************************************/

/* draw_led() will both draw led (where the pixels are identified by '1' or
 * x-segment displays (where the pixels are masked with lowercase letters)
 *
 * the value of 'valueorcolor' is a mask when lowercase letters are in the led
 * string or is a color when '1' characters are in the led string
 */
void draw_led(running_machine *machine, bitmap_t *bitmap, const char *led, int valueorcolor, int x, int y);

/* a radius two led:
 *
 *	" 111\r"
 *	"11111\r"
 *	"11111\r"
 *	"11111\r"
 *	" 111";
 */
extern const char radius_2_led[];

/**************************************************************************/

#endif /* __MSCOMMON_H__ */
