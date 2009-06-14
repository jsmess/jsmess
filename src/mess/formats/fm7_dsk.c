/*
 *   Fujitsu FM-7 series D77 disk images
 * 
 * 
 *   Header (total size = 0x2b0 bytes):
 *     0x00 - Disk label
 *     0x1a - Write protect (bit 3)
 *     0x1b - 2D/2DD format (bit 3)
 *     0x1c-1f - image size (should match file size)
 *     0x20 - offsets for each track (max 164)
 *
 *   Sectors (0x110 bytes each, typically)
 *     0x00 - Sector info
 * 			byte 0 - track number
 * 			byte 1 - side (0 or 1)
 * 			byte 2 - sector number
 *     0x10 - sector data
 * 
 *   Images can be concatenated together.
 *   Sectors can be in any order.
 *   Tracks are in the order:
 * 			Track 0 side 0
 * 			Track 0 side 1
 * 			Track 1 side 0
 * 			...
 * 
 * 
 */

#include "driver.h"
#include "formats/fm7_dsk.h"

#define D77_HEADER_LEN 0x2b0

struct d77_tag
{
	UINT32 image_size;
	UINT32 trackoffset[164];
	UINT8 write_protect;
	UINT8 disk_type;
};

static struct d77_tag *get_d77_tag(floppy_image *floppy)
{
	return floppy_tag(floppy, "d77tag");
}

static int d77_get_tracks_per_disk(floppy_image *floppy)
{
	return 82;  // 82 tracks per side
}

static int d77_get_heads_per_disk(floppy_image *floppy)
{
	return 2;  // 2 heads
}

static floperr_t d77_get_sector_length(floppy_image *floppy, int head, int track, int sector, UINT32 *sector_length)
{
	*sector_length = 256;  // 256 byte sectors
	return FLOPPY_ERROR_SUCCESS;
}

static floperr_t d77_read_track(floppy_image *floppy, int head, int track, UINT64 offset, void *buffer, size_t buflen)
{
//	floperr_t err;
	
	return FLOPPY_ERROR_UNSUPPORTED;
}

static UINT32 d77_get_sector_offset(floppy_image* floppy, int head, int track, int sector)
{
	struct d77_tag* tag = get_d77_tag(floppy);
	UINT32 offset = 0;
	
	// get offset of the beginning of the track
	offset = tag->trackoffset[(track*2)+head];
	offset += (272 * sector);
	
	if(offset > tag->image_size)
		return 0;

	logerror("d77_get_sector_offset - track %i, side %i, returns %08x\n",track,head,offset+16);
	return offset + 16;	
}

static floperr_t d77_get_indexed_sector_info(floppy_image *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length)
{
	struct d77_tag* tag = get_d77_tag(floppy);
	UINT32 offset;
	UINT8 sector_hdr[16];
	
	offset = tag->trackoffset[(track*2)+head];
	offset += (272 * sector_index);
	
	if(offset > tag->image_size)
		return FLOPPY_ERROR_SEEKERROR;
		
	floppy_image_read(floppy,sector_hdr,offset,16);

	if(sector_index >= sector_hdr[4])
		return FLOPPY_ERROR_SEEKERROR;

	if(sector_length)
		*sector_length = 256;
	if(cylinder)
		*cylinder = sector_hdr[0];
	if(side)
		*side = sector_hdr[1];
	if(sector)
		*sector = sector_hdr[2];
	
	return FLOPPY_ERROR_SUCCESS;
}

static floperr_t d77_read_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	UINT64 offset;
	UINT8 sector_data[256];
	
	offset = d77_get_sector_offset(floppy,head,track,sector);
	if(offset == 0)
		return FLOPPY_ERROR_SEEKERROR;
		
	if(buflen > 256)
		return FLOPPY_ERROR_INTERNAL;
		
	floppy_image_read(floppy,sector_data,offset,256);
	
	memcpy(buffer,sector_data,buflen);
	
	return FLOPPY_ERROR_SUCCESS;
}

static floperr_t d77_read_indexed_sector(floppy_image *floppy, int head, int track, int sector, void *buffer, size_t buffer_len)
{
	return d77_read_sector(floppy,head,track,sector,buffer,buffer_len);
}

static void d77_get_header(floppy_image* floppy,UINT32* size, UINT8* prot, UINT8* type, UINT32* offsets)
{
	UINT8 header[D77_HEADER_LEN];
	int x,s;
	
	floppy_image_read(floppy,header,0,D77_HEADER_LEN);

	if(prot)
		*prot = header[0x1a];
	if(type)
		*prot = header[0x1b];
	if(size)
	{
		s = 0;
		s |= header[0x1f] << 24;
		s |= header[0x1e] << 16;
		s |= header[0x1d] << 8;
		s |= header[0x1c];
		*size = s;
	}
	if(offsets)
	{
		for(x=0;x<164;x++)
		{
			s = 0;
			s |= header[0x23 + (x*4)] << 24;
			s |= header[0x22 + (x*4)] << 16;
			s |= header[0x21 + (x*4)] << 8;
			s |= header[0x20 + (x*4)];
			*(offsets+x) = s;
		}
	}
}

FLOPPY_IDENTIFY(fm7_d77_identify)
{
	UINT32 size;
	
	d77_get_header(floppy,&size,NULL,NULL,NULL);	
	
	if(floppy_image_size(floppy) == size)
	{
		*vote = 100;
		return FLOPPY_ERROR_SUCCESS;
	}
	else
	{
		*vote = 0;
		return FLOPPY_ERROR_INVALIDIMAGE;
	}
}

FLOPPY_CONSTRUCT(fm7_d77_construct)
{
	struct FloppyCallbacks *callbacks;
	struct d77_tag *tag;
	UINT32 size;
	UINT8 prot,type = 0;
	UINT32 offs[164];
	int x;
	
	if(params)
	{
		// create
		return FLOPPY_ERROR_UNSUPPORTED;
	}
	else
	{
		// load
		d77_get_header(floppy,&size,&prot,&type,offs);
	}
	
	tag = floppy_create_tag(floppy,"d77tag",sizeof(struct d77_tag));
	if (!tag)
		return FLOPPY_ERROR_OUTOFMEMORY;
	
	tag->write_protect = prot;
	tag->disk_type = type;
	tag->image_size = size;
	for(x=0;x<164;x++)
		tag->trackoffset[x] = offs[x];
		
	callbacks = floppy_callbacks(floppy);
	callbacks->read_track = d77_read_track;
	callbacks->get_heads_per_disk = d77_get_heads_per_disk;
	callbacks->get_tracks_per_disk = d77_get_tracks_per_disk;
	callbacks->get_sector_length = d77_get_sector_length;
	callbacks->read_sector = d77_read_sector;
	callbacks->read_indexed_sector = d77_read_indexed_sector;
	callbacks->get_indexed_sector_info = d77_get_indexed_sector_info;
	
		
	return FLOPPY_ERROR_SUCCESS;
}

FLOPPY_OPTIONS_START( fm7 )
	FLOPPY_OPTION( fm7_d77, "d77",	"XM7 FM-7 Floppy Disk image",	fm7_d77_identify,	fm7_d77_construct,
		HEADS(1-[2])
		TRACKS([82])
		SECTORS(1-[17])
		SECTOR_LENGTH(128/[256])
		FIRST_SECTOR_ID(0-[1]))
FLOPPY_OPTIONS_END


