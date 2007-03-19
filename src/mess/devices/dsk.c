/***************************************************************************

  dsk.c

 CPCEMU standard and extended disk image support.
 Used on Amstrad CPC and Spectrum +3 drivers.

 KT - 27/2/00 - Moved Disk Image handling code into this file
							- Fixed a few bugs
							- Cleaned code up a bit
***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "devices/flopdrv.h"
#include "devices/dsk.h"
/* disk image and extended disk image support code */
/* supports up to 84 tracks and 2 sides */

#define dsk_MAX_TRACKS 84
#define dsk_MAX_SIDES	2
#define dsk_NUM_DRIVES 4
#define dsk_SECTORS_PER_TRACK	20

typedef struct
{
	unsigned char *data; /* the whole image data */
	unsigned long track_offsets[dsk_MAX_TRACKS*dsk_MAX_SIDES]; /* offset within data for each track */
	unsigned long sector_offsets[dsk_SECTORS_PER_TRACK]; /* offset within current track for sector data */
	int current_track;		/* current track */
	int disk_image_type;  /* image type: standard or extended */
} dsk_drive;

static void dsk_disk_image_init(dsk_drive *);

static void dsk_seek_callback(mess_image *img, int physical_track);
static void dsk_get_id_callback(mess_image *img, struct chrn_id *id, int id_index, int side);
static int dsk_get_sectors_per_track(mess_image *img, int side);

static void dsk_write_sector_data_from_buffer(mess_image *img, int sector_index, int side, const char *ptr, int length, int ddam);
static void dsk_read_sector_data_into_buffer(mess_image *img, int sector_index, int side, char *ptr, int length);

static floppy_interface dsk_floppy_interface =
{
	dsk_seek_callback,
	dsk_get_sectors_per_track,
	dsk_get_id_callback,
	dsk_read_sector_data_into_buffer,
	dsk_write_sector_data_from_buffer,
	NULL,
	NULL
};

static dsk_drive drives[dsk_NUM_DRIVES]; /* the drives */

static dsk_drive *get_drive(mess_image *img)
{
	return &drives[image_index_in_device(img)];
}

static int dsk_floppy_verify(UINT8 *diskimage_data)
{
	if ( (memcmp(diskimage_data, "MV - CPC", 8)==0) || 	/* standard disk image? */
		 (memcmp(diskimage_data, "EXTENDED", 8)==0))	/* extended disk image? */
	{
		return IMAGE_VERIFY_PASS;
	}
	return IMAGE_VERIFY_FAIL;
}



static int device_init_dsk_floppy(mess_image *image)
{
	return floppy_drive_init(image, NULL);
}



/* load floppy */
static int device_load_dsk_floppy(mess_image *image)
{
	int id = image_index_in_device(image);
	dsk_drive *thedrive = &drives[id];

	/* load disk image */
	thedrive->data = (unsigned char *) image_ptr(image);
	if (thedrive->data)
	{
		dsk_disk_image_init(thedrive); /* initialise dsk */
		floppy_drive_set_disk_image_interface(image, &dsk_floppy_interface);
		if(dsk_floppy_verify(thedrive->data) == IMAGE_VERIFY_PASS)
        	return INIT_PASS;
		else
        	return INIT_PASS;
	}
	return INIT_PASS;
}

static int dsk_save(mess_image *img, unsigned char **ptr)
{
	int datasize;
	unsigned char *data;

	/* get file size */
	datasize = image_length(img);

	if (datasize!=0)
	{
		data = *ptr;
		if (data!=NULL)
		{
			image_fwrite(img, data, datasize);

			/* ok! */
			return 1;
		}
	}

	return 0;
}


static void device_unload_dsk_floppy(mess_image *image)
{
	int id = image_index_in_device(image);
	dsk_drive *thedrive = &drives[id];

	if (thedrive->data)
		dsk_save(image, &thedrive->data);
	thedrive->data = NULL;
}



