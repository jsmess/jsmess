#include <string.h>
#include "formats/trs_dsk.h"
#include "formats/basicdsk.h"
#include "formats/coco_dsk.h"

/* ----------------------------------------------------------------------- 
 * JV1 (Jeff Vavasour 1) format                                         
 *                                                                         
 * Used by Jeff Vavasour's TRS-80 Emulators                                  
 *
 * Very straight basic disk; 1 head, 10 sectors, 256 sector length
 * ----------------------------------------------------------------------- */

#define TRS80_JV1_HEADS				1
#define TRS80_JV1_SECTORS			10
#define TRS80_JV1_SECTORLENGTH		256
#define TRS80_JV1_FIRSTSECTORID		0

static floperr_t trs80_jv1_identify(floppy_image *floppy, int *vote)
{
	UINT64 size;
	size = floppy_image_size(floppy);
	*vote = (size % (TRS80_JV1_HEADS * TRS80_JV1_SECTORS * TRS80_JV1_SECTORLENGTH))
		? 0 : 100;
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t trs80_jv1_construct(floppy_image *floppy, option_resolution *params)
{
	struct basicdsk_geometry geometry;

	memset(&geometry, 0, sizeof(geometry));
	geometry.heads				= TRS80_JV1_HEADS;
	geometry.sectors			= TRS80_JV1_SECTORS;
	geometry.first_sector_id	= TRS80_JV1_FIRSTSECTORID;
	geometry.sector_length		= TRS80_JV1_SECTORLENGTH;

	if (params)
	{
		/* create */
		geometry.tracks = option_resolution_lookup_int(params, PARAM_TRACKS);
	}
	else
	{
		/* load */
		geometry.tracks = (int) (floppy_image_size(floppy) / geometry.heads
			/ geometry.sectors / geometry.sector_length);
	}
	return basicdsk_construct(floppy, &geometry);
}



/* ----------------------------------------------------------------------- */

FLOPPY_OPTIONS_START( trs80 )
	FLOPPY_OPTION( trs80_jv1, "dsk\0",			"TRS-80 JV1 disk image",	trs80_jv1_identify,	trs80_jv1_construct,
		TRACKS([35]-255))
	FLOPPY_OPTION( trs80_dmk, "dsk\0dmk\0",		"TRS-80 DMK disk image",	coco_dmk_identify,	coco_dmk_construct,
		HEADS([1]-2)
		TRACKS([35]-255)
		SECTORS(1-[10]-18)
		SECTOR_LENGTH(128/[256]/512/1024/2048/4096/8192)
		INTERLEAVE(0-[6]-17)
		FIRST_SECTOR_ID([0]-1))
FLOPPY_OPTIONS_END
