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

#include "driver.h"
#include "machine/6883sam.h"
#include "video/m6847.h"
#include "includes/coco.h"


/* TODO: this is really hacky, needs an updated/rewritten sam emulation
 * to work correctly.
 */

static int last_scanline = -1;
const UINT8 *coco_videoram = 0;

READ8_DEVICE_HANDLER( coco_mc6847_videoram_r )
{
	coco_state *state = device->machine->driver_data;
	int scanline;

	scanline = mc6847_get_scanline(device);

	if (last_scanline != scanline)
	{
		coco_videoram = sam_m6847_get_video_ram(state->sam, scanline);
		last_scanline = scanline;
	}

	offset = offset % 32;

	mc6847_inv_w(device, BIT(coco_videoram[offset], 6));
	mc6847_as_w(device, BIT(coco_videoram[offset], 7));

	return coco_videoram[offset];
}

VIDEO_UPDATE( coco )
{
	const device_config *mc6847 = devtag_get_device(screen->machine, "mc6847");
	return mc6847_update(mc6847, bitmap, cliprect);
}
