/*
	This code handles the floppy drives.
	All FDD actions should be performed using these functions.

	The functions are emulated and a disk image is used.

  Disk image operation:
  - set disk image functions using floppy_drive_set_disk_image_interface

  Real disk operation:
  - set unit id

  TODO:
	- Disk change handling.
	- Override write protect if disk image has been opened in read mode
*/

#include "driver.h"
#include "flopdrv.h"


#define VERBOSE		0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)
#define FLOPDRVTAG	"flopdrv"

static struct floppy_drive *get_drive(const device_config *img)
{
	return image_lookuptag(img, FLOPDRVTAG);
}

static TIMER_CALLBACK(floppy_drive_index_callback);

/* this is called on device init */
void floppy_drive_init(const device_config *img, const floppy_interface *iface)
{
	struct floppy_drive *pDrive;

	assert(image_slotexists(img));

	pDrive = image_alloctag(img, FLOPDRVTAG, sizeof(struct floppy_drive));

	/* initialise flags */
	pDrive->flags = 0;
	pDrive->index_pulse_callback = NULL;
	pDrive->ready_state_change_callback = NULL;
	pDrive->index_timer = timer_alloc(img->machine, floppy_drive_index_callback, (void *) img);
	pDrive->index = 0;

	/* all drives are double-sided 80 track - can be overriden in driver! */
	floppy_drive_set_geometry(img, FLOPPY_DRIVE_DS_80);

	/* initialise id index - not so important */
	pDrive->id_index = 0;
	/* initialise track */
	pDrive->current_track = 1;

	/* default RPM */
	pDrive->rpm = 300;

	pDrive->controller = NULL;
	
	floppy_drive_set_disk_image_interface(img, iface);
}



/* index pulses at rpm/60 Hz, and stays high 1/20th of time */
static void floppy_drive_index_func(const device_config *img)
{
	struct floppy_drive *pDrive = get_drive(img);

	double ms = 1000. / (pDrive->rpm / 60.);

	if (pDrive->index)
	{
		pDrive->index = 0;
		timer_adjust_oneshot(pDrive->index_timer, double_to_attotime(ms*19/20/1000.0), 0);
	}
	else
	{
		pDrive->index = 1;
		timer_adjust_oneshot(pDrive->index_timer, double_to_attotime(ms/20/1000.0), 0);
	}

	if (pDrive->index_pulse_callback)
		pDrive->index_pulse_callback(pDrive->controller, img, pDrive->index);
}



static TIMER_CALLBACK(floppy_drive_index_callback)
{
	const device_config *image = (const device_config *) ptr;
	floppy_drive_index_func(image);
}



/* set the callback for the index pulse */
void floppy_drive_set_index_pulse_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *image, int state))
{
	struct floppy_drive *pDrive = get_drive(img);
	pDrive->index_pulse_callback = callback;
}


void floppy_drive_set_ready_state_change_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *img, int state))
{
	struct floppy_drive *pDrive = get_drive(img);
	pDrive->ready_state_change_callback = callback;
}

/*************************************************************************/
/* IO_FLOPPY device functions */

/* return and set current status
  use for setting:-
  1) write protect/enable
  2) drive present/missing
*/

int	floppy_status(const device_config *img, int new_status)
{
	/* return current status only? */
	if (new_status!=-1)
	{
		/* we don't set the flags directly.
		The flags are "cooked" when we do a floppy_drive_get_flag_state depending on
		if drive is connected etc. So if we wrote the flags back it would
		corrupt this information. Therefore we update the flags depending on new_status */

		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_DISK_WRITE_PROTECTED, (new_status & FLOPPY_DRIVE_DISK_WRITE_PROTECTED));
	}

	/* return current status */
	return floppy_drive_get_flag_state(img,0x0ff);
}

/* set interface for image interface */
void floppy_drive_set_disk_image_interface(const device_config *img, const floppy_interface *iface)
{
	struct floppy_drive *pDrive = get_drive(img);
	if (iface)
		memcpy(&pDrive->interface_, iface, sizeof(floppy_interface));
	else
		memset(&pDrive->interface_, 0, sizeof(floppy_interface));
}

