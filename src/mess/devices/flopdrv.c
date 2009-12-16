/*
    This code handles the floppy drives.
    All FDD actions should be performed using these functions.

    The functions are emulated and a disk image is used.

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

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define FLOPDRVTAG	"flopdrv"
#define LOG_FLOPPY		0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _floppy_drive floppy_drive;
struct _floppy_drive
{
	/* callbacks */
	devcb_resolved_write_line out_idx_func;
	devcb_resolved_read_line in_mon_func;
	devcb_resolved_write_line out_tk00_func;
	devcb_resolved_write_line out_wpt_func;
	devcb_resolved_write_line out_rdy_func;

	/* state of input lines */
	int drtn; /* direction */
	int stp;  /* step */
	int wtg;  /* write gate */
	int mon;  /* motor on */

	/* state of output lines */
	int idx;  /* index pulse */
	int tk00; /* track 00 */
	int wpt;  /* write protect */
	int rdy;  /* ready */

	/* drive select logic */
	int drive_id;
	int active;

	const floppy_config	*config;

	/* flags */
	int flags;
	/* maximum track allowed */
	int max_track;
	/* num sides */
	int num_sides;
	/* current track - this may or may not relate to the present cylinder number
    stored by the fdc */
	int current_track;

	/* index pulse timer */
	void	*index_timer;
	/* index pulse callback */
	void	(*index_pulse_callback)(const device_config *controller,const device_config *image, int state);
	/* rotation per minute => gives index pulse frequency */
	float rpm;

	void	(*ready_state_change_callback)(const device_config *controller,const device_config *img, int state);

	int id_index;

	const device_config *controller;

	floppy_image *floppy;
	int track;
	void (*load_proc)(const device_config *image);
	void (*unload_proc)(const device_config *image);
	int (*tracktranslate_proc)(const device_config *image, floppy_image *floppy, int physical_track);
	void *custom_data;
	int floppy_drive_type;
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
extern DEVICE_GET_INFO(apple525);
extern DEVICE_GET_INFO(sonydriv);

INLINE floppy_drive *get_safe_token(const device_config *device)
{
	assert( device != NULL );
	assert( device->token != NULL );
	assert( device->type == DEVICE_GET_INFO_NAME(floppy) || device->type == DEVICE_GET_INFO_NAME(apple525) || device->type == DEVICE_GET_INFO_NAME(sonydriv));
	return (floppy_drive *) device->token;
}

floppy_image *flopimg_get_image(const device_config *image)
{
	return get_safe_token(image)->floppy;
}

void *flopimg_get_custom_data(const device_config *image)
{
	floppy_drive *flopimg = get_safe_token( image );
	return flopimg->custom_data;
}

void flopimg_alloc_custom_data(const device_config *image,void *custom)
{
	floppy_drive *flopimg = get_safe_token( image );
	flopimg->custom_data = custom;
}

static void flopimg_seek_callback(const device_config *image, int physical_track)
{
	floppy_drive *flopimg = get_safe_token( image );
	if (!flopimg || !flopimg->floppy)
		return;

	/* translate the track number if necessary */
	if (flopimg->tracktranslate_proc)
		physical_track = flopimg->tracktranslate_proc(image, flopimg->floppy, physical_track);

	flopimg->track = physical_track;
}

static int flopimg_get_sectors_per_track(const device_config *image, int side)
{
	floperr_t err;
	int sector_count;
	floppy_drive *flopimg = get_safe_token( image );

	if (!flopimg || !flopimg->floppy)
		return 0;

	err = floppy_get_sector_count(flopimg->floppy, side, flopimg->track, &sector_count);
	if (err)
		return 0;
	return sector_count;
}

