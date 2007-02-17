/******************************************************************************

Video Technology Laser 110-310 computers:

    Video Technology Laser 110
      Sanyo Laser 110
    Video Technology Laser 200
      Salora Fellow
      Texet TX-8000
      Video Technology VZ-200
    Video Technology Laser 210
      Dick Smith Electronics VZ-200
      Sanyo Laser 210
    Video Technology Laser 310
      Dick Smith Electronics VZ-300

Video hardware:

	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	- Guy Thomason
	- Jason Oakley
	- Bushy Maunder
	- and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors.

******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/vtech1.h"
#include "video/m6847.h"


/******************************************************************************
 Palette Initialisation
******************************************************************************/

UINT32 vtech1_palette_mono[16];
UINT32 vtech1_palette_color[16];

/* note - Juergen's colors do not match the colors in the m6847 code */
static UINT8 vtech1_palette[] =
{
      0, 224,   0,    /* green */
    208, 255,   0,    /* yellow (greenish) */
      0,   0, 255,    /* blue */
    255,   0,   0,    /* red */
    224, 224, 144,    /* buff */
      0, 255, 160,    /* cyan (greenish) */
    255,   0, 255,    /* magenta */
    240, 112,   0,    /* orange */
      0,   0,   0,    /* black (block graphics) */
      0, 224,   0,    /* green */
      0,   0,   0,    /* black (block graphics) */
    224, 224, 144,    /* buff */
      0,  64,   0,    /* dark green (alphanumeric characters) */
      0, 224,  24,    /* bright green (alphanumeric characters) */
     64,  16,   0,    /* dark orange (alphanumeric characters) */
    255, 196,  24,    /* bright orange (alphanumeric characters) */
};


static ATTR_CONST UINT8 vtech1_get_attributes(UINT8 c)
{
	UINT8 result = 0x00;
	if (c & 0x40)				result |= M6847_INV;
	if (c & 0x80)				result |= M6847_AS;
	if (vtech1_latch & 0x10)	result |= M6847_CSS;
	if (vtech1_latch & 0x08)	result |= M6847_AG | M6847_GM1;
	return result;
}


static const UINT8 *vtech1_get_video_ram(int scanline)
{
	return videoram + (scanline / (vtech1_latch & 0x08 ? 3 : 12)) * 0x20;
}


VIDEO_START( vtech1m )
{
	m6847_config cfg;
	int i;

	/* TODO: Monochrome should be moved into M6847 code */
	for (i = 0; i < sizeof(vtech1_palette_mono); i++)
	{
        int mono;
        mono  = (int)(vtech1_palette[i*3+0] * 0.299);  /* red */
        mono += (int)(vtech1_palette[i*3+1] * 0.587);  /* green */
        mono += (int)(vtech1_palette[i*3+2] * 0.114);  /* blue */
        vtech1_palette_mono[i] = (mono << 16) + (mono << 8) + mono;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_PAL;
	cfg.get_attributes = vtech1_get_attributes;
	cfg.get_video_ram = vtech1_get_video_ram;
	cfg.custom_palette = vtech1_palette_mono;
	m6847_init(&cfg);
	return 0;
}


VIDEO_START( vtech1 )
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_PAL;
	cfg.get_attributes = vtech1_get_attributes;
	cfg.get_video_ram = vtech1_get_video_ram;
	m6847_init(&cfg);
	return 0;
}