/* set flag state */
void floppy_drive_set_flag_state(const device_config *img, int flag, int state)
{
	struct floppy_drive *drv = get_drive(img);
	int prev_state;
	int new_state;

	/* get old state */
	prev_state = drv->flags & flag;

	/* set new state */
	drv->flags &= ~flag;
	if (state)
		drv->flags |= flag;

	/* get new state */
	new_state = drv->flags & flag;

	/* changed state? */
	if (prev_state ^ new_state)
	{
		if (flag & FLOPPY_DRIVE_READY)
		{
			/* trigger state change callback */
			if (drv->ready_state_change_callback)
				drv->ready_state_change_callback(drv->controller, img, new_state);
		}
	}
}

void floppy_drive_set_motor_state(const device_config *img, int state)
{
	int new_motor_state = 0;
	int previous_state = 0;

	/* previous state */
	if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_MOTOR_ON))
		previous_state = 1;

	/* calc new state */

	/* drive present? */
	if (image_slotexists(img))
	{
		/* disk inserted? */
		if (image_exists(img))
		{
			/* drive present and disc inserted */

			/* state of motor is same as the programmed state */
			if (state)
			{
				new_motor_state = 1;
			}
		}
	}

	if ((new_motor_state^previous_state)!=0)
	{
		/* if timer already setup remove it */
		if (image_slotexists(img))
		{
			struct floppy_drive *pDrive = get_drive(img);

			pDrive->index = 0;
			if (new_motor_state)
			{
				/* off->on */
				/* check it's in range */

				/* setup timer to trigger at rpm */
				floppy_drive_index_func(img);
			}
			else
			{
				/* on->off */
				timer_adjust_oneshot(pDrive->index_timer, attotime_zero, 0);
			}
		}
	}

	floppy_drive_set_flag_state(img, FLOPPY_DRIVE_MOTOR_ON, new_motor_state);

}

/* for pc, drive is always ready, for amstrad,pcw,spectrum it is only ready under
a fixed set of circumstances */
/* use this to set ready state of drive */
void floppy_drive_set_ready_state(const device_config *img, int state, int flag)
{
	if (flag)
	{
		/* set ready only if drive is present, disk is in the drive,
		and disk motor is on - for Amstrad, Spectrum and PCW*/

		/* drive present? */
		if (image_slotexists(img))
		{
			/* disk inserted? */
			if (image_exists(img))
			{
				if (floppy_drive_get_flag_state(img, FLOPPY_DRIVE_MOTOR_ON))
				{

					/* set state */
					floppy_drive_set_flag_state(img, FLOPPY_DRIVE_READY, state);
                    return;
				}
			}
		}

		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_READY, 0);
	}
	else
	{
		/* force ready state - for PC driver */
		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_READY, state);
	}

}


/* get flag state */
int	floppy_drive_get_flag_state(const device_config *img, int flag)
{
	struct floppy_drive *drv = get_drive(img);
	int drive_flags;
	int flags;

	flags = 0;

	drive_flags = drv->flags;

	/* these flags are independant of a real drive/disk image */
    flags |= drive_flags & (FLOPPY_DRIVE_READY | FLOPPY_DRIVE_MOTOR_ON | FLOPPY_DRIVE_INDEX);

	flags |= drive_flags & FLOPPY_DRIVE_HEAD_AT_TRACK_0;

	/* if disk image is read-only return write protected all the time */
	if (!image_is_writable(img))
	{
		flags |= FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
	}
	else
	{
		/* return real state of write protected flag */
		flags |= drive_flags & FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
	}

	/* drive present not */
	if (!image_slotexists(img))
	{
		/* adjust some flags if drive is not present */
		flags &= ~FLOPPY_DRIVE_HEAD_AT_TRACK_0;
		flags |= FLOPPY_DRIVE_DISK_WRITE_PROTECTED;
	}

    flags &= flag;

	return flags;
}


void floppy_drive_set_geometry(const device_config *img, floppy_type type)
{
	int max_track, num_sides;

	switch (type) {
	case FLOPPY_DRIVE_SS_40:	/* single sided, 40 track drive e.g. Amstrad CPC internal 3" drive */
		max_track = 42;
		num_sides = 1;
		break;

	case FLOPPY_DRIVE_DS_80:
		max_track = 83;
		num_sides = 2;
		break;

	default:
		assert(0);
		return;
	}
	floppy_drive_set_geometry_absolute(img, max_track, num_sides);
}

void floppy_drive_set_geometry_absolute(const device_config *img, int tracks, int sides)
{
	struct floppy_drive *pDrive = get_drive(img);
	pDrive->max_track = tracks;
	pDrive->num_sides = sides;
}

