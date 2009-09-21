/*********************************************************************

	formats/td0_dsk.c

	TD0 disk images

*********************************************************************/

#include <string.h>
#include "driver.h"
#include "formats/flopimg.h"
#include "devices/flopdrv.h"

#define TD0_DSK_TAG	"td0tag"

struct td0dsk_tag
{
	int heads;
	int tracks;
	int sector_size;
	UINT64 track_offsets[84*2]; /* offset within data for each track */
};


static struct td0dsk_tag *get_tag(floppy_image *floppy)
{
	struct td0dsk_tag *tag;
	tag = floppy_tag(floppy, TD0_DSK_TAG);
	return tag;
}



FLOPPY_IDENTIFY( td0_dsk_identify )
{
	UINT8 header[2];

	floppy_image_read(floppy, header, 0, 2);
	if (header[0]=='T' && header[1]=='D') {
		*vote = 100;
	} else {
		*vote = 0;
	}
	return FLOPPY_ERROR_SUCCESS;
}

static int td0_get_heads_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->heads;
}

static int td0_get_tracks_per_disk(floppy_image *floppy)
{
	return get_tag(floppy)->tracks;
}

static UINT64 td0_get_track_offset(floppy_image *floppy, int head, int track)
{
	return get_tag(floppy)->track_offsets[(track<<1) + head];
}

static floperr_t get_offset(floppy_image *floppy, int head, int track, int sector, int sector_is_index, UINT64 *offset)
{
	UINT64 offs;
	UINT8 header[8];
	UINT8 sectors_per_track;	
	int i;

	if ((head < 0) || (head >= get_tag(floppy)->heads) || (track < 0) || (track >= get_tag(floppy)->tracks)
			|| (sector < 0) )
		return FLOPPY_ERROR_SEEKERROR;

	// position on begining of track data
	offs = td0_get_track_offset(floppy, head, track);

	// read track header
	floppy_image_read(floppy, header, offs - 4, 4);
	
	// take number of sectors per track
	sectors_per_track = header[0];
	
	if (!sector_is_index) {
		// when taking ID's return seek error if number is over counter
		if (sector > sectors_per_track) {
			return FLOPPY_ERROR_SEEKERROR;
		}
	}
	
	// move trought sectors
	for(i=0;i < sector-1;i++) {
		floppy_image_read(floppy, header, offs, 6);
		offs+= 6;
		if ((header[4] & 0x30)==0) {
			floppy_image_read(floppy, header+6, offs, 2);
			offs+= 2;		
			offs+= header[6] + (header[7]<<8);				
		}
	}	
	// read size of sector
	floppy_image_read(floppy, header, offs, 6);
	get_tag(floppy)->sector_size = 1 << (header[3] + 7);

	if (offset)
		*offset = offs;
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t internal_td0_read_sector(floppy_image *floppy, int head, int track, int sector, int sector_is_index, void *buffer, size_t buflen)
{
	UINT64 offset;
	floperr_t err;
	UINT8 header[9];
	int size,realsize,i;
	int buff_pos;
	int data_pos;
	UINT8 *data;
	UINT8 *buf;
	
	buf = (UINT8*)buffer;	
	// take sector offset
	err = get_offset(floppy, head, track, sector, sector_is_index, &offset);
	if (err)
		return err;
		
	// read sector header
	floppy_image_read(floppy, header, offset, 6);
	offset+=6;
	// if there is no date just jump out
	if ((header[4] & 0x30)!=0) return FLOPPY_ERROR_SUCCESS;
		
	floppy_image_read(floppy, header+6, offset, 3);
	offset+=3;
	// take data size
	size = header[6] + (header[7]<<8)-1;
	// take real sector size
	realsize =  1 << (header[3] + 7);
	
	data = malloc(size);
	// read sector data
	floppy_image_read(floppy, data, offset, size);
	
	buff_pos = 0;
	data_pos = 0;
	
	switch(header[8]) {
		case 0:	 
				// encoding type 0
				//  - plain data
				memcpy(buffer,data,size);
				break;		
		case 1: 
				// encoding type 1
				//	- 2 bytes size
				//  - 2 bytes of data
				//  data is reapeted specified number of times
				while(buff_pos<realsize) {
					for (i=0;i<data[data_pos]+(data[data_pos+1] << 8);i++) {
						buf[buff_pos] = data[data_pos+2];buff_pos++;
						buf[buff_pos] = data[data_pos+3];buff_pos++;
					}
					data_pos+=4;
				}
				break;
		case 2: 
				// encoding type 2
				//	- if first byte is zero next byte represent size of
				//		plain data after it
				//	- if different then zero when multiply by 2 represent
				//		size of data that should be reapeted next byte times
				while(buff_pos<realsize) {
					if (data[data_pos]==0x00) {
						int size = data[data_pos+1];						
						memcpy(buf+buff_pos,data + data_pos + 2,size);
						data_pos += 2 + size;
						buff_pos += size;
					} else {
						int size   = 2*data[data_pos];
						int repeat = data[data_pos+1];
						data_pos+=2;
												
						for (i=0;i<repeat;i++) {							
							memcpy(buf + buff_pos,data + data_pos,size);
							buff_pos += size;
						}
						data_pos += size;
					}
				}
				break;
		default: 
				free(data);
				return FLOPPY_ERROR_INTERNAL;
				break;
	}
	free(data);
	return FLOPPY_ERROR_SUCCESS;
}


static floperr_t td0_read_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return internal_td0_read_sector(floppy, head, track, sector, FALSE, buffer, buflen);
}

