/* mfm disk image type used by euphoric */
/* this disc image contains raw mfm data */
/* TODO:


	- Check code is correct for single density discs
	- Check code is correct for detecting number of sides and track count
	- clever write and read which correctly setup the raw mfm
*/
#include "driver.h"
#include "devices/mfmdisk.h"
#include "devices/flopdrv.h"

static void mfm_disk_seek_callback(mess_image *image, int);
static int mfm_disk_get_sectors_per_track(mess_image *image, int);
static void mfm_disk_get_id_callback(mess_image *image, chrn_id *, int, int);
static void mfm_disk_read_sector_data_into_buffer(mess_image *image, int side, int index1, char *ptr, int length);
static void mfm_disk_write_sector_data_from_buffer(mess_image *image, int side, int index1, const char *ptr, int length,int ddam);

const floppy_interface mfm_disk_floppy_interface =
{
	mfm_disk_seek_callback,
	mfm_disk_get_sectors_per_track,             /* done */
	mfm_disk_get_id_callback,                   /* done */
	mfm_disk_read_sector_data_into_buffer,      /* done */
	mfm_disk_write_sector_data_from_buffer, /* done */
	NULL,
	NULL
};


#define MFM_ID	"MFM_DISK"

#define MFM_DISK_DENSITY_FM_HIGH 1
#define MFM_DISK_DENSITY_MFM_LO 2

/* size of header */
#define mfm_disk_header_size	0x0100

/* size of a double density track in this image */
const int TrackSizeMFMLo = 0x01900;

/* number of disk images that this code can handle at one time */
#define MAX_MFM_DISK	4

/* max number of sectors that info can be stored about */
#define MAX_SECTORS		32

struct mfm_disk_sector_info
{
	unsigned char *id_ptr;
	unsigned char *data_ptr;
};

struct mfm_disk_info
{
	/* pointer to image data */
	unsigned char *pData;

	/* current track */
	int CurrentTrack;

	/* from header */
	unsigned long NumTracks;

	/* from header */
	unsigned long NumSides;

	/* from header */
	unsigned long Density;



	/* these apply to the current track */
	int CachedTrack, CachedSide;
	unsigned long NumSectors;
	struct mfm_disk_sector_info	sectors[32];


};

/* the disk images */
static struct mfm_disk_info	mfm_disks[MAX_MFM_DISK];

/* this code is not very clever. It assumes that the data is stored as:

  id mark, data block, id mark, data block...

  This is ok for standard format discs and some copyprotections, however,
  I have seen discs on the Amstrad CPC which have:

  id mark, id mark, id mark, data block....

  This code cannot detect these, because it does not know the difference between
  a data mark/deleted data mark and data in a data field or id field.

  This code also assumes that the N value in the ID field defines the size of the
  data block following. If the data block is a different size, it could miss sectors,
  or jump into the middle of a sector and then get confused about a id mark/data mark/
  deleted data mark and data in the sector field!
*/

static struct mfm_disk_info *get_disk(mess_image *image)
{
	int disk = image_index_in_device(image);
	return &mfm_disks[disk];
}