void floppy_drive_seek(const device_config *img, signed int signed_tracks)
{
	struct floppy_drive *pDrive;

	pDrive = get_drive(img);

	LOG(("seek from: %d delta: %d\n",pDrive->current_track, signed_tracks));

	/* update position */
	pDrive->current_track+=signed_tracks;

	if (pDrive->current_track<0)
	{
		pDrive->current_track = 0;
	}
	else
	if (pDrive->current_track>=pDrive->max_track)
	{
		pDrive->current_track = pDrive->max_track-1;
	}

	/* set track 0 flag */
	pDrive->flags &= ~FLOPPY_DRIVE_HEAD_AT_TRACK_0;

	if (pDrive->current_track==0)
	{
		pDrive->flags |= FLOPPY_DRIVE_HEAD_AT_TRACK_0;
	}

	/* inform disk image of step operation so it can cache information */
	if (image_exists(img) && pDrive->interface_.seek_callback)
		pDrive->interface_.seek_callback(img, pDrive->current_track);

        pDrive->id_index = 0;
}


/* this is not accurate. But it will do for now */
int	floppy_drive_get_next_id(const device_config *img, int side, chrn_id *id)
{
	struct floppy_drive *pDrive;
	int spt;

	pDrive = get_drive(img);

	/* get sectors per track */
	spt = 0;
	if (pDrive->interface_.get_sectors_per_track)
		spt = pDrive->interface_.get_sectors_per_track(img, side);

	/* set index */
	if ((pDrive->id_index==(spt-1)) || (spt==0))
	{
		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_INDEX, 1);
	}
	else
	{
		floppy_drive_set_flag_state(img, FLOPPY_DRIVE_INDEX, 0);
	}

	/* get id */
	if (spt!=0)
	{
		if (pDrive->interface_.get_id_callback)
			pDrive->interface_.get_id_callback(img, id, pDrive->id_index, side);
	}

	pDrive->id_index++;
	if (spt!=0)
		pDrive->id_index %= spt;
	else
		pDrive->id_index = 0;

	return (spt == 0) ? 0 : 1;
}

int	floppy_drive_get_current_track(const device_config *img)
{
	struct floppy_drive *drv = get_drive(img);
	return drv->current_track;
}

void floppy_drive_read_track_data_info_buffer(const device_config *img, int side, void *ptr, int *length )
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.read_track_data_info_buffer)
			drv->interface_.read_track_data_info_buffer(img, side, ptr, length);
	}
}

void floppy_drive_write_track_data_info_buffer(const device_config *img, int side, const void *ptr, int *length )
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.write_track_data_info_buffer)
			drv->interface_.write_track_data_info_buffer(img, side, ptr, length);
	}
}

void floppy_drive_format_sector(const device_config *img, int side, int sector_index,int c,int h, int r, int n, int filler)
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.format_sector)
			drv->interface_.format_sector(img, side, sector_index,c, h, r, n, filler);
	}
}

void floppy_drive_read_sector_data(const device_config *img, int side, int index1, void *pBuffer, int length)
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.read_sector_data_into_buffer)
			drv->interface_.read_sector_data_into_buffer(img, side, index1, pBuffer,length);
	}
}

void floppy_drive_write_sector_data(const device_config *img, int side, int index1, const void *pBuffer,int length, int ddam)
{
	if (image_exists(img))
	{
		struct floppy_drive *drv = get_drive(img);
		if (drv->interface_.write_sector_data_from_buffer)
			drv->interface_.write_sector_data_from_buffer(img, side, index1, pBuffer,length,ddam);
	}
}

int floppy_drive_get_datarate_in_us(DENSITY density)
{
	int usecs;
	/* 64 for single density */
	switch (density)
	{
		case DEN_FM_LO:
		{
			usecs = 128;
		}
		break;

		case DEN_FM_HI:
		{
			usecs = 64;
		}
		break;

		default:
		case DEN_MFM_LO:
		{
			usecs = 32;
		}
		break;

		case DEN_MFM_HI:
		{
			usecs = 16;
		}
		break;
	}

	return usecs;
}



void floppy_drive_set_rpm(const device_config *img, float rpm)
{
	struct floppy_drive *drv = get_drive(img);
	drv->rpm = rpm;
}