static floperr_t td0_read_indexed_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return internal_td0_read_sector(floppy, head, track, sector, TRUE, buffer, buflen);
}

static floperr_t td0_get_sector_length(floppy_image *floppy, int head, int track, int sector, UINT32 *sector_length)
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

static floperr_t td0_get_indexed_sector_info(floppy_image *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length, unsigned long *flags)
{
	floperr_t retVal;
	UINT64 offset;
	UINT8 sector_info[9];

	retVal = get_offset(floppy, head, track, sector_index, FALSE, &offset);

	floppy_image_read(floppy, sector_info, offset, 9);
	if (cylinder)
		*cylinder = sector_info[0];
	if (side)
		*side = sector_info[1];
	if (sector)
		*sector = sector_info[2];
	if (sector_length) {
		*sector_length = 1 << (sector_info[3] + 7);
	}
	if (flags) {
		*flags = 0;
		if (sector_info[4] & 0x02) *flags |= ID_FLAG_CRC_ERROR_IN_DATA_FIELD;
		if (sector_info[4] & 0x04) *flags |= ID_FLAG_DELETED_DATA;
	}
		
	return retVal;
}


FLOPPY_CONSTRUCT( td0_dsk_construct )
{
	struct FloppyCallbacks *callbacks;
	struct td0dsk_tag *tag;
	UINT8 header[16];
	int number_of_sectors;
	int position;
	int i;
	int track;

	if(params)
	{
		// create
		return FLOPPY_ERROR_UNSUPPORTED;
	}

	floppy_image_read(floppy, header, 0, 16);

	tag = (struct td0dsk_tag *) floppy_create_tag(floppy, TD0_DSK_TAG, sizeof(struct td0dsk_tag));
	if (!tag)
		return FLOPPY_ERROR_OUTOFMEMORY;

	tag->heads   = header[9];
	if (tag->heads > 1) {
		tag->heads  = 2;
	} 
	
	//  header len + comment header + comment len
	position = 12;
	if (header[7] & 0x80) {
		position += 10 + header[14] + (header[15]<<8);
	}
	tag->tracks = 0;
	
	do { 
		// read track header
		floppy_image_read(floppy, header, position, 4);
		track = header[1];
		number_of_sectors = header[0];
		if (number_of_sectors!=0xff){ 
			position+=4;
			tag->track_offsets[(track<<1) + (header[2] & 1)] = position;
			for(i=0;i<number_of_sectors;i++) {
				// read sector header
				floppy_image_read(floppy, header, position, 6);
				position+=6;
				// read sector size
				if ((header[4] & 0x30)==0) {
					// if there is sector data
					floppy_image_read(floppy, header, position, 2);
					position+=2;
					// skip sector data
					position+= header[0] + (header[1]<<8);
				}
			}
			tag->tracks++;
		}		
	} while(number_of_sectors!=0xff);
	tag->tracks++;

	callbacks = floppy_callbacks(floppy);
	callbacks->read_sector = td0_read_sector;
	callbacks->read_indexed_sector = td0_read_indexed_sector;
	callbacks->get_sector_length = td0_get_sector_length;
	callbacks->get_heads_per_disk = td0_get_heads_per_disk;
	callbacks->get_tracks_per_disk = td0_get_tracks_per_disk;
	callbacks->get_indexed_sector_info = td0_get_indexed_sector_info;
	return FLOPPY_ERROR_SUCCESS;
}

