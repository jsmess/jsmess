/*********************************************************************

	formats/adam_dsk.c

	Coleco Adam disk images

*********************************************************************/

#include <string.h>

#include "formats/adam_dsk.h"
#include "formats/basicdsk.h"


static FLOPPY_IDENTIFY(adam_dsk_identify)
{
	*vote = (floppy_image_size(floppy) == (40*8*512)) ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;
}



static FLOPPY_CONSTRUCT(adam_dsk_construct)
{
	struct basicdsk_geometry geometry;
	memset(&geometry, 0, sizeof(geometry));
	geometry.heads = 1;
	geometry.first_sector_id = 0;
	geometry.sector_length = 512;
	geometry.tracks = 40;
	geometry.sectors = 8;
	return basicdsk_construct(floppy, &geometry);
}



/* ----------------------------------------------------------------------- */

FLOPPY_OPTIONS_START( adam )
	FLOPPY_OPTION( adam_dsk, "dsk\0",		"Adam floppy disk image",	adam_dsk_identify, adam_dsk_construct, NULL)
FLOPPY_OPTIONS_END