void floppy_drive_set_controller(const device_config *img, const device_config *controller)
{
	struct floppy_drive *drv = get_drive(img);
	drv->controller = controller;
}



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define FLOPPY_TAG		"floptag"
#define LOG_FLOPPY		0



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mess_flopimg mess_flopimg;
struct _mess_flopimg
{
	floppy_image *floppy;
	int track;
	void (*load_proc)(const device_config *image);
	void (*unload_proc)(const device_config *image);
	int (*tracktranslate_proc)(const device_config *image, floppy_image *floppy, int physical_track);
};



typedef struct _floppy_error_map floppy_error_map;
struct _floppy_error_map
{
	floperr_t ferr;
	image_error_t ierr;
	const char *message;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static const floppy_error_map errmap[] =
{
	{ FLOPPY_ERROR_SUCCESS,			IMAGE_ERROR_SUCCESS },
	{ FLOPPY_ERROR_INTERNAL,		IMAGE_ERROR_INTERNAL },
	{ FLOPPY_ERROR_UNSUPPORTED,		IMAGE_ERROR_UNSUPPORTED },
	{ FLOPPY_ERROR_OUTOFMEMORY,		IMAGE_ERROR_OUTOFMEMORY },
	{ FLOPPY_ERROR_INVALIDIMAGE,	IMAGE_ERROR_INVALIDIMAGE }
};

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static mess_flopimg *get_flopimg(const device_config *image)
{
	return (mess_flopimg *) image_lookuptag(image, FLOPPY_TAG);
}



floppy_image *flopimg_get_image(const device_config *image)
{
	return get_flopimg(image)->floppy;
}



static void flopimg_seek_callback(const device_config *image, int physical_track)
{
	mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	/* translate the track number if necessary */
	if (flopimg->tracktranslate_proc)
		physical_track = flopimg->tracktranslate_proc(image, flopimg->floppy, physical_track);

	flopimg->track = physical_track;
}



static int flopimg_get_sectors_per_track(const device_config *image, int side)
{
	mess_flopimg *flopimg;
	floperr_t err;
	int sector_count;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return 0;

	err = floppy_get_sector_count(flopimg->floppy, side, flopimg->track, &sector_count);
	if (err)
		return 0;
	return sector_count;
}



static void flopimg_get_id_callback(const device_config *image, chrn_id *id, int id_index, int side)
{
	mess_flopimg *flopimg;
	int cylinder, sector, N;
	unsigned long flags;
	UINT32 sector_length;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_get_indexed_sector_info(flopimg->floppy, side, flopimg->track, id_index, &cylinder, &side, &sector, &sector_length, &flags);

	N = compute_log2(sector_length);

	id->C = cylinder;
	id->H = side;
	id->R = sector;
	id->data_id = id_index;
	id->flags = flags;
	id->N = ((N >= 7) && (N <= 10)) ? N - 7 : 0;
}



static void log_readwrite(const char *name, int head, int track, int sector, const char *buf, int length)
{
	char membuf[1024];
	int i;
	for (i = 0; i < length; i++)
		sprintf(membuf + i*2, "%02x", (int) (UINT8) buf[i]);
	logerror("%s:  head=%i track=%i sector=%i buffer='%s'\n", name, head, track, sector, membuf);
}



static void flopimg_read_sector_data_into_buffer(const device_config *image, int side, int index1, char *ptr, int length)
{
	mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_read_indexed_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length);

	if (LOG_FLOPPY)
		log_readwrite("sector_read", side, flopimg->track, index1, ptr, length);
}



static void flopimg_write_sector_data_from_buffer(const device_config *image, int side, int index1, const char *ptr, int length, int ddam)
{
	mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	if (LOG_FLOPPY)
		log_readwrite("sector_write", side, flopimg->track, index1, ptr, length);

	floppy_write_indexed_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length, ddam);
}



static void flopimg_read_track_data_info_buffer(const device_config *image, int side, void *ptr, int *length)
{
	mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_read_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
}



static void flopimg_write_track_data_info_buffer(const device_config *image, int side, const void *ptr, int *length)
{
	mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_write_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
}



static const floppy_interface mess_floppy_interface =
{
	flopimg_seek_callback,
	flopimg_get_sectors_per_track,
	flopimg_get_id_callback,
	flopimg_read_sector_data_into_buffer,
	flopimg_write_sector_data_from_buffer,
	flopimg_read_track_data_info_buffer,
	flopimg_write_track_data_info_buffer,
	NULL
};

