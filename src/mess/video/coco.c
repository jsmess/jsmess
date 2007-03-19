/***************************************************************************

	Video hardware for CoCo/Dragon family

	driver by Nathan Woods

	See mess/machine/coco.c for references
 
	TODO: Determine how the CoCo 2B (which used the M6847T1 was hooked up
	to the M6847T1 chip to generate its text video modes.  My best guess is as
	follows:

		GM0 would enable lowercase if INV is off, and force INV on by default
		GM1 would toggle INV
		GM2 enables an alternate border

***************************************************************************/

#include <assert.h>
#include <math.h>

#include "driver.h"
#include "machine/6821pia.h"
#include "machine/6883sam.h"
#include "video/m6847.h"
#include "video/generic.h"
#include "includes/coco.h"


/*************************************
 *
 *	Code
 *
 *************************************/

ATTR_CONST UINT8 coco_get_attributes(UINT8 c)
{
	UINT8 result = 0x00;
	UINT8 pia1_pb = pia_get_output_b(1);

	if (c & 0x40)		result |= M6847_INV;
	if (c & 0x80)		result |= M6847_AS;
	if (pia1_pb & 0x08)	result |= M6847_CSS;
	if (pia1_pb & 0x10)	result |= M6847_GM0 | M6847_INTEXT;
	if (pia1_pb & 0x20)	result |= M6847_GM1;
	if (pia1_pb & 0x40)	result |= M6847_GM2;
	if (pia1_pb & 0x80)	result |= M6847_AG;
	return result;
}



static void coco_horizontal_sync_callback(int data)
{
	pia_0_ca1_w(0, data);
}



static void coco_field_sync_callback(int data)
{
	pia_0_cb1_w(0, data);
}



static void internal_video_start_coco(m6847_type type)
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = type;
	
	/* NPW 14-May-2006 - Ugly hack; using CPU timing factor seems to break some
	 * Dragon games */
	if (Machine->gamedrv->name[0] == 'c')
		cfg.cpu0_timing_factor = 4;

	cfg.get_attributes = coco_get_attributes;
	cfg.get_video_ram = sam_m6847_get_video_ram;
	cfg.horizontal_sync_callback = coco_horizontal_sync_callback;
	cfg.field_sync_callback = coco_field_sync_callback;

	m6847_init(&cfg);
}



VIDEO_START( dragon )
{
	internal_video_start_coco(M6847_VERSION_ORIGINAL_PAL);
	return 0;
}

VIDEO_START( coco )
{
	internal_video_start_coco(M6847_VERSION_ORIGINAL_NTSC);
	return 0;
}

VIDEO_START( coco2b )
{
	internal_video_start_coco(M6847_VERSION_M6847T1_NTSC);
	return 0;
}

