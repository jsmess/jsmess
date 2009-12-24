/*********************************************************************

    formats/nes_dsk.c

    NES disk images

*********************************************************************/


#include <string.h>

#include "formats/nes_dsk.h"
#include "formats/basicdsk.h"



/*****************************************************************************
 NES floppy core functions
*****************************************************************************/


static FLOPPY_IDENTIFY( nes_dsk_identify )
{
	UINT64 size;
	UINT8 header[3];

	*vote = 100;
	
	/* get first 3 bytes */
	floppy_image_read(floppy, &header, 0, sizeof(header));

	/* first check the size of the image */
	size = floppy_image_size(floppy);
		
	if ((size != 65516) && (size != 131016) && (size != 262016))
		*vote = 0;

	/* then check the first sector for the magic string */
	if (memcmp(header, "FDS", 3))
		*vote = 0;

	return FLOPPY_ERROR_SUCCESS;
}


static FLOPPY_CONSTRUCT( nes_dsk_construct )
{
	return FLOPPY_ERROR_SUCCESS;
}


/*****************************************************************************
 NES floppy options
*****************************************************************************/

FLOPPY_OPTIONS_START( nes_only )
	FLOPPY_OPTION(
		fds_dsk,
		"fds",
		"NES floppy disk image",
		nes_dsk_identify,
		nes_dsk_construct,
		NULL
	)
		{ NULL }
};
