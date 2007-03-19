/******************************************************************************

atom.c

******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "video/m6847.h"
#include "includes/atom.h"

static ATTR_CONST UINT8 atom_get_attributes(UINT8 c)
{
	extern UINT8 atom_8255_porta;
	extern UINT8 atom_8255_portc;
	UINT8 result = 0x00;
	if (c & 0x40)				result |= M6847_AS | M6847_INTEXT;
	if (c & 0x80)				result |= M6847_INV;
	if (atom_8255_porta & 0x80)	result |= M6847_GM2;
	if (atom_8255_porta & 0x40)	result |= M6847_GM1;
	if (atom_8255_porta & 0x20)	result |= M6847_GM0;
	if (atom_8255_porta & 0x10)	result |= M6847_AG;
	if (atom_8255_portc & 0x08)	result |= M6847_CSS;
	return result;
}

static const UINT8 *atom_get_video_ram(int scanline)
{
	return videoram + (scanline / 12) * 0x20;
}

VIDEO_START( atom )
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_PAL;
	cfg.get_attributes = atom_get_attributes;
	cfg.get_video_ram = atom_get_video_ram;
	m6847_init(&cfg);

	return 0;
}

