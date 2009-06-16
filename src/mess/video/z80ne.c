/***************************************************************************

	Video hardware for Z80NE

	driver by Roberto Lavarone

	See mess/machine/z80ne.c for references

***************************************************************************/

/* MC6847 pin connections:
 * GM0 to GND
 * GM1 to GND
 * GM2 to GND
 * AG  to GND
 * CSS to GND
 * DD6 to GND
 * DD7 to GND
 *
 * D7 D6 PIN32 PIN31 PIN34
 *        INV  INTEXT AS
 * 0  0    0     0     0   Alphanumeric
 * 0  1    0     0     0   Alphanumeric
 * 1  0    0     1     1   Semigraphic 6
 * 1  1    1     0     0   Inverted Alphanumeric
 */


#include "driver.h"
#include "video/m6847.h"
#include "includes/z80ne.h"

#define M6847_RGB(r,g,b)	((r << 16) | (g << 8) | (b << 0))

static const UINT32 lx388palette[] =
{
	M6847_RGB(0x00, 0xff, 0x00),	/* GREEN */
	M6847_RGB(0x00, 0xff, 0x00),	/* YELLOW in original, here GREEN */
	M6847_RGB(0x00, 0x00, 0xff),	/* BLUE */
	M6847_RGB(0xff, 0x00, 0x00),	/* RED */
	M6847_RGB(0xff, 0xff, 0xff),	/* BUFF */
	M6847_RGB(0x00, 0xff, 0xff),	/* CYAN */
	M6847_RGB(0xff, 0x00, 0xff),	/* MAGENTA */
	M6847_RGB(0xff, 0x80, 0x00),	/* ORANGE */

	M6847_RGB(0x00, 0x20, 0x00),	/* BLACK in original, here DARK green */
	M6847_RGB(0x00, 0xff, 0x00),	/* GREEN */
	M6847_RGB(0x00, 0x00, 0x00),	/* BLACK */
	M6847_RGB(0xff, 0xff, 0xff),	/* BUFF */

	M6847_RGB(0x00, 0x20, 0x00),	/* ALPHANUMERIC DARK GREEN */
	M6847_RGB(0x00, 0xff, 0x00),	/* ALPHANUMERIC BRIGHT GREEN */
	M6847_RGB(0x40, 0x10, 0x00),	/* ALPHANUMERIC DARK ORANGE */
	M6847_RGB(0xff, 0xc4, 0x18)		/* ALPHANUMERIC BRIGHT ORANGE */
};

static ATTR_CONST UINT8 lx388_get_attributes(running_machine *machine, UINT8 c, int scanline, int pos)
{
	UINT8 result = 0x00;
	if (c & 0x80)
	{
		if (c & 0x40)
			result |= M6847_INV;
		else
			result |= M6847_INTEXT | M6847_AS;
	}
	return result;
}

static const UINT8 *lx388_get_video_ram(running_machine *machine, int scanline)
{
	return videoram + (scanline / 12) * 0x20;
}

VIDEO_START(lx388)
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_PAL;
	cfg.get_attributes = lx388_get_attributes;
	cfg.get_video_ram = lx388_get_video_ram;
	cfg.custom_palette = lx388palette;
	m6847_init(machine, &cfg);
}