static void flopimg_get_id_callback(const device_config *image, chrn_id *id, int id_index, int side)
{
	floppy_drive *flopimg;
	int cylinder, sector, N;
	unsigned long flags;
	UINT32 sector_length;

	flopimg = get_safe_token( image );
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

static void floppy_drive_set_geometry_absolute(const device_config *img, int tracks, int sides)
{
	floppy_drive *pDrive = get_safe_token( img );
	pDrive->max_track = tracks;
	pDrive->num_sides = sides;
}

void floppy_drive_set_geometry(const device_config *img, floppy_type type)
{
	int max_track, num_sides;

	switch (type) {
	case FLOPPY_DRIVE_SS_40:	/* single sided, 40 track drive e.g. Amstrad CPC internal 3" drive */
		max_track = 42;
		num_sides = 1;
		break;

	case FLOPPY_DRIVE_DS_40:	/* double sided, 40 track drive */
		max_track = 42;
		num_sides = 2;
		break;

	case FLOPPY_DRIVE_SS_80:	/* single sided, 80 track drive */
		max_track = 83;
		num_sides = 1;
		break;

	case FLOPPY_DRIVE_DS_80:	/* double sided, 80 track drive */
		max_track = 83;
		num_sides = 2;
		break;

	default:
		assert(0);
		return;
	}
	floppy_drive_set_geometry_absolute(img, max_track, num_sides);
}

static TIMER_CALLBACK(floppy_drive_index_callback);

/* this is called on device init */
static void floppy_drive_init(const device_config *img)
{
	floppy_drive *pDrive = get_safe_token( img );

	/* initialise flags */
	pDrive->flags = 0;
	pDrive->index_pulse_callback = NULL;
	pDrive->ready_state_change_callback = NULL;
	pDrive->index_timer = timer_alloc(img->machine, floppy_drive_index_callback, (void *) img);
	pDrive->idx = 0;

	floppy_drive_set_geometry(img, ((floppy_config*)img->static_config)->floppy_type);

	/* initialise id index - not so important */
	pDrive->id_index = 0;
	/* initialise track */
	pDrive->current_track = 1;

	/* default RPM */
	pDrive->rpm = 300;

	pDrive->controller = NULL;

	pDrive->custom_data = NULL;

	pDrive->floppy_drive_type = FLOPPY_TYPE_REGULAR;
}

/* index pulses at rpm/60 Hz, and stays high 1/20th of time */
static void floppy_drive_index_func(const device_config *img)
{
	floppy_drive *drive = get_safe_token( img );

	double ms = 1000.0 / (drive->rpm / 60.0);

	if (drive->idx)
	{
		drive->idx = 0;
		timer_adjust_oneshot(drive->index_timer, double_to_attotime(ms*19/20/1000.0), 0);
	}
	else
	{
		drive->idx = 1;
		timer_adjust_oneshot(drive->index_timer, double_to_attotime(ms/20/1000.0), 0);
	}

	devcb_call_write_line(&drive->out_idx_func, drive->idx);

	if (drive->index_pulse_callback)
		drive->index_pulse_callback(drive->controller, img, drive->idx);
}

static TIMER_CALLBACK(floppy_drive_index_callback)
{
	const device_config *image = (const device_config *) ptr;
	floppy_drive_index_func(image);
}

/*************************************************************************/
/* IO_FLOPPY device functions */

/* set flag state */
void floppy_drive_set_flag_state(const device_config *img, int flag, int state)
{
	floppy_drive *drv = get_safe_token( img );
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
			devcb_call_write_line(&drv->out_rdy_func, new_state ? ASSERT_LINE : CLEAR_LINE);

			if (drv->ready_state_change_callback)
				drv->ready_state_change_callback(drv->controller, img, new_state);
		}
	}
}

/* for pc, drive is always ready, for amstrad,pcw,spectrum it is only ready under
a fixed set of circumstances */
/* use this to set ready state of drive */
void floppy_drive_set_ready_state(const device_config *img, int state, int flag)
{
	floppy_drive *drive = get_safe_token(img);

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
				if (drive->mon == CLEAR_LINE)
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
	floppy_drive *drv = get_safe_token( img );
	int drive_flags;
	int flags;

	flags = 0;

	drive_flags = drv->flags;

	/* these flags are independant of a real drive/disk image */
    flags |= drive_flags & (FLOPPY_DRIVE_READY | FLOPPY_DRIVE_INDEX);

    flags &= flag;

	return flags;
}


void floppy_drive_seek(const device_config *img, signed int signed_tracks)
{
	floppy_drive *pDrive;

	pDrive = get_safe_token( img );

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
	pDrive->tk00 = (pDrive->current_track == 0) ? CLEAR_LINE : ASSERT_LINE;
	devcb_call_write_line(&pDrive->out_tk00_func, pDrive->tk00);

	/* inform disk image of step operation so it can cache information */
	if (image_exists(img))
		flopimg_seek_callback(img, pDrive->current_track);

        pDrive->id_index = 0;
}


