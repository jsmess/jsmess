/* This is a basic disc image format.
Each driver which uses this must use basicdsk_set_geometry, so that
the data will be accessed correctly */


/* THIS DISK IMAGE CODE USED TO BE PART OF THE WD179X EMULATION, EXTRACTED INTO THIS FILE */
#include "driver.h"
#include "devices/basicdsk.h"
#include "devices/flopdrv.h"
#include "image.h"

#define basicdsk_MAX_DRIVES 4
#define VERBOSE 1


typedef struct
{
	UINT8	 track;
	UINT8	 sector;
	UINT8	 status;
}	SECMAP;

typedef struct
{
	const char *image_name; 		/* file name for disc image */
	mess_image	*image_file;			/* file handle for disc image */
	int 	mode;					/* open mode == 0 read only, != 0 read/write */
	unsigned long image_size;		/* size of image file */

	SECMAP	*secmap;

	UINT8	unit;					/* unit number if image_file == REAL_FDD */

	UINT8	tracks; 				/* maximum # of tracks */
	UINT8	heads;					/* maximum # of heads */

	UINT16	offset; 				/* track 0 offset */
	UINT8	first_sector_id;		/* id of first sector */
	UINT8	sec_per_track;			/* sectors per track */

	UINT8	head;					/* current head # */
	UINT8	track;					/* current track # */

    UINT8   N;
	UINT16	sector_length;			/* sector length (byte) */

	int		track_divider;			/* 2 if using a 40-track image in a 80-track drive, 1 otherwise */

	/* a bit for each sector in the image. If the bit is set, this sector
	has a deleted data address mark. If the bit is not set, this sector
	has a data address mark */
	UINT8	*ddam_map;
	unsigned long ddam_map_size;

	unsigned long (*calcoffset)(UINT8 t, UINT8 h, UINT8 s,
		UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero);
} basicdsk;

static basicdsk basicdsk_drives[basicdsk_MAX_DRIVES];

static void basicdsk_seek_callback(mess_image *img, int);
static int basicdsk_get_sectors_per_track(mess_image *img, int);
static void basicdsk_get_id_callback(mess_image *img,  chrn_id *, int, int);
static void basicdsk_read_sector_data_into_buffer(mess_image *img, int side, int index1, char *ptr, int length);
static void basicdsk_write_sector_data_from_buffer(mess_image *img, int side, int index1, const char *ptr, int length,int ddam);

const floppy_interface basicdsk_floppy_interface =
{
	basicdsk_seek_callback,
	basicdsk_get_sectors_per_track,             /* done */
	basicdsk_get_id_callback,                   /* done */
	basicdsk_read_sector_data_into_buffer,      /* done */
	basicdsk_write_sector_data_from_buffer, /* done */\
	NULL,
	NULL
};

static basicdsk *get_basicdsk(mess_image *img)
{
	int drive = image_index_in_device(img);
	assert(drive >= 0);
	assert(drive < (sizeof(basicdsk_drives) / sizeof(basicdsk_drives[0])));
	return &basicdsk_drives[drive];
}

int device_init_basicdsk_floppy(mess_image *image)
{
	return floppy_drive_init(image, &basicdsk_floppy_interface);
}

/* attempt to insert a disk into the drive specified with id */
int device_load_basicdsk_floppy(mess_image *image)
{
	basicdsk *w = get_basicdsk(image);

	w->image_file = image;
	w->mode = image_is_writable(image);

	/* this will be setup in the set_geometry function */
	w->ddam_map = NULL;

	/* the following line is unsafe, but floppy_drives_init assumes we start on track 0,
	so we need to reflect this */
	w->track = 0;

	floppy_drive_set_disk_image_interface(image, &basicdsk_floppy_interface);

	return INIT_PASS;
}

void device_unload_basicdsk_floppy(mess_image *image)
{
	basicdsk *w = get_basicdsk(image);
	w->image_file = NULL;
	w->mode = 0;
}

/* set data mark/deleted data mark for the sector specified. If ddam!=0, the sector will
have a deleted data mark, if ddam==0, the sector will have a data mark */
void basicdsk_set_ddam(mess_image *img, UINT8 physical_track, UINT8 physical_side, UINT8 sector_id,UINT8 ddam)
{
	unsigned long ddam_bit_offset, ddam_bit_index, ddam_byte_offset;
	basicdsk *pDisk = get_basicdsk(img);

	if (!pDisk->ddam_map)
		return;

	logerror("basicdsk_set_ddam: T:$%02x H:%d S:$%02x = %d\n", physical_track, physical_side, sector_id,ddam);

    /* calculate bit-offset into map */
	ddam_bit_offset = (((physical_track * pDisk->heads) + physical_side)*pDisk->sec_per_track) +
					sector_id - pDisk->first_sector_id;

	/* if offset exceeds the number of bits that are stored in the ddam map return 0 */
	if (ddam_bit_offset>=(pDisk->ddam_map_size<<3))
		return;

	/* calculate byte offset */
	ddam_byte_offset = ddam_bit_offset>>3;
	/* calc bit index within byte */
	ddam_bit_index = ddam_bit_offset & 0x07;

	/* clear bit */
	pDisk->ddam_map[ddam_byte_offset] &= ~(1<<ddam_bit_index);

	/* deleted dam? */
	if (ddam)
	{
		/* set deleted dam */
		pDisk->ddam_map[ddam_byte_offset] |= (1<<ddam_bit_index);
	}
}