static void dsk_dsk_init_track_offsets(dsk_drive *thedrive)
{
	int track_offset;
	int i;
	int track_size;
	int tracks, sides;
	int skip, length,offs;
	unsigned char *file_loaded = thedrive->data;


	/* get size of each track from main header. Size of each
	track includes a 0x0100 byte header, and the actual sector data for
	all sectors on the track */
	track_size = file_loaded[0x032] | (file_loaded[0x033]<<8);

	/* main header is 0x0100 in size */
	track_offset = 0x0100;

	sides = file_loaded[0x031];
	tracks = file_loaded[0x030];


	/* single sided? */
	if (sides==1)
	{
		skip = 2;
		length = tracks;
	}
	else
	{
		skip = 1;
		length = tracks*sides;
	}

	offs = 0;
	for (i=0; i<length; i++)
	{
		thedrive->track_offsets[offs] = track_offset;
		track_offset+=track_size;
		offs+=skip;
	}

}

static void dsk_dsk_init_sector_offsets(dsk_drive *thedrive,int track,int side)
{
	int track_offset;

	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = thedrive->track_offsets[(track<<1) + side];

	if (track_offset!=0)
	{
		int spt;
		int sector_offset;
		int sector_size;
		int i;

		unsigned char *track_header;

		track_header= &thedrive->data[track_offset];

		/* sectors per track as specified in nec765 format command */
		/* sectors on this track */
		spt = track_header[0x015];

		sector_size = (1<<(track_header[0x014]+7));

		/* track header is 0x0100 bytes in size */
		sector_offset = 0x0100;

		for (i=0; i<spt; i++)
		{
			thedrive->sector_offsets[i] = sector_offset;
			sector_offset+=sector_size;
		}
	}
}

static void	 dsk_extended_dsk_init_track_offsets(dsk_drive *thedrive)
{
	int track_offset;
	int i;
	int track_size;
	int tracks, sides;
	int offs, skip, length;
	unsigned char *file_loaded = thedrive->data;

	sides = file_loaded[0x031];
	tracks = file_loaded[0x030];

	if (sides==1)
	{
		skip = 2;
		length = tracks;
	}
	else
	{
		skip = 1;
		length = tracks*sides;
	}

	/* main header is 0x0100 in size */
	track_offset = 0x0100;
	offs = 0;
	for (i=0; i<length; i++)
	{
		int track_size_high_byte;

		/* track size is specified as a byte, and is multiplied
		by 256 to get size in bytes. If 0, track doesn't exist and
		is unformatted, otherwise it exists. Track size includes 0x0100
		header */
		track_size_high_byte = file_loaded[0x034 + i];

		if (track_size_high_byte != 0)
		{
			/* formatted track */
			track_size = track_size_high_byte<<8;

			thedrive->track_offsets[offs] = track_offset;
			track_offset+=track_size;
		}

		offs+=skip;
	}
}


static void dsk_extended_dsk_init_sector_offsets(dsk_drive *thedrive,int track,int side)
{
	int track_offset;

	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = thedrive->track_offsets[(track<<1) + side];

	if (track_offset!=0)
	{
		int spt;
		int sector_offset;
		int sector_size;
		int i;
		unsigned char *id_info;
		unsigned char *track_header;

		track_header= &thedrive->data[track_offset];

		/* sectors per track as specified in nec765 format command */
		/* sectors on this track */
		spt = track_header[0x015];

		id_info = track_header + 0x018;

		/* track header is 0x0100 bytes in size */
		sector_offset = 0x0100;

		for (i=0; i<spt; i++)
		{
                        sector_size = id_info[(i<<3) + 6] + (id_info[(i<<3) + 7]<<8);

			thedrive->sector_offsets[i] = sector_offset;
			sector_offset+=sector_size;
		}
	}
}



static void dsk_disk_image_init(dsk_drive *thedrive)
{
	/*-----------------27/02/00 11:26-------------------
	 clear offsets
	--------------------------------------------------*/
	memset(&thedrive->track_offsets[0], 0, dsk_MAX_TRACKS*dsk_MAX_SIDES*sizeof(unsigned long));
	memset(&thedrive->sector_offsets[0], 0, 20*sizeof(unsigned long));

	if (memcmp(thedrive->data,"MV - CPC",8)==0)
	{
		thedrive->disk_image_type = 0;

		/* standard disk image */
		dsk_dsk_init_track_offsets(thedrive);

	}
	else
	if (memcmp(thedrive->data,"EXTENDED",8)==0)
	{
		thedrive->disk_image_type = 1;

		/* extended disk image */
		dsk_extended_dsk_init_track_offsets(thedrive);
	}
}