/* ----------------------------------------------------------------------- */

static int image_fseek_thunk(void *file, INT64 offset, int whence)
{
	return image_fseek((const device_config *) file, offset, whence);
}

static size_t image_fread_thunk(void *file, void *buffer, size_t length)
{
	return image_fread((const device_config *) file, buffer, length);
}

static size_t image_fwrite_thunk(void *file, const void *buffer, size_t length)
{
	return image_fwrite((const device_config *) file, buffer, length);
}

static UINT64 image_fsize_thunk(void *file)
{
	return image_length((const device_config *) file);
}

/* ----------------------------------------------------------------------- */

const struct io_procs mess_ioprocs =
{
	NULL,
	image_fseek_thunk,
	image_fread_thunk,
	image_fwrite_thunk,
	image_fsize_thunk
};




/* ----------------------------------------------------------------------- */


static DEVICE_START( floppy )
{
    image_alloctag(device, FLOPPY_TAG, sizeof(mess_flopimg));
	floppy_drive_init(device, &mess_floppy_interface);
}



static int internal_floppy_device_load(const device_config *image, int create_format, option_resolution *create_args)
{
	floperr_t err;
	mess_flopimg *flopimg;
	const mess_device_class *devclass;
	const struct FloppyFormat *floppy_options;
	int floppy_flags, i;
	const char *extension;
	int keep_geometry = 0;

	/* look up instance data */
	flopimg = get_flopimg(image);

	/* figure out the floppy options */
	devclass = mess_devclass_from_core_device(image);
	floppy_options = mess_device_get_info_ptr(devclass, MESS_DEVINFO_PTR_FLOPPY_OPTIONS);

	if (image_has_been_created(image))
	{
		/* creating an image */
		assert(create_format >= 0);
		err = floppy_create((void *) image, &mess_ioprocs, &floppy_options[create_format], create_args, &flopimg->floppy);
		if (err)
			goto error;
	}
	else
	{
		/* opening an image */
		floppy_flags = image_is_writable(image) ? FLOPPY_FLAGS_READWRITE : FLOPPY_FLAGS_READONLY;
		extension = image_filetype(image);
		err = floppy_open_choices((void *) image, &mess_ioprocs, extension, floppy_options, floppy_flags, &flopimg->floppy);
		if (err)
			goto error;
	}
        
	/* if we can get head and track counts, then set the geometry accordingly
	   However, at least the ti99 system family requires that medium track 
	   count and drive track count be handled separately. 
	   It is possible to insert a 40 track medium in an 80 track drive; the 
	   TI controllers read the track count from sector 0 and automatically 
	   apply double steps. Setting the track count of the drive to the 
	   medium track count will then lead to unreachable tracks.
	*/
	keep_geometry = (int)mess_device_get_info_int(mess_devclass_from_core_device(image), MESS_DEVINFO_INT_KEEP_DRIVE_GEOMETRY);

	if (!keep_geometry 
            && floppy_callbacks(flopimg->floppy)->get_heads_per_disk
		&& floppy_callbacks(flopimg->floppy)->get_tracks_per_disk)
	{
		floppy_drive_set_geometry_absolute(image,
			floppy_get_tracks_per_disk(flopimg->floppy),
			floppy_get_heads_per_disk(flopimg->floppy));
	}
	return INIT_PASS;

error:
	for (i = 0; i < sizeof(errmap) / sizeof(errmap[0]); i++)
	{
		if (err == errmap[i].ferr)
			image_seterror(image, errmap[i].ierr, errmap[i].message);
	}
	return INIT_FAIL;
}



static DEVICE_IMAGE_LOAD( floppy )
{
	mess_flopimg *flopimg;
	int retVal = internal_floppy_device_load(image, -1, NULL);
	flopimg = image_lookuptag(image, FLOPPY_TAG);
	if (retVal==INIT_PASS) {
		/* if we have one of our hacky unload procs, call it */
		if (flopimg->load_proc)
			flopimg->load_proc(image);
	}
	return retVal;
}



static DEVICE_IMAGE_CREATE( floppy )
{
	return internal_floppy_device_load(image, create_format, create_args);	
}



static DEVICE_IMAGE_UNLOAD( floppy )
{
	mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);

	/* if we have one of our hacky unload procs, call it */
	if (flopimg->unload_proc)
		flopimg->unload_proc(image);

	floppy_close(flopimg->floppy);
	flopimg->floppy = NULL;
}



