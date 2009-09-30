/*********************************************************************

    formats/atarist_dsk.c

    Atari ST disk images

*********************************************************************/

#include <string.h>

#include "formats/atarist_dsk.h"
#include "formats/basicdsk.h"

static FLOPPY_IDENTIFY(atarist_dsk_identify)
{
	*vote = 100;
	return FLOPPY_ERROR_SUCCESS;
}

static FLOPPY_CONSTRUCT(atarist_dsk_construct)
{
	struct basicdsk_geometry geometry;
	int heads = 0;
	int tracks = 0;
	int sectors = 0;
	UINT8 bootsector[512];

	floppy_image_read(floppy, bootsector, 0, 512);
	sectors = bootsector[0x18];
	heads = bootsector[0x1a];
	tracks = (bootsector[0x13] | (bootsector[0x14] << 8)) / sectors / heads;

	memset(&geometry, 0, sizeof(geometry));
	geometry.heads = heads;
	geometry.first_sector_id = 1;
	geometry.sector_length = 512;
	geometry.tracks = tracks;
	geometry.sectors = sectors;
	return basicdsk_construct(floppy, &geometry);
}

/* ----------------------------------------------------------------------- */

FLOPPY_OPTIONS_START( atarist )
	FLOPPY_OPTION( atarist, "st",		"Atari ST floppy disk image",	atarist_dsk_identify, atarist_dsk_construct, NULL)
FLOPPY_OPTIONS_END

