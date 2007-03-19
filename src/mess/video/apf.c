/******************************************************************************

******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "video/m6847.h"
#include "includes/apf.h"

UINT8 *apf_video_ram;
UINT8 apf_m6847_attr;

static ATTR_CONST UINT8 apf_get_attributes(UINT8 c)
{
	/* this seems to be the same so far, as it gives the same result as vapf */
	UINT8 result = apf_m6847_attr;
	if (c & 0x40)	result |= M6847_INV;
	if (c & 0x80)	result |= M6847_AS;
	return result;
}



static void apf_vsync_int(int line)
{
	extern unsigned int apf_ints;
	if (line)
		apf_ints |= 0x10;
	else
		apf_ints &= ~0x10;
	apf_update_ints();
}



static const UINT8 *apf_get_video_ram(int scanline)
{
	return &apf_video_ram[(scanline / 12) * 0x20 + 0x200];
}



VIDEO_START(apf)
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_NTSC;
	cfg.field_sync_callback = apf_vsync_int;
	cfg.get_attributes = apf_get_attributes;
	cfg.get_video_ram = apf_get_video_ram;
	m6847_init(&cfg);
	return 0;
}