/* get dam state for specified sector */
static int basicdsk_get_ddam(mess_image *img, UINT8 physical_track, UINT8 physical_side, UINT8 sector_id)
{
	unsigned long ddam_bit_offset, ddam_bit_index, ddam_byte_offset;
	basicdsk *pDisk = get_basicdsk(img);

	if (!pDisk->ddam_map || !image_exists(img))
		return 0;

	/* calculate bit-offset into map */
	ddam_bit_offset = (((physical_track * pDisk->heads) + physical_side)*pDisk->sec_per_track) +
					sector_id - pDisk->first_sector_id;

	/* if offset exceeds the number of bits that are stored in the ddam map return 0 */
	if (ddam_bit_offset>=(pDisk->ddam_map_size<<3))
		return 0;

	/* calculate byte offset */
	ddam_byte_offset = ddam_bit_offset>>3;
	/* calc bit index within byte */
	ddam_bit_index = ddam_bit_offset & 0x07;

	/* clear bit */
	return ((pDisk->ddam_map[ddam_byte_offset] & (1<<ddam_bit_index))!=0);
}


void basicdsk_set_calcoffset(mess_image *img, unsigned long (*calcoffset)(UINT8 t, UINT8 h, UINT8 s,
	UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero))
{
	basicdsk *pDisk = get_basicdsk(img);
	pDisk->calcoffset = calcoffset;
}

/* dir_sector is a relative offset from the start of the disc,
dir_length is a relative offset from the start of the disc */
void basicdsk_set_geometry(mess_image *img, UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero, int track_skipping)
{
	unsigned long N;
	unsigned long ShiftCount;
	basicdsk *pDisk = get_basicdsk(img);

	pDisk->tracks = tracks;
	pDisk->heads = heads;
	pDisk->first_sector_id = first_sector_id;
	pDisk->sec_per_track = sec_per_track;
	pDisk->sector_length = sector_length;
	pDisk->offset = offset_track_zero;

	pDisk->track_divider = track_skipping ? 2 : 1;

	floppy_drive_set_geometry_absolute(img, tracks * pDisk->track_divider, heads );
	
	pDisk->image_size = pDisk->tracks * pDisk->heads * pDisk->sec_per_track * pDisk->sector_length;

	/* setup a new ddam map */
	pDisk->ddam_map_size = ((pDisk->tracks * pDisk->heads * pDisk->sec_per_track)+7)>>3;
	pDisk->ddam_map = (UINT8 *) image_realloc(img, pDisk->ddam_map, pDisk->ddam_map_size);

	if (pDisk->ddam_map!=NULL)
	{
		memset(pDisk->ddam_map, 0, pDisk->ddam_map_size);
	}


	/* from sector length calculate N value for sector id's */
	/* N = 0 for 128, N = 1 for 256, N = 2 for 512 ... */
	N = (pDisk->sector_length);
	ShiftCount = 0;

	if (N!=0)
	{
		while ((N & 0x080000000)==0)
		{
			N = N<<1;
			ShiftCount++;
		}

		/* get left-shift required to shift 1 to this
		power of 2 */

		pDisk->N = (31 - ShiftCount)-7;
	}
	else
	{
		pDisk->N = 0;
	}
}


/* seek to track/head/sector relative position in image file */
static int basicdsk_seek(basicdsk * w, UINT8 t, UINT8 h, UINT8 s)
{
	unsigned long offset;

	/* allow two additional tracks */
    if (t >= w->tracks + 2)
	{
		logerror("basicdsk track %d >= %d\n", t, w->tracks + 2);
		return 0;
	}

    if (h >= w->heads)
    {
		logerror("basicdsk head %d >= %d\n", h, w->heads);
		return 0;
	}

    if (s >= (w->first_sector_id + w->sec_per_track))
	{
		logerror("basicdsk sector %d\n", w->sec_per_track+w->first_sector_id);
		return 0;
	}

	if (w->calcoffset)
	{
		offset = w->calcoffset(t, h, s, w->tracks, w->heads, w->sec_per_track, w->sector_length, w->first_sector_id, w->offset);
	}
	else
	{
		offset = 0;
		offset += t;
		offset *= w->heads;
		offset += h;
		offset *= w->sec_per_track;
		offset += (s-w->first_sector_id);
		offset *= w->sector_length;
		offset += w->offset;
	}


#if VERBOSE
    logerror("basicdsk seek track:%d head:%d sector:%d-> offset #0x%08lX\n",
             t, h, s, offset);
#endif

	if (offset > w->image_size)
	{
		logerror("basicdsk seek offset %ld >= %ld\n", offset, w->image_size);
		return 0;
	}

	if (image_fseek(w->image_file, offset, SEEK_SET) < 0)
	{
		logerror("basicdsk seek failed\n");
		return 0;
	}

	return 1;
}