static void dsk_seek_callback(mess_image *img, int physical_track)
{
	get_drive(img)->current_track = physical_track;
}

static int get_track_offset(mess_image *img, int side)
{
	dsk_drive *thedrive = get_drive(img);
	side = side & 0x01;
	return thedrive->track_offsets[(thedrive->current_track<<1) + side];
}

static unsigned char *get_floppy_data(mess_image *img)
{
	return get_drive(img)->data;
}

static void dsk_get_id_callback(mess_image *img, chrn_id *id, int id_index, int side)
{
	int id_offset;
	int track_offset;
	unsigned char *track_header;
	unsigned char *data;

	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = get_track_offset(img, side);

	/* track exists? */
	if (track_offset==0)
		return;

	/* yes */
	data = get_floppy_data(img);

	if (data==0)
		return;

	track_header = data + track_offset;

	id_offset = 0x018 + (id_index<<3);

	id->C = track_header[id_offset + 0];
	id->H = track_header[id_offset + 1];
	id->R = track_header[id_offset + 2];
	id->N = track_header[id_offset + 3];
	id->flags = 0;
	id->data_id = id_index;

	if (track_header[id_offset + 5] & 0x040)
	{
		id->flags |= ID_FLAG_DELETED_DATA;
	}




//	id->ST0 = track_header[id_offset + 4];
//	id->ST1 = track_header[id_offset + 5];

}


static void dsk_set_ddam(mess_image *img, int id_index, int side, int ddam)
{
	int id_offset;
	int track_offset;
	unsigned char *track_header;
	unsigned char *data;

	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = get_track_offset(img, side);

	/* track exists? */
	if (track_offset==0)
		return;

	/* yes */
	data = get_floppy_data(img);

	if (data==0)
		return;

	track_header = data + track_offset;

	id_offset = 0x018 + (id_index<<3);

	track_header[id_offset + 5] &= ~0x040;

	if (ddam)
	{
		track_header[id_offset + 5] |= 0x040;
	}
}


static char * dsk_get_sector_ptr_callback(mess_image *img, int sector_index, int side)
{
	int track_offset;
	int sector_offset;
	int track;
	dsk_drive *thedrive;
	unsigned char *data;

	side = side & 0x01;

	thedrive = get_drive(img);

	track = thedrive->current_track;

	/* offset to track header in image */
	track_offset = get_track_offset(img, side);

	/* track exists? */
	if (track_offset==0)
		return NULL;


	/* setup sector offsets */
	switch (thedrive->disk_image_type)
	{
	case 0:
		dsk_dsk_init_sector_offsets(thedrive,track, side);
		break;


	case 1:
		dsk_extended_dsk_init_sector_offsets(thedrive, track, side);
		break;

	default:
		break;
	}

	sector_offset = thedrive->sector_offsets[sector_index];

	data = get_floppy_data(img);

	if (data==0)
		return NULL;

	return (char *)(data + track_offset + sector_offset);
}

static void dsk_write_sector_data_from_buffer(mess_image *img, int side, int index1, const char *ptr, int length, int ddam)
{
	char * pSectorData;

	pSectorData = dsk_get_sector_ptr_callback(img, index1, side);

	if (pSectorData!=NULL)
	{
		memcpy(pSectorData, ptr, length);
	}

	/* set ddam */
	dsk_set_ddam(img, index1, side,ddam);
}

static void dsk_read_sector_data_into_buffer(mess_image *img, int side, int index1, char *ptr, int length)
{
	char *pSectorData;

	pSectorData = dsk_get_sector_ptr_callback(img, index1, side);

	if (pSectorData!=NULL)
		memcpy(ptr, pSectorData, length);
}

static int dsk_get_sectors_per_track(mess_image *img, int side)
{
	int track_offset;
	unsigned char *track_header;
	unsigned char *data;

	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = get_track_offset(img, side);

	/* track exists? */
	if (track_offset==0)
		return 0;

	data = get_floppy_data(img);

	if (data==0)
		return 0;

	/* yes, get sectors per track */
	track_header = data  + track_offset;

	return track_header[0x015];
}



void legacydsk_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_dsk_floppy; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_dsk_floppy; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_dsk_floppy; break;
		case DEVINFO_PTR_STATUS:						/* info->status = floppy_status; */ break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:						strcpy(info->s = device_temp_str(), __FILE__); break;
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;
	}
}

