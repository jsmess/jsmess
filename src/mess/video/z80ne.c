/***************************************************************************

	Video hardware for Z80NE

	driver by Roberto Lavarone

	See mess/machine/z80ne.c for references

***************************************************************************/

#include "driver.h"
#include "video/m6847.h"
#include "includes/z80ne.h"


/*
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
	m6847_init(machine, &cfg);
}