/* this is not accurate. But it will do for now */
int	floppy_drive_get_next_id(const device_config *img, int side, chrn_id *id)
{
	floppy_drive *pDrive;
	int spt;

	pDrive = get_safe_token( img );

	/* get sectors per track */
	spt = flopimg_get_sectors_per_track(img, side);

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
		flopimg_get_id_callback(img, id, pDrive->id_index, side);
	}

	pDrive->id_index++;
	if (spt!=0)
		pDrive->id_index %= spt;
	else
		pDrive->id_index = 0;

	return (spt == 0) ? 0 : 1;
}

void floppy_drive_read_track_data_info_buffer(const device_config *img, int side, void *ptr, int *length )
{
	floppy_drive *flopimg;
	if (image_exists(img))
	{
		flopimg = get_safe_token( img );
		if (!flopimg || !flopimg->floppy)
			return;

		floppy_read_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
	}
}

void floppy_drive_write_track_data_info_buffer(const device_config *img, int side, const void *ptr, int *length )
{
	floppy_drive *flopimg;
	if (image_exists(img))
	{
		flopimg = get_safe_token( img );
		if (!flopimg || !flopimg->floppy)
			return;

		floppy_write_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
	}
}

void floppy_drive_format_sector(const device_config *img, int side, int sector_index,int c,int h, int r, int n, int filler)
{
	if (image_exists(img))
	{
/*      if (drv->interface_.format_sector)
            drv->interface_.format_sector(img, side, sector_index,c, h, r, n, filler);*/
	}
}

void floppy_drive_read_sector_data(const device_config *img, int side, int index1, void *ptr, int length)
{
	floppy_drive *flopimg;
	if (image_exists(img))
	{
		flopimg = get_safe_token( img );
		if (!flopimg || !flopimg->floppy)
			return;

		floppy_read_indexed_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length);

		if (LOG_FLOPPY)
			log_readwrite("sector_read", side, flopimg->track, index1, ptr, length);

	}
}

void floppy_drive_write_sector_data(const device_config *img, int side, int index1, const void *ptr,int length, int ddam)
{
	floppy_drive *flopimg;
	if (image_exists(img))
	{
		flopimg = get_safe_token( img );
		if (!flopimg || !flopimg->floppy)
			return;

		if (LOG_FLOPPY)
			log_readwrite("sector_write", side, flopimg->track, index1, ptr, length);

		floppy_write_indexed_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length, ddam);
	}
}

void floppy_install_load_proc(const device_config *image, void (*proc)(const device_config *image))
{
	floppy_drive *flopimg = get_safe_token( image );
	flopimg->load_proc = proc;
}

void floppy_install_unload_proc(const device_config *image, void (*proc)(const device_config *image))
{
	floppy_drive *flopimg = get_safe_token( image );
	flopimg->unload_proc = proc;
}

void floppy_install_tracktranslate_proc(const device_config *image, int (*proc)(const device_config *image, floppy_image *floppy, int physical_track))
{
	floppy_drive *flopimg = get_safe_token( image );
	flopimg->tracktranslate_proc = proc;
}

/* set the callback for the index pulse */
void floppy_drive_set_index_pulse_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *image, int state))
{
	floppy_drive *pDrive = get_safe_token( img );
	pDrive->index_pulse_callback = callback;
}


void floppy_drive_set_ready_state_change_callback(const device_config *img, void (*callback)(const device_config *controller,const device_config *img, int state))
{
	floppy_drive *pDrive = get_safe_token( img );
	pDrive->ready_state_change_callback = callback;
}

int	floppy_drive_get_current_track(const device_config *img)
{
	floppy_drive *drv = get_safe_token( img );
	return drv->current_track;
}

void floppy_drive_set_rpm(const device_config *img, float rpm)
{
	floppy_drive *drv = get_safe_token( img );
	drv->rpm = rpm;
}

void floppy_drive_set_controller(const device_config *img, const device_config *controller)
{
	floppy_drive *drv = get_safe_token( img );
	drv->controller = controller;
}