/*************************************
 *
 *	Hacks for specific systems
 *
 *************************************/

void floppy_install_load_proc(const device_config *image, void (*proc)(const device_config *image))
{
	mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);
	flopimg->load_proc = proc;
}

void floppy_install_unload_proc(const device_config *image, void (*proc)(const device_config *image))
{
	mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);
	flopimg->unload_proc = proc;
}



void floppy_install_tracktranslate_proc(const device_config *image, int (*proc)(const device_config *image, floppy_image *floppy, int physical_track))
{
	mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);
	flopimg->tracktranslate_proc = proc;
}

/*************************************
 *
 *	Device specification function
 *
 *************************************/

void floppy_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	char *s;
	int i, count;
	const struct FloppyFormat *floppy_options;

	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:				info->i = IO_FLOPPY; break;
		case MESS_DEVINFO_INT_READABLE:			info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:			info->i = 1; break;
		case MESS_DEVINFO_INT_CREATABLE:
			floppy_options = mess_device_get_info_ptr(devclass, MESS_DEVINFO_PTR_FLOPPY_OPTIONS);
			info->i = floppy_options->param_guidelines ? 1 : 0;
			break;

		case MESS_DEVINFO_INT_CREATE_OPTCOUNT:
			/* count total floppy options */
			floppy_options = mess_device_get_info_ptr(devclass, MESS_DEVINFO_PTR_FLOPPY_OPTIONS);
			for (count = 0; floppy_options[count].construct; count++)
				;
			info->i = count;
			break;

		case MESS_DEVINFO_INT_KEEP_DRIVE_GEOMETRY:
			info->i = 0;
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			/* retrieve the floppy options */
			floppy_options = mess_device_get_info_ptr(devclass, MESS_DEVINFO_PTR_FLOPPY_OPTIONS);

			/* set up a temporary string */
			s = device_temp_str();
			info->s = s;
			s[0] = '\0';

			/* append each of the extensions */
			for (i = 0; floppy_options[i].construct; i++)
				specify_extension(s, 256, floppy_options[i].extensions);
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:				info->start = DEVICE_START_NAME(floppy); break;
		case MESS_DEVINFO_PTR_LOAD:				info->load = DEVICE_IMAGE_LOAD_NAME(floppy); break;
		case MESS_DEVINFO_PTR_CREATE:			info->create = DEVICE_IMAGE_CREATE_NAME(floppy); break;
		case MESS_DEVINFO_PTR_UNLOAD:			info->unload = DEVICE_IMAGE_UNLOAD_NAME(floppy); break;
		case MESS_DEVINFO_PTR_CREATE_OPTGUIDE:	info->p = (void *) floppy_option_guide; break;

		default:
			floppy_options = mess_device_get_info_ptr(devclass, MESS_DEVINFO_PTR_FLOPPY_OPTIONS);
			if ((state >= MESS_DEVINFO_STR_CREATE_OPTNAME) && (state < MESS_DEVINFO_STR_CREATE_OPTNAME + DEVINFO_CREATE_OPTMAX))
			{
				info->s = (void *) floppy_options[state - MESS_DEVINFO_STR_CREATE_OPTNAME].name;
			}
			else if ((state >= MESS_DEVINFO_STR_CREATE_OPTDESC) && (state < MESS_DEVINFO_STR_CREATE_OPTDESC + DEVINFO_CREATE_OPTMAX))
			{
				info->s = (void *) floppy_options[state - MESS_DEVINFO_STR_CREATE_OPTDESC].description;
			}
			else if ((state >= MESS_DEVINFO_STR_CREATE_OPTEXTS) && (state < MESS_DEVINFO_STR_CREATE_OPTEXTS + DEVINFO_CREATE_OPTMAX))
			{
				info->s = (void *) floppy_options[state - MESS_DEVINFO_STR_CREATE_OPTEXTS].extensions;
			}
			else if ((state >= MESS_DEVINFO_PTR_CREATE_OPTSPEC) && (state < MESS_DEVINFO_PTR_CREATE_OPTSPEC + DEVINFO_CREATE_OPTMAX))
			{
				info->p = (void *) floppy_options[state - MESS_DEVINFO_PTR_CREATE_OPTSPEC].param_guidelines;
			}
			break;
	}
}