#if 0
			w->status = seek(w, w->track, w->head, w->sector);
			if (w->status == 0)
				read_sector(w);


	/* if a track was just formatted */
	if (w->dam_cnt)
	{
		int i;
		for (i = 0; i < w->dam_cnt; i++)
		{
			if (w->track == w->dam_list[i][0] &&
				w->head == w->dam_list[i][1] &&
				w->sector == w->dam_list[i][2])
			{
#if VERBOSE
                                logerror("basicdsk reading formatted sector %d, track %d, head %d\n", w->sector, w->track, w->head);
#endif
				w->data_offset = w->dam_data[i];
				return;
			}
		}
		/* sector not found, now the track buffer is invalid */
		w->dam_cnt = 0;
	}

    /* if this is the real thing */
    if (w->image_file == REAL_FDD)
    {
	int tries = 3;
		do {
			w->status = osd_fdc_get_sector(w->track, w->head, w->head, w->sector, w->buffer);
			tries--;
		} while (tries && (w->status & (STA_2_REC_N_FND | STA_2_CRC_ERR | STA_2_LOST_DAT)));
		/* no error bits set ? */
		if ((w->status & (STA_2_REC_N_FND | STA_2_CRC_ERR | STA_2_LOST_DAT)) == 0)
		{
			/* start transferring data to the emulation now */
			w->status_drq = STA_2_DRQ;
			if (w->callback)
                                (*w->callback) (basicdsk_DRQ_SET);
			w->status |= STA_2_DRQ | STA_2_BUSY;
        }
        return;
    }
	else if (image_fread(w->image_file, w->buffer, w->sector_length) != w->sector_length)
	{
		w->status = STA_2_LOST_DAT;
		return;
	}


#endif



/*void    basicdsk_step_callback(basicdsk *w, int drive, int direction)
{
			w->track += direction;
}*/

#if 0
/* write a sector */
static void basicdsk_write_sector(basicdsk *w)
{
	if (w->image_file == REAL_FDD)
	{
                osd_fdc_put_sector(w->track, w->head, w->head, w->sector, w->buffer, w->write_cmd & FDC_DELETED_AM);
		return;
	}

    seek(w, w->track, w->head, w->sector);
    image_fwrite(w->image_file, w->buffer, w->data_offset)
}