/* ----------------------------------------------------------------------- */
DEVICE_START( floppy )
{
	floppy_drive *floppy = get_safe_token( device );
	floppy->config = device->static_config;
	floppy_drive_init(device);

	floppy->drive_id = floppy_get_drive(device);
	floppy->active = FALSE;

	/* by default we are write-protected */
	floppy->wpt = CLEAR_LINE;

	/* not at track 0 */
	floppy->tk00 = ASSERT_LINE;

	/* motor off */
	floppy->mon = ASSERT_LINE;

	/* resolve callbacks */
	devcb_resolve_write_line(&floppy->out_idx_func, &floppy->config->out_idx_func, device);
	devcb_resolve_read_line(&floppy->in_mon_func, &floppy->config->in_mon_func, device);
	devcb_resolve_write_line(&floppy->out_tk00_func, &floppy->config->out_tk00_func, device);
	devcb_resolve_write_line(&floppy->out_wpt_func, &floppy->config->out_wpt_func, device);
	devcb_resolve_write_line(&floppy->out_rdy_func, &floppy->config->out_rdy_func, device);
}

static int internal_floppy_device_load(const device_config *image, int create_format, option_resolution *create_args)
{
	floperr_t err;
	floppy_drive *flopimg;
	const struct FloppyFormat *floppy_options;
	int floppy_flags, i;
	const char *extension;

	/* look up instance data */
	flopimg = get_safe_token( image );

	/* figure out the floppy options */
	floppy_options = ((floppy_config*)image->static_config)->formats;

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
	if (((floppy_config*)image->static_config)->keep_drive_geometry==DO_NOT_KEEP_GEOMETRY && floppy_callbacks(flopimg->floppy)->get_heads_per_disk && floppy_callbacks(flopimg->floppy)->get_tracks_per_disk)
	{
		floppy_drive_set_geometry_absolute(image,
			floppy_get_tracks_per_disk(flopimg->floppy),
			floppy_get_heads_per_disk(flopimg->floppy));
	}
	return INIT_PASS;

error:
	for (i = 0; i < ARRAY_LENGTH(errmap); i++)
	{
		if (err == errmap[i].ferr)
			image_seterror(image, errmap[i].ierr, errmap[i].message);
	}
	return INIT_FAIL;
}



DEVICE_IMAGE_LOAD( floppy )
{
	floppy_drive *flopimg;
	int retVal = internal_floppy_device_load(image, -1, NULL);
	flopimg = get_safe_token( image );
	if (retVal==INIT_PASS) {
		/* if we have one of our hacky unload procs, call it */
		if (flopimg->load_proc)
			flopimg->load_proc(image);
	}
	return retVal;
}

DEVICE_IMAGE_CREATE( floppy )
{
	return internal_floppy_device_load(image, create_format, create_args);
}

DEVICE_IMAGE_UNLOAD( floppy )
{
	floppy_drive *flopimg = get_safe_token( image );
	if (flopimg->unload_proc)
		flopimg->unload_proc(image);

	floppy_close(flopimg->floppy);
	flopimg->floppy = NULL;
}

const device_config *floppy_get_device(running_machine *machine,int drive)
{
	switch(drive) {
		case 0 : return devtag_get_device(machine,FLOPPY_0);
		case 1 : return devtag_get_device(machine,FLOPPY_1);
		case 2 : return devtag_get_device(machine,FLOPPY_2);
		case 3 : return devtag_get_device(machine,FLOPPY_3);
	}
	return NULL;
}

int floppy_get_drive_type(const device_config *image)
{
	floppy_drive *flopimg = get_safe_token( image );
	return flopimg->floppy_drive_type;
}

void floppy_set_type(const device_config *image,int ftype)
{
	floppy_drive *flopimg = get_safe_token( image );
	flopimg->floppy_drive_type = ftype;
}

const device_config *floppy_get_device_by_type(running_machine *machine,int ftype,int drive)
{
	int i;
	int cnt = 0;
	for (i=0;i<4;i++) {
		const device_config *disk = floppy_get_device(machine,i);
		if (floppy_get_drive_type(disk)==ftype) {
			if (cnt==drive) {
				return disk;
			}
			cnt++;
		}
	}
	return NULL;
}

int floppy_get_drive(const device_config *image)
{
	int drive =0;
	if (strcmp(image->tag, FLOPPY_0) == 0) drive = 0;
	if (strcmp(image->tag, FLOPPY_1) == 0) drive = 1;
	if (strcmp(image->tag, FLOPPY_2) == 0) drive = 2;
	if (strcmp(image->tag, FLOPPY_3) == 0) drive = 3;
	return drive;
}

