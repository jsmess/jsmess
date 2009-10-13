/*********************************************************************

	mscommon.c

	MESS specific generic functions

*********************************************************************/

#include "driver.h"
#include "mscommon.h"


/***************************************************************************

	LED code

***************************************************************************/

void draw_led(running_machine *machine, bitmap_t *bitmap, const char *led, int valueorcolor, int x, int y)
{
	char c;
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++)
	{
		c = led[i];
		if (c == '1')
		{
			*BITMAP_ADDR16(bitmap, y+yi, x+xi) = valueorcolor;
		}
		else if (c >= 'a')
		{
			mask = 1 << (c - 'a');
			color = (valueorcolor & mask) ? 1 : 0;
			*BITMAP_ADDR16(bitmap, y+yi, x+xi) = color;
		}
		if (c != '\r')
		{
			xi++;
		}
		else
		{
			yi++;
			xi=0;
		}
	}
}

const char radius_2_led[] =
	" 111\r"
	"11111\r"
	"11111\r"
	"11111\r"
	" 111";
