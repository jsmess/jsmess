/* This is a .D88 disc image format. */

#include "driver.h"
#include "includes/d88.h"
#include "devices/flopdrv.h"
#include "image.h"

#define d88image_MAX_DRIVES 4
#define VERBOSE 1
static d88image     d88image_drives[d88image_MAX_DRIVES];

static void d88image_seek_callback(mess_image *img,int);
static int d88image_get_sectors_per_track(mess_image *img,int);
static void d88image_get_id_callback(mess_image *img, chrn_id *, int, int);
static void d88image_read_sector_data_into_buffer(mess_image *img, int side, int index1, char *ptr, int length);
static void d88image_write_sector_data_from_buffer(mess_image *img, int side, int index1, const char *ptr, int length,int ddam);

static floppy_interface d88image_floppy_interface=
{
	d88image_seek_callback,
	d88image_get_sectors_per_track,             /* done */
	d88image_get_id_callback,                   /* done */
	d88image_read_sector_data_into_buffer,      /* done */
	d88image_write_sector_data_from_buffer, /* done */
	NULL,
	NULL
};

static d88image *get_d88image(mess_image *img)
{
	return &d88image_drives[image_index_in_device(img)];
}

DEVICE_INIT(d88image_floppy)
{
	return floppy_drive_init(image, &d88image_floppy_interface);
}

/* attempt to insert a disk into the drive specified with id */
DEVICE_LOAD(d88image_floppy)
{
	UINT8 tmp8;
	UINT16 tmp16;
	UINT32 tmp32;
	int i,j,k;
	unsigned long toffset;
	d88image *w;
	int id = image_index_in_device(image);

	assert(id < d88image_MAX_DRIVES);

	w = &d88image_drives[id];

	w->image_file = image;
	w->mode = image_is_writable(image);

	/* the following line is unsafe, but floppy_drives_init assumes we start on track 0,
	so we need to reflect this */
	w->track = 0;

	image_fread(w->image_file, w->disk_name, 17);
	for(i=0;i<9;i++) image_fread(w->image_file, &tmp8, 1);
	image_fread(w->image_file, &tmp8, 1);
	w->write_protected = (tmp8&0x10 || !w->mode);
	image_fread(w->image_file, &tmp8, 1);
	w->disktype = tmp8 >> 4;
	image_fread(w->image_file, &tmp32, 4);
	w->image_size = LITTLE_ENDIANIZE_INT32(tmp32);

	for(i = 0; i<D88_NUM_TRACK; i++)
	{
		image_fseek(w->image_file, 0x20 + i*4, SEEK_SET);
		image_fread(w->image_file, &tmp32, 4);
		toffset = LITTLE_ENDIANIZE_INT32(tmp32);
		if(toffset)
		{
			image_fseek(w->image_file, toffset + 4, SEEK_SET);
			image_fread(w->image_file, &tmp16, 2);
			w->num_sects[i] = LITTLE_ENDIANIZE_INT16(tmp16);
			w->sects[i] = image_malloc(image, sizeof(d88sect)*w->num_sects[i]);
			image_fseek(w->image_file, toffset, SEEK_SET);

			for(j=0;j<w->num_sects[i];j++)
			{
				image_fread(w->image_file, &(w->sects[i][j].C), 1);
				image_fread(w->image_file, &(w->sects[i][j].H), 1);
				image_fread(w->image_file, &(w->sects[i][j].R), 1);
				image_fread(w->image_file, &(w->sects[i][j].N), 1);
				image_fread(w->image_file, &tmp16, 2);
				image_fread(w->image_file, &tmp8, 1);
					w->sects[i][j].den=tmp8&0x40 ?
					(w->disktype==2 ? DEN_FM_HI : DEN_FM_LO) :
					(w->disktype==2 ? DEN_MFM_HI : DEN_MFM_LO);
				image_fread(w->image_file, &tmp8, 1);
				w->sects[i][j].flags=tmp8&0x10 ? ID_FLAG_DELETED_DATA : 0;
				image_fread(w->image_file, &tmp8, 1);

				switch(tmp8 & 0xf0) {
				case 0xa0:
					w->sects[i][j].flags|=ID_FLAG_CRC_ERROR_IN_ID_FIELD;
					break;
				case 0xb0:
					w->sects[i][j].flags|=ID_FLAG_CRC_ERROR_IN_DATA_FIELD;
					break;
				}
				for(k=0;k<5;k++)
					image_fread(w->image_file, &tmp8, 1);
				image_fread(w->image_file, &tmp16, 2);
				w->sects[i][j].offset = image_ftell(w->image_file);
				image_fseek(w->image_file, LITTLE_ENDIANIZE_INT16(tmp16), SEEK_CUR);
			}
		}
		else
		{
			w->num_sects[i] = 0;
			w->sects[i]=NULL;
		}
	}
	return  INIT_PASS;
}

/* seek to track/head/sector relative position in image file */
static int d88image_seek(d88image * w, UINT8 t, UINT8 h, UINT8 s)
{
	unsigned long offset;
	/* allow two additional tracks */
    if (t >= D88_NUM_TRACK/2)
	{
		logerror("d88image track %d >= %d\n", t, D88_NUM_TRACK/2);
		return 0;
	}

    if (h >= 2)
    {
		logerror("d88image head %d >= %d\n", h, 2);
		return 0;
	}

    if (s >= w->num_sects[t*2+h])
	{
		logerror("d88image sector %d\n", w->num_sects[t*2+h]);
		return 0;
	}

	offset = w->sects[t*2+h][s].offset;


#if VERBOSE
    logerror("d88image seek track:%d head:%d sector:%d-> offset #0x%08lX\n",
             t, h, s, offset);
#endif

	if (offset > w->image_size)
	{
		logerror("d88image seek offset %ld >= %ld\n", offset, w->image_size);
		return 0;
	}

	if (image_fseek(w->image_file, offset, SEEK_SET) < 0)
	{
		logerror("d88image seek failed\n");
		return 0;
	}

	return 1;
}

static void d88image_get_id_callback(mess_image *img, chrn_id *id, int id_index, int side)
{
	d88image *w = get_d88image(img);
	d88sect *s = &(w->sects[w->track*2+side][id_index]);

	/* construct a id value */
	id->C = s->C;
	id->H = s->H;
	id->R = s->R;
	id->N = s->N;
	id->data_id = id_index;
	id->flags = s->flags;
}

static int d88image_get_sectors_per_track(mess_image *img, int side)
{
	d88image *w = get_d88image(img);

	/* attempting to access an invalid side or track? */
	if ((side>=2) || (w->track>=D88_NUM_TRACK/2))
	{
		/* no sectors */
		return 0;
	}
	/* return number of sectors per track */
	return w->num_sects[w->track*2+side];
}

static void d88image_seek_callback(mess_image *img, int physical_track)
{
	d88image *w = get_d88image(img);
	w->track = physical_track;
}

static void d88image_write_sector_data_from_buffer(mess_image *img, int side, int index1, const char *ptr, int length, int ddam)
{
	d88image *w = get_d88image(img);
	d88sect *s = &(w->sects[w->track*2+side][index1]);

	if (d88image_seek(w, w->track, side, index1))
	{
		image_fwrite(w->image_file, ptr, length);
	}

	s->flags = ddam ? ID_FLAG_DELETED_DATA : 0;
}

static void d88image_read_sector_data_into_buffer(mess_image *img, int side, int index1, char *ptr, int length)
{
	d88image *w = get_d88image(img);

	if (d88image_seek(w, w->track, side, index1))
	{
		image_fread(w->image_file, ptr, length);
	}
}


