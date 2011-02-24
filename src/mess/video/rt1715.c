/***************************************************************************

        Robotron 1715 video driver by Miodrag Milanovic

        10/06/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/rt1715.h"

SCREEN_UPDATE( rt1715 )
{
	SCREEN_UPDATE_CALL ( generic_bitmapped );
	return 0;
}

static const rgb_t rt1715_palette[3] = {
	MAKE_RGB(0x00, 0x00, 0x00), // black
	MAKE_RGB(0xa0, 0xa0, 0xa0), // white
	MAKE_RGB(0xff, 0xff, 0xff)	// highlight
};

PALETTE_INIT( rt1715 )
{
	palette_set_colors(machine, 0, rt1715_palette, ARRAY_LENGTH(rt1715_palette));
}