/* write an entire track by extracting the sectors */
static void basicdsk_write_track(basicdsk *w)
{
	if (floppy_drive_get_flag_state(drv,FLOPPY_DRIVE_DISK_WRITE_PROTECTED))
    {
		w->status |= STA_1_WRITE_PRO;
		return;
	}
#endif

#if 0
UINT8 *f;
int cnt;
	w->dam_cnt = 0;
    if (w->image_file != REAL_FDD && w->mode == 0)
    {
#if VERBOSE
                logerror("basicdsk write_track write protected image\n");
#endif
        w->status = STA_2_WRITE_PRO;
        return;
    }

	memset(w->dam_list, 0xff, sizeof(w->dam_list));
	memset(w->dam_data, 0x00, sizeof(w->dam_data));

	f = w->buffer;
#if VERBOSE
        logerror("basicdsk write_track %s_LOW\n", (w->density) ? "MFM" : "FM" );
#endif
    cnt = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;

	do
	{
		while ((--cnt > 0) && (*f != 0xfe))	/* start of DAM ?? */
			f++;

		if (cnt > 4)
		{
		int seclen;
			cnt -= 5;
			f++;			   /* skip FE */
			w->dam_list[w->dam_cnt][0] = *f++;	  /* copy track number */
			w->dam_list[w->dam_cnt][1] = *f++;	  /* copy head number */
			w->dam_list[w->dam_cnt][2] = *f++;	  /* copy sector number */
			w->dam_list[w->dam_cnt][3] = *f++;	  /* copy sector length */
			/* sector length in bytes */
			seclen = 128 << w->dam_list[w->dam_cnt][3];
#if VERBOSE
                        logerror("basicdsk write_track FE @%5d T:%02X H:%02X S:%02X L:%02X\n",
					(int)(f - w->buffer),
					w->dam_list[w->dam_cnt][0],w->dam_list[w->dam_cnt][1],
					w->dam_list[w->dam_cnt][2],w->dam_list[w->dam_cnt][3]);
#endif
			/* search start of DATA */
			while ((--cnt > 0) && (*f != 0xf9) && (*f != 0xfa) && (*f != 0xfb))
				f++;
			if (cnt > seclen)
			{
				cnt--;
				/* skip data address mark */
                f++;
                /* set pointer to DATA to later write the sectors contents */
				w->dam_data[w->dam_cnt] = (int)(f - w->buffer);
				w->dam_cnt++;
#if VERBOSE
                                logerror("basicdsk write_track %02X @%5d data: %02X %02X %02X %02X ... %02X %02X %02X %02X\n",
						f[-1],
						(int)(f - w->buffer),
						f[0], f[1], f[2], f[3],
						f[seclen-4], f[seclen-3], f[seclen-2], f[seclen-1]);
#endif
				f += seclen;
				cnt -= seclen;
			}
        }
	} while (cnt > 0);

	if (w->image_file == REAL_FDD)
	{
		w->status = osd_fdc_format(w->track, w->head, w->dam_cnt, w->dam_list[0]);

        if ((w->status & 0xfc) == 0)
		{
			/* now put all sectors contained in the format buffer */
			for (cnt = 0; cnt < w->dam_cnt; cnt++)
			{
				w->status = osd_fdc_put_sector(w->track, w->head, cnt, w->buffer[dam_data[cnt]], 0);
				/* bail out if an error occured */
				if (w->status & 0xfc)
					break;
			}
        }
    }
	else
	{
		/* now put all sectors contained in the format buffer */
		for (cnt = 0; cnt < w->dam_cnt; cnt++)
		{
			w->status = seek(w, w->track, w->head, w->dam_list[cnt][2]);
			if (w->status == 0)
			{
				if (image_fwrite(w->image_file, &w->buffer[w->dam_data[cnt]], w->sector_length) != w->sector_length)
				{
					w->status = STA_2_LOST_DAT;
					return;
				}
			}
		}
	}
}
#endif
#if 0
			if (w->image_file != REAL_FDD)
			{
				/* read normal or deleted data address mark ? */
				w->status |= deleted_dam(w);
			}
#endif


#if 0

	if ((data | 1) == 0xff)	   /* change single/double density ? */
	{
		/* only supports FM/LO and MFM/LO */
		w->density = (data & 1) ? DEN_MFM_LO : DEN_FM_LO;
#if 0
		if (w->image_file == REAL_FDD)
			osd_fdc_density(w->unit, w->density, w->tracks, w->sec_per_track, w->sec_per_track, 1);
#endif
		return;
	}
#endif


static void basicdsk_get_id_callback(mess_image *img, chrn_id *id, int id_index, int side)
{
	basicdsk *w = get_basicdsk(img);

	/* construct a id value */
	id->C = w->track;
	id->H = side;
	id->R = w->first_sector_id + id_index;
    id->N = w->N;
	id->data_id = w->first_sector_id + id_index;
	id->flags = 0;

	/* get dam */
	if (basicdsk_get_ddam(img, w->track, side, id->R))
	{
		id->flags |= ID_FLAG_DELETED_DATA;
	}

}

static int basicdsk_get_sectors_per_track(mess_image *img, int side)
{
	basicdsk *w = get_basicdsk(img);

	/* attempting to access an invalid side or track? */
	if ((side>=w->heads) || (w->track>=w->tracks))
	{
		/* no sectors */
		return 0;
	}
	/* return number of sectors per track */
	return w->sec_per_track;
}

static void basicdsk_seek_callback(mess_image *img, int physical_track)
{
	basicdsk *w = get_basicdsk(img);
	assert(w->track_divider);
	w->track = physical_track / w->track_divider;
}

static void basicdsk_write_sector_data_from_buffer(mess_image *img, int side, int index1, const char *ptr, int length, int ddam)
{
	basicdsk *w = get_basicdsk(img);

	if (basicdsk_seek(w, w->track, side, index1)&&w->mode)
	{
		image_fwrite(w->image_file, ptr, length);
	}

	basicdsk_set_ddam(img, w->track, side, index1, ddam);
}

static void basicdsk_read_sector_data_into_buffer(mess_image *img, int side, int index1, char *ptr, int length)
{
	basicdsk *w = get_basicdsk(img);

	if (basicdsk_seek(w, w->track, side, index1))
	{
		image_fread(w->image_file, ptr, length);
	}
}



void legacybasicdsk_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:						info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:					info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:					info->i = 1; break;
		case DEVINFO_INT_CREATABLE:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:						info->init = device_init_basicdsk_floppy; break;
		case DEVINFO_PTR_UNLOAD:					info->unload = device_unload_basicdsk_floppy; break;
		case DEVINFO_PTR_STATUS:					/* info->status = floppy_status; */ break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
	}
}


