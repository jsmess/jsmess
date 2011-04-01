/***************************************************************************

        Radio-86RK video driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "video/i8275.h"
#include "includes/radio86.h"

I8275_DISPLAY_PIXELS(radio86_display_pixels)
{
	int i;
	bitmap_t *bitmap = device->machine().generic.tmpbitmap;
	UINT8 *charmap = device->machine().region("gfx1")->base();
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


I8275_DISPLAY_PIXELS(mikrosha_display_pixels)
{
	radio86_state *state = device->machine().driver_data<radio86_state>();
	int i;
	bitmap_t *bitmap = device->machine().generic.tmpbitmap;
	UINT8 *charmap = device->machine().region("gfx1")->base() + (state->m_mikrosha_font_page & 1) * 0x400;
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

I8275_DISPLAY_PIXELS(apogee_display_pixels)
{
	int i;
	bitmap_t *bitmap = device->machine().generic.tmpbitmap;
	UINT8 *charmap = device->machine().region("gfx1")->base() + (gpa & 1) * 0x400;
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

I8275_DISPLAY_PIXELS(partner_display_pixels)
{
	int i;
	bitmap_t *bitmap = device->machine().generic.tmpbitmap;
	UINT8 *charmap = device->machine().region("gfx1")->base() + 0x400 * (gpa * 2 + hlgt);
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
		*BITMAP_ADDR16(bitmap, y, x + i) = (pixels >> (5-i)) & 1;
	}
}

SCREEN_UPDATE( radio86 )
{
	device_t *devconf = screen->machine().device("i8275");
	i8275_update( devconf, bitmap, cliprect);
	SCREEN_UPDATE_CALL ( generic_bitmapped );
	return 0;
}

static const rgb_t radio86_palette[3] = {
	MAKE_RGB(0x00, 0x00, 0x00), // black
	MAKE_RGB(0xa0, 0xa0, 0xa0), // white
	MAKE_RGB(0xff, 0xff, 0xff)	// highlight
};

PALETTE_INIT( radio86 )
{
	palette_set_colors(machine, 0, radio86_palette, ARRAY_LENGTH(radio86_palette));
}

