/***************************************************************************

        Apogee BK-01 driver by Miodrag Milanovic

        05/06/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "video/i8275.h"

I8275_DISPLAY_PIXELS(apogee_display_pixels)
{
	int i;
	bitmap_t *bitmap = tmpbitmap;
	UINT8 *charmap = memory_region(REGION_GFX1) + (gpa & 1) * 0x400;
	UINT8 pixels = charmap[(linecount & 7) + (charcode << 3)] ^ 0xff;
	if (vsp) {
		pixels = 0;
	}
	if (lten) {
		pixels = 0xff;
	}
	if (rvv) {
		pixels ^= 0xff;
	}
	for(i=0;i<6;i++) {
		*BITMAP_ADDR16(bitmap, y, x + i) = (pixels >> (5-i)) & 1 ? (hlgt ? 2 : 1) : 0;
	}
}

VIDEO_UPDATE( apogee )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, I8275, "i8275");
	i8275_update( devconf, bitmap, cliprect);
	VIDEO_UPDATE_CALL ( generic_bitmapped );
	return 0;
}

const rgb_t apogee_palette[3] = {
	MAKE_RGB(0x00, 0x00, 0x00), // black
	MAKE_RGB(0xa0, 0xa0, 0xa0), // white
	MAKE_RGB(0xff, 0xff, 0xff)	// highlight
};

PALETTE_INIT( apogee )
{
	palette_set_colors(machine, 0, apogee_palette, ARRAY_LENGTH(apogee_palette));
}

