/***************************************************************************

        Mikrosha video driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "video/i8275.h"
#include "includes/mikrosha.h"

UINT8 mikrosha_font_page;

I8275_DISPLAY_PIXELS(mikrosha_display_pixels)
{
	int i;
	bitmap_t *bitmap = tmpbitmap;
	UINT8 *charmap = memory_region(device->machine, REGION_GFX1) + (mikrosha_font_page & 1) * 0x400;
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

VIDEO_UPDATE( mikrosha )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, I8275, "i8275");
	i8275_update( devconf, bitmap, cliprect);
	VIDEO_UPDATE_CALL ( generic_bitmapped );
	return 0;
}

static const rgb_t mikrosha_palette[3] = {
	MAKE_RGB(0x00, 0x00, 0x00), // black
	MAKE_RGB(0xa0, 0xa0, 0xa0), // white
	MAKE_RGB(0xff, 0xff, 0xff)	// highlight
};

PALETTE_INIT( mikrosha )
{
	palette_set_colors(machine, 0, mikrosha_palette, ARRAY_LENGTH(mikrosha_palette));
}

