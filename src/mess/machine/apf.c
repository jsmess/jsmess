/***********************************************************************

	apf.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

***********************************************************************/

#include "driver.h"
#include "includes/apf.h"
#include "devices/cassette.h"
#include "devices/basicdsk.h"
#include "image.h"

/* 256 bytes per sector, single sided, single density, 40 track  */
DEVICE_LOAD( apfimag_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		basicdsk_set_geometry(image, 40, 1, 8, 256, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}
