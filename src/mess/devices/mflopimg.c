/*********************************************************************

	mflopimg.c

	MESS interface to the floppy disk image abstraction code

*********************************************************************/

#include "mame.h"
#include "mflopimg.h"
#include "flopdrv.h"
#include "formats/flopimg.h"



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