/* TODO: Error checking if a id is found at very end of track, or a sector which
goes over end of track */
static void mfm_info_cache_sector_info(mess_image *image,unsigned char *pTrackPtr, unsigned long Length)
{
	/* initialise these with single density values if single density */
	unsigned char IdMark = 0x0fe;
	unsigned char DataMark = 0x0fb;
	unsigned char DeletedDataMark = 0x0f8;

	unsigned long SectorCount;
	unsigned long N = 0;
	unsigned long SearchCode = 0;
	unsigned char *pStart = pTrackPtr;
	struct mfm_disk_info *mfm_disk = get_disk(image);

	SectorCount = 0;

	do
	{
		switch (SearchCode)
		{
			/* searching for id's */
			case 0:
			{
				/* found id mark? */
				if (pTrackPtr[0] == IdMark)
				{
					/* store pointer to id mark */
					mfm_disk->sectors[SectorCount].id_ptr = pTrackPtr;
					SectorCount++;

					/* grab N value - used to skip data in data field */
					N = pTrackPtr[4];

					/* skip past id field and crc */
					pTrackPtr+=7;

					/* now looking for data field */
					SearchCode = 1;
				}
				else
				{
					/* update position */
					pTrackPtr++;
				}
			}
			break;

			/* searching for data id's */
			case 1:
			{
				/* found data or deleted data? */
				if ((pTrackPtr[0] == DataMark) || (pTrackPtr[0] == DeletedDataMark))
				{
					/* yes */
					mfm_disk->sectors[SectorCount-1].data_ptr = pTrackPtr;

					/* skip data field and id */
					pTrackPtr+=(1<<(N+7)) + 3;

					/* now looking for id field */
					SearchCode = 0;

				}
				else
				{
					pTrackPtr++;
				}
			}
			break;

			default:
				break;
		}
	}
	while ((pTrackPtr-pStart)<Length);

	mfm_disk->NumSectors = SectorCount;
	logerror("mfm disk: Num Sectors %02x\n,", (int) SectorCount);

}

static unsigned long mfm_disk_get_track_size(mess_image *image)
{
	struct mfm_disk_info *pDisk = get_disk(image);
	switch (pDisk->Density)
	{
		case MFM_DISK_DENSITY_MFM_LO:
			return TrackSizeMFMLo;

		default:
			break;
	}

	return 0;
}


static unsigned char *mfm_disk_get_track_ptr(mess_image *image, int track, int side)
{
	struct mfm_disk_info *pDisk = get_disk(image);

	unsigned long TrackSize;

	TrackSize = mfm_disk_get_track_size(image);

	return (unsigned char *)((unsigned long)pDisk->pData + mfm_disk_header_size + (unsigned long)(TrackSize*((track*pDisk->NumSides)+side)));
}

/* this is endian safe */
static unsigned long mfm_get_long(unsigned char *addr)
{
	int i;
	unsigned long Data = 0;
	int Shift;

	Shift = 0;

	/* get 4 bytes */
	for (i=0; i<4; i++)
	{
		/* get byte, shift it up and combine with existing value so far */
		Data = Data | ((addr[i] & 0x0ff)<<Shift);
		/* update shift */
		Shift = Shift+8;
	}

	return Data;
}

#if 0
/* check a mfm_disk image is valid */
int	mfm_disk_id(int id)
{
	mame_file *file;
	int result = 0;

	/* open file and determine image geometry */
	file = image_fopen(IO_FLOPPY, id, FILETYPE_IMAGE, OSD_FOPEN_READ);

	if (file)
	{
		unsigned char header[mfm_disk_header_size];
		unsigned long FileLength;

		/* get length of file */
		FileLength = mame_fsize(file);

		if (FileLength!=0)
		{
			/* load header */
			if (mame_fread(file, header, mfm_disk_header_size))
			{
				/* check for text ident */
				if (memcmp(header, MFM_ID, 8)==0)
				{
					unsigned long NumTracks;
					unsigned long TrackSize;
					unsigned long NumSides;
					unsigned long Density;

					NumTracks = mfm_get_long(&header[12]);

					NumSides = mfm_get_long(&header[16]);

					Density = mfm_get_long(&header[8]);

					switch (Density)
					{
						default:
						case MFM_DISK_DENSITY_MFM_LO:
						{
							TrackSize = TrackSizeMFMLo;
						}
						break;

					}

					/* size must be long enough to contain the data */
					if (FileLength>=((NumTracks*NumSides*TrackSize)+mfm_disk_header_size))
					{
						logerror("mfm disk id succeeded!\n");
						result = 1;
					}
				}
			}
		}

		mame_fclose(file);
	}

	return result;
}
#endif