int floppy_get_drive_by_type(const device_config *image,int ftype)
{
	int i,drive =0;
	for (i=0;i<4;i++) {
		const device_config *disk = floppy_get_device(image->machine,i);
		if (floppy_get_drive_type(disk)==ftype) {
			if (image==disk) {
				return drive;
			}
			drive++;
		}
	}
	return drive;
}

int floppy_get_count(running_machine *machine)
{
	int cnt = 0;
	if (devtag_get_device(machine,FLOPPY_0)) cnt++;
    if (devtag_get_device(machine,FLOPPY_1)) cnt++;
    if (devtag_get_device(machine,FLOPPY_2)) cnt++;
    if (devtag_get_device(machine,FLOPPY_3)) cnt++;
	return cnt;
}


/* drive select 0 */
WRITE_LINE_DEVICE_HANDLER( floppy_ds0_w )
{
	floppy_drive *drive = get_safe_token(device);

	if (state == CLEAR_LINE)
		drive->active = (drive->drive_id == 0);
}

/* drive select 1 */
WRITE_LINE_DEVICE_HANDLER( floppy_ds1_w )
{
	floppy_drive *drive = get_safe_token(device);

	if (state == CLEAR_LINE)
		drive->active = (drive->drive_id == 1);
}

/* drive select 2 */
WRITE_LINE_DEVICE_HANDLER( floppy_ds2_w )
{
	floppy_drive *drive = get_safe_token(device);

	if (state == CLEAR_LINE)
		drive->active = (drive->drive_id == 2);
}

/* drive select 3 */
WRITE_LINE_DEVICE_HANDLER( floppy_ds3_w )
{
	floppy_drive *drive = get_safe_token(device);

	if (state == CLEAR_LINE)
		drive->active = (drive->drive_id == 3);
}

/* shortcut to write all four ds lines */
WRITE8_DEVICE_HANDLER( floppy_ds_w )
{
	floppy_ds0_w(device, BIT(data, 0));
	floppy_ds1_w(device, BIT(data, 1));
	floppy_ds2_w(device, BIT(data, 2));
	floppy_ds3_w(device, BIT(data, 3));
}

/* motor on, active low */
WRITE_LINE_DEVICE_HANDLER( floppy_mon_w )
{
	floppy_drive *drive = get_safe_token(device);

	/* force off if there is no attached image */
	if (!image_exists(device))
		state = ASSERT_LINE;

	if (image_slotexists(device))
	{
		/* off -> on */
		if (drive->mon && state == CLEAR_LINE)
		{
			drive->idx = 0;
			floppy_drive_index_func(device);
		}

		/* on -> off */
		else if (drive->mon == CLEAR_LINE && state)
			timer_adjust_oneshot(drive->index_timer, attotime_zero, 0);
	}

	drive->mon = state;
}

/* direction */
WRITE_LINE_DEVICE_HANDLER( floppy_drtn_w )
{
	floppy_drive *drive = get_safe_token(device);
	drive->drtn = state;
}

/* write data */
WRITE_LINE_DEVICE_HANDLER( floppy_wtd_w )
{

}

/* step */
WRITE_LINE_DEVICE_HANDLER( floppy_stp_w )
{
	floppy_drive *drive = get_safe_token(device);

	/* move head one track when going from high to low and write gate is high */
	if (drive->active && drive->stp && state == CLEAR_LINE && drive->wtg)
	{
		/* move head according to the direction line */
		if (drive->drtn)
		{
			/* move head outward */
			if (drive->current_track > 0)
				drive->current_track--;

			/* are we at track 0 now? */
			drive->tk00 = (drive->current_track == 0) ? CLEAR_LINE : ASSERT_LINE;
		}
		else
		{
			/* move head inward */
			if (drive->current_track < drive->max_track)
				drive->current_track++;

			/* we can't be at track 0 here, so reset the line */
			drive->tk00 = ASSERT_LINE;
		}

		/* update track 0 line with new status */
		devcb_call_write_line(&drive->out_tk00_func, drive->tk00);
	}

	drive->stp = state;
}

/* write gate */
WRITE_LINE_DEVICE_HANDLER( floppy_wtg_w )
{
	floppy_drive *drive = get_safe_token(device);
	drive->wtg = state;
}

/* write protect signal, active low */
READ_LINE_DEVICE_HANDLER( floppy_wpt_r )
{
	floppy_drive *drive = get_safe_token(device);

	if (image_slotexists(device))
	{
		if (image_is_writable(device))
			drive->wpt = ASSERT_LINE;
		else
			drive->wpt = CLEAR_LINE;
	}

	return drive->wpt;
}

