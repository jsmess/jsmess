/*************************************************************************

    formats/coupedsk.c

    SAM Coupe disk image formats

**************************************************************************/

#include "coupedsk.h"
#include "formats/basicdsk.h"


#define SAD_HEADER_LEN  22



/*************************************
 *
 *  MGT disk image format
 *
 *************************************/

/*
    Straight rip without any header
    
    Data:
    
    Side 0 Track 0
    Side 1 Track 0
    Side 0 Track 1
    Side 1 Track 1
    ...
*/


FLOPPY_CONSTRUCT( coupe_mgt_construct )
{
	struct basicdsk_geometry geometry;

	memset(&geometry, 0, sizeof(geometry));
	geometry.heads = 2;
	geometry.tracks = 80;
	geometry.sector_length = 512;
	geometry.first_sector_id = 1;

	if (params)
	{
		/* create */
		geometry.sectors = option_resolution_lookup_int(params, PARAM_SECTORS);
	}
	else
	{
		/* load */
		UINT64 size = floppy_image_size(floppy);

		/* verify size */
		if (size != 737280 && size != 819200)
			return FLOPPY_ERROR_INVALIDIMAGE;

		geometry.sectors = size / 81920;
		
	}

	return basicdsk_construct(floppy, &geometry);
}


FLOPPY_IDENTIFY( coupe_mgt_identify )
{
	UINT64 size;

	size = floppy_image_size(floppy);

	*vote = (size == 737280 || size == 819200) ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;	
}



/*************************************
 *
 *  SAD disk image format
 *
 *************************************/

/*
    Header (22 bytes):
    
    0-17 "Aley's disk backup"
    18   number of sides
    19   number of tracks
    20   number of sectors per track
    21   sector size divided by 64
    
    Data:
    
    Side 0 Track 0
    Side 0 Track 1
    Side 0 Track 2
    ...
    Side 1 Track 0
    Side 1 Track 1
    ...  
*/


static UINT64 coupe_sad_translate_offset(floppy_image *floppy,
	const struct basicdsk_geometry *geom, int track, int head, int sector)
{
	return head * geom->tracks * geom->sectors + geom->sectors * track + sector;
}


static void coupe_sad_interpret_header(floppy_image *floppy,
	int *heads, int *tracks, int *sectors, int *sector_size)
{
	UINT8 header[SAD_HEADER_LEN];

	floppy_image_read(floppy, header, 0, SAD_HEADER_LEN);
	
	*heads = header[18];
	*tracks = header[19];
	*sectors = header[20];
	*sector_size = header[21] << 6;
}


FLOPPY_CONSTRUCT( coupe_sad_construct )
{
	struct basicdsk_geometry geometry;
	int heads, tracks, sectors, sector_length;

	memset(&geometry, 0, sizeof(geometry));

	if (params)
	{
		/* create */
		heads = option_resolution_lookup_int(params, PARAM_HEADS);
		tracks = option_resolution_lookup_int(params, PARAM_TRACKS);
		sectors = option_resolution_lookup_int(params, PARAM_SECTORS);
		sector_length = option_resolution_lookup_int(params, PARAM_SECTOR_LENGTH);
	}
	else
	{
		/* load */
		coupe_sad_interpret_header(floppy, &heads, &tracks, &sectors, &sector_length);
	}

	/* fill in the data */
	geometry.offset = SAD_HEADER_LEN;
	geometry.heads = heads;
	geometry.tracks = tracks;
	geometry.sectors = sectors;
	geometry.first_sector_id = 1;
	geometry.sector_length = sector_length;
	geometry.translate_offset = coupe_sad_translate_offset;

	return basicdsk_construct(floppy, &geometry);
}


FLOPPY_IDENTIFY( coupe_sad_identify )
{
	int heads, tracks, sectors, sector_size;
	UINT64 size, calculated_size;

	size = floppy_image_size(floppy);
	
	/* read values from SAD header */
	coupe_sad_interpret_header(floppy, &heads, &tracks, &sectors, &sector_size);

	/* calculate expected disk image size */
	calculated_size = SAD_HEADER_LEN + heads * tracks * sectors * sector_size;

	*vote = (size == calculated_size) ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;
}



/*************************************
 *
 *  SDF disk image format
 *
 *************************************/

FLOPPY_CONSTRUCT( coupe_sdf_construct )
{
	return FLOPPY_ERROR_INVALIDIMAGE;
}


FLOPPY_IDENTIFY( coupe_sdf_identify )
{
	*vote = 0;
	return FLOPPY_ERROR_SUCCESS;
}



/*************************************
 *
 *  Floppy options
 *
 *************************************/

FLOPPY_OPTIONS_START(coupe)
	FLOPPY_OPTION
	(
		coupe_mgt, "mgt,dsk,sad", "SAM Coupe MGT disk image", coupe_mgt_identify, coupe_mgt_construct,
		HEADS([2])
		TRACKS([80])
		SECTORS(9-[10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1])
	)
	FLOPPY_OPTION
	(
		coupe_sad, "sad", "SAM Coupe SAD disk image", coupe_sad_identify, coupe_sad_construct,
		HEADS(1-[2]-255)
		TRACKS(1-[80]-255)
		SECTORS(1-[10]-255)
		SECTOR_LENGTH(64/128/256/[512]/1024/2048/4096)
		FIRST_SECTOR_ID([1])
	)
	FLOPPY_OPTION
	(
		coupe_sdf, "sdf", "SAM Coupe SDF disk image", coupe_sdf_identify, coupe_sdf_construct,
		HEADS([2])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1])
	)
FLOPPY_OPTIONS_END
