/***************************************************************************

        Radio-86RK video driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "video/i8275.h"

I8275_DISPLAY_PIXELS(radio86_display_pixels)
{
	//int x, int y, UINT8 linecount, UINT8 charcode, UINT8 lineattr, UINT8 lten, UINT8 rvv, UINT8 vsp, UINT8 gpa, UINT8 hlgt);
	int i;
	bitmap_t *bitmap = tmpbitmap;
	UINT8 *charmap = memory_region(REGION_GFX1);
	UINT8 pixels = charmap[(linecount & 7) + (charcode << 3)] ^ 0xff;
	if (vsp) {
		pixels = 0;
	}
	if (lten) {
		pixels = 0xff;
	}
	for(i=0;i<6;i++) {
		*BITMAP_ADDR16(bitmap, y, x + i) = (pixels >> (5-i)) & 1;
	}
}

VIDEO_UPDATE( radio86 )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, I8275, "i8275");
	i8275_update( devconf, bitmap, cliprect);
	VIDEO_UPDATE_CALL ( generic_bitmapped );
	return 0;
}