/* track 0 detect */
READ_LINE_DEVICE_HANDLER( floppy_tk00_r )
{
	floppy_drive *drive = get_safe_token(device);
	return drive->tk00;
}


/*************************************
 *
 *  Device specification function
 *
 *************************************/
/*-------------------------------------------------
    safe_strcpy - hack
-------------------------------------------------*/

static void safe_strcpy(char *dst, const char *src)
{
	strcpy(dst, src ? src : "");
}

DEVICE_GET_INFO(floppy)
{
	switch( state )
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(floppy_drive); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0; break;
		case DEVINFO_INT_CLASS:						info->i = DEVICE_CLASS_PERIPHERAL; break;
		case DEVINFO_INT_IMAGE_TYPE:				info->i = IO_FLOPPY; break;
		case DEVINFO_INT_IMAGE_READABLE:			info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1; break;
		case DEVINFO_INT_IMAGE_CREATABLE:	{
												int cnt = 0;
												if ( device && device->static_config )
												{
													const struct FloppyFormat *floppy_options = ((floppy_config*)device->static_config)->formats;
													int	i;
													for ( i = 0; floppy_options[i].construct; i++ ) {
														if(floppy_options[i].param_guidelines) cnt++;
													}
												}
												info->i = (cnt>0) ? 1 : 0;
											}
											break;
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME(floppy); break;
		case DEVINFO_FCT_IMAGE_CREATE:				info->f = (genf *) DEVICE_IMAGE_CREATE_NAME(floppy); break;
		case DEVINFO_FCT_IMAGE_LOAD:				info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(floppy); break;
		case DEVINFO_FCT_IMAGE_UNLOAD:				info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(floppy); break;
		case DEVINFO_PTR_IMAGE_CREATE_OPTGUIDE:		info->p = (void *)floppy_option_guide; break;
		case DEVINFO_INT_IMAGE_CREATE_OPTCOUNT:		{
			const struct FloppyFormat *floppy_options = ((floppy_config*)device->static_config)->formats;
			int count;
			for (count = 0; floppy_options[count].construct; count++)
				;
			info->i = count;
			break;
			}

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:						strcpy(info->s, "Floppy Disk"); break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Floppy Disk"); break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:
			if ( device && device->static_config )
			{
				const struct FloppyFormat *floppy_options = ((floppy_config*)device->static_config)->formats;
				int		i;
				/* set up a temporary string */
				info->s[0] = '\0';
				for ( i = 0; floppy_options[i].construct; i++ )
					specify_extension( info->s, 256, floppy_options[i].extensions );
			}
			break;
		default:
			{
				if ( device && device->static_config )
				{
					const struct FloppyFormat *floppy_options = ((floppy_config*)device->static_config)->formats;
					if ((state >= DEVINFO_PTR_IMAGE_CREATE_OPTSPEC) && (state < DEVINFO_PTR_IMAGE_CREATE_OPTSPEC + DEVINFO_CREATE_OPTMAX)) {
						info->p = (void *) floppy_options[state - DEVINFO_PTR_IMAGE_CREATE_OPTSPEC].param_guidelines;
					} else if ((state >= DEVINFO_STR_IMAGE_CREATE_OPTNAME) && (state < DEVINFO_STR_IMAGE_CREATE_OPTNAME + DEVINFO_CREATE_OPTMAX)) {
						safe_strcpy(info->s,floppy_options[state - DEVINFO_STR_IMAGE_CREATE_OPTNAME].name);
					}
					else if ((state >= DEVINFO_STR_IMAGE_CREATE_OPTDESC) && (state < DEVINFO_STR_IMAGE_CREATE_OPTDESC + DEVINFO_CREATE_OPTMAX)) {
						safe_strcpy(info->s,floppy_options[state - DEVINFO_STR_IMAGE_CREATE_OPTDESC].description);
					}
					else if ((state >= DEVINFO_STR_IMAGE_CREATE_OPTEXTS) && (state < DEVINFO_STR_IMAGE_CREATE_OPTEXTS + DEVINFO_CREATE_OPTMAX)) {
						safe_strcpy(info->s,floppy_options[state - DEVINFO_STR_IMAGE_CREATE_OPTEXTS].extensions);
					}
				}
			}

			break;
	}
}
