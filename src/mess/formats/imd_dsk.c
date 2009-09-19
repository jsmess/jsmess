/*********************************************************************

	formats/imd_dsk.c

	IMD disk images

*********************************************************************/

#include <string.h>
#include "driver.h"
#include "formats/flopimg.h"
#include "devices/flopdrv.h"

#define IMD_DSK_TAG	"imdtag"

struct imddsk_tag
{
	int heads;
	int tracks;
	int sector_size;
	UINT64 track_offsets[84*2]; /* offset within data for each track */
};


static struct imddsk_tag *get_tag(floppy_image *floppy)
{
	struct imddsk_tag *tag;
	tag = floppy_tag(floppy, IMD_DSK_TAG);
	return tag;
}



FLOPPY_IDENTIFY( imd_dsk_identify )
{
	UINT8 header[3];

	floppy_image_read(floppy, header, 0, 3);
	if (header[0]=='I' && header[1]=='M' && header[2]=='D') {
		*vote = 100;
	} else {
		*vote = 0;
	}
	return FLOPPY_ERROR_SUCCESS;
}

static int imd_get_heads_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->heads;
}

static int imd_get_tracks_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->tracks;
}

/*static UINT64 imd_get_track_offset(floppy_image *floppy, int head, int track)
{
	return get_tag(floppy)->track_offsets[(track<<1) + head];
}
*/
static floperr_t get_offset(floppy_image *floppy, int head, int track, int sector, int sector_is_index, UINT64 *offset)
{
	UINT64 offs;
	UINT8 sectors_per_track = 0;	

	if ((head < 0) || (head >= get_tag(floppy)->heads) || (track < 0) || (track >= get_tag(floppy)->tracks)
			|| (sector < 0) )
		return FLOPPY_ERROR_SEEKERROR;

	
	if (!sector_is_index) {
		// when taking ID's return seek error if number is over counter
		if (sector > sectors_per_track) {
			return FLOPPY_ERROR_SEEKERROR;
		}
	}
	
	if (offset)
		*offset = offs;
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t internal_imd_read_sector(floppy_image *floppy, int head, int track, int sector, int sector_is_index, void *buffer, size_t buflen)
{
	UINT64 offset;
	floperr_t err;
	
	// take sector offset
	err = get_offset(floppy, head, track, sector, sector_is_index, &offset);
	if (err)
		return err;
		
	return FLOPPY_ERROR_SUCCESS;
}


static floperr_t imd_read_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return internal_imd_read_sector(floppy, head, track, sector, FALSE, buffer, buflen);
}

static floperr_t imd_read_indexed_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return internal_imd_read_sector(floppy, head, track, sector, TRUE, buffer, buflen);
}

static floperr_t imd_get_sector_length(floppy_image *floppy, int head, int track, int sector, UINT32 *sector_length)
{
	floperr_t err;
	err = get_offset(floppy, head, track, sector, FALSE, NULL);
	if (err)
		return err;

	if (sector_length) {		
		*sector_length = get_tag(floppy)->sector_size;
	}
	return FLOPPY_ERROR_SUCCESS;
}

static floperr_t imd_get_indexed_sector_info(floppy_image *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length, unsigned long *flags)
{
	floperr_t retVal;
	UINT64 offset;

	retVal = get_offset(floppy, head, track, sector_index, FALSE, &offset);

	if (cylinder)
		*cylinder = 0;
	if (side)
		*side = 0;
	if (sector)
		*sector = 0;
	if (sector_length) {
		*sector_length = 0;
	}
	if (flags)
		*flags = 0;
	return retVal;
}


FLOPPY_CONSTRUCT( imd_dsk_construct )
{
	struct FloppyCallbacks *callbacks;
	
	if(params)
	{
		// create
		return FLOPPY_ERROR_UNSUPPORTED;
	}

	callbacks = floppy_callbacks(floppy);
	callbacks->read_sector = imd_read_sector;
	callbacks->read_indexed_sector = imd_read_indexed_sector;
	callbacks->get_sector_length = imd_get_sector_length;
	callbacks->get_heads_per_disk = imd_get_heads_per_disk;
	callbacks->get_tracks_per_disk = imd_get_tracks_per_disk;
	callbacks->get_indexed_sector_info = imd_get_indexed_sector_info;

	return FLOPPY_ERROR_UNSUPPORTED;
}