/* load image */
int device_load_mfm_disk(mess_image *image)
{
	struct mfm_disk_info *pDisk = get_disk(image);

	pDisk->pData = image_ptr(image);
	if (!pDisk->pData)
		return INIT_FAIL;

	pDisk->NumTracks = mfm_get_long(&pDisk->pData[12]);
	pDisk->NumSides = mfm_get_long(&pDisk->pData[16]);
	pDisk->Density = mfm_get_long(&pDisk->pData[8]);
	pDisk->CachedTrack = -1;
	pDisk->CachedSide = -1;
	pDisk->NumSectors = 0;

	floppy_drive_set_disk_image_interface(image, &mfm_disk_floppy_interface);

	logerror("mfm disk inserted!\n");
	return INIT_PASS;
}

/* cache info about track */
static void mfm_disk_cache_data(mess_image *image, int track, int side)
{
	struct mfm_disk_info *mfm_disk = get_disk(image);

	if ((track!=mfm_disk->CachedTrack) || (side!=mfm_disk->CachedSide))
	{
		unsigned char *pTrackPtr = mfm_disk_get_track_ptr(image, track, side);
		unsigned long TrackSize = mfm_disk_get_track_size(image);

		mfm_info_cache_sector_info(image, pTrackPtr, TrackSize);

		mfm_disk->CachedTrack = track;
		mfm_disk->CachedSide = side;
	}
}



static void mfm_disk_get_id_callback(mess_image *image, chrn_id *id, int id_index, int side)
{
	struct mfm_disk_info *mfm_disk = get_disk(image);

	mfm_disk_cache_data(image, mfm_disk->CurrentTrack, side);

	if (id_index<mfm_disk->NumSectors)
	{
		/* construct a id value */
		id->C = mfm_disk->sectors[id_index].id_ptr[1];
		id->H = mfm_disk->sectors[id_index].id_ptr[2];
		id->R = mfm_disk->sectors[id_index].id_ptr[3];
		id->N = mfm_disk->sectors[id_index].id_ptr[4];
		id->data_id = id_index;
		id->flags = 0;

		/* get dam - assumes double density - to be fixed */
		if (mfm_disk->sectors[id_index].data_ptr[0]==0x0f8)
		{
			id->flags |= ID_FLAG_DELETED_DATA;
		}
	}
}

static int mfm_disk_get_sectors_per_track(mess_image *image, int side)
{
	struct mfm_disk_info *mfm_disk = get_disk(image);

	/* attempting to access an invalid side or track? */
	if ((side>=mfm_disk->NumSides) || (mfm_disk->CurrentTrack>=mfm_disk->NumTracks))
	{
		/* no sectors */
		return 0;
	}
	mfm_disk_cache_data(image, mfm_disk->CurrentTrack, side);

	/* return number of sectors per track */
	return mfm_disk->NumSectors;
}

static void mfm_disk_seek_callback(mess_image *image, int physical_track)
{
	struct mfm_disk_info *mfm_disk = get_disk(image);
	mfm_disk->CurrentTrack = physical_track;
}

/* reading and writing are not clever, they need to be so that the data is correctly written back
to the image */
static void mfm_disk_write_sector_data_from_buffer(mess_image *image, int side, int index1, const char *ptr, int length, int ddam)
{
	struct mfm_disk_info *mfm_disk = get_disk(image);
	unsigned char *pSectorPtr;

	mfm_disk_cache_data(image, mfm_disk->CurrentTrack, side);

	pSectorPtr = mfm_disk->sectors[index1].data_ptr+1;

	memcpy(pSectorPtr, ptr, length);
}

static void mfm_disk_read_sector_data_into_buffer(mess_image *image, int side, int index1, char *ptr, int length)
{
	struct mfm_disk_info *mfm_disk = get_disk(image);
	unsigned char *pSectorPtr;

	mfm_disk_cache_data(image, mfm_disk->CurrentTrack, side);

	pSectorPtr = mfm_disk->sectors[index1].data_ptr+1;

	memcpy(ptr, pSectorPtr, length);
}

