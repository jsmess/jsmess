/*********************************************************************

	mflopimg.c

	MESS interface to the floppy disk image abstraction code

*********************************************************************/

#include "mflopimg.h"
#include "utils.h"
#include "image.h"
#include "devices/flopdrv.h"

#define FLOPPY_TAG		"floptag"
#define LOG_FLOPPY		0

struct mess_flopimg
{
	floppy_image *floppy;
	int track;
	void (*unload_proc)(mess_image *image);
	int (*tracktranslate_proc)(mess_image *image, floppy_image *floppy, int physical_track);
};

struct floppy_error_map
{
	floperr_t ferr;
	image_error_t ierr;
	const char *message;
};

static const struct floppy_error_map errmap[] =
{
	{ FLOPPY_ERROR_SUCCESS,			IMAGE_ERROR_SUCCESS },
	{ FLOPPY_ERROR_INTERNAL,		IMAGE_ERROR_INTERNAL },
	{ FLOPPY_ERROR_UNSUPPORTED,		IMAGE_ERROR_UNSUPPORTED },
	{ FLOPPY_ERROR_OUTOFMEMORY,		IMAGE_ERROR_OUTOFMEMORY },
	{ FLOPPY_ERROR_INVALIDIMAGE,	IMAGE_ERROR_INVALIDIMAGE }
};


static struct mess_flopimg *get_flopimg(mess_image *image)
{
	return (struct mess_flopimg *) image_lookuptag(image, FLOPPY_TAG);
}



floppy_image *flopimg_get_image(mess_image *image)
{
	return get_flopimg(image)->floppy;
}



static void flopimg_seek_callback(mess_image *image, int physical_track)
{
	struct mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	/* translate the track number if necessary */
	if (flopimg->tracktranslate_proc)
		physical_track = flopimg->tracktranslate_proc(image, flopimg->floppy, physical_track);

	flopimg->track = physical_track;
}



static int flopimg_get_sectors_per_track(mess_image *image, int side)
{
	struct mess_flopimg *flopimg;
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



static void flopimg_get_id_callback(mess_image *image, chrn_id *id, int id_index, int side)
{
	struct mess_flopimg *flopimg;
	int cylinder, sector, N;
	UINT32 sector_length;
	
	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_get_indexed_sector_info(flopimg->floppy, side, flopimg->track, id_index, &cylinder, &side, &sector, &sector_length);

	N = compute_log2(sector_length);

	id->C = cylinder;
	id->H = side;
	id->R = sector;
	id->data_id = id_index;
	id->flags = 0;
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



static void flopimg_read_sector_data_into_buffer(mess_image *image, int side, int index1, char *ptr, int length)
{
	struct mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_read_indexed_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length);
	
	if (LOG_FLOPPY)
		log_readwrite("sector_read", side, flopimg->track, index1, ptr, length);
}



static void flopimg_write_sector_data_from_buffer(mess_image *image, int side, int index1, const char *ptr, int length,int ddam)
{
	struct mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	if (LOG_FLOPPY)
		log_readwrite("sector_write", side, flopimg->track, index1, ptr, length);

	floppy_write_indexed_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length);
}



static void flopimg_read_track_data_info_buffer(mess_image *image, int side, void *ptr, int *length)
{
	struct mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_read_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
}



static void flopimg_write_track_data_info_buffer(mess_image *image, int side, const void *ptr, int *length)
{
	struct mess_flopimg *flopimg;

	flopimg = get_flopimg(image);
	if (!flopimg || !flopimg->floppy)
		return;

	floppy_write_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
}



static floppy_interface mess_floppy_interface =
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
	return image_fseek((mess_image *) file, offset, whence);
}

static size_t image_fread_thunk(void *file, void *buffer, size_t length)
{
	return image_fread((mess_image *) file, buffer, length);
}

static size_t image_fwrite_thunk(void *file, const void *buffer, size_t length)
{
	return image_fwrite((mess_image *) file, buffer, length);
}

static UINT64 image_fsize_thunk(void *file)
{
	return image_length((mess_image *) file);
}

/* ----------------------------------------------------------------------- */

struct io_procs mess_ioprocs =
{
	NULL,
	image_fseek_thunk,
	image_fread_thunk,
	image_fwrite_thunk,
	image_fsize_thunk
};




/* ----------------------------------------------------------------------- */


static int device_init_floppy(mess_image *image)
{
	if (!image_alloctag(image, FLOPPY_TAG, sizeof(struct mess_flopimg)))
		return INIT_FAIL;
	return floppy_drive_init(image, &mess_floppy_interface);
}



static int internal_floppy_device_load(mess_image *image, int create_format, option_resolution *create_args)
{
	floperr_t err;
	struct mess_flopimg *flopimg;
	const struct IODevice *dev;
	const struct FloppyFormat *floppy_options;
	int floppy_flags, i;
	const char *extension;

	/* look up instance data */
	flopimg = get_flopimg(image);

	/* figure out the floppy options */
	dev = image_device(image);
	floppy_options = device_get_info_ptr(&dev->devclass, DEVINFO_PTR_FLOPPY_OPTIONS);

	if (image_has_been_created(image))
	{
		/* creating an image */
		assert(create_format >= 0);
		err = floppy_create(image, &mess_ioprocs, &floppy_options[create_format], create_args, &flopimg->floppy);
		if (err)
			goto error;
	}
	else
	{
		/* opening an image */
		floppy_flags = image_is_writable(image) ? FLOPPY_FLAGS_READWRITE : FLOPPY_FLAGS_READONLY;
		extension = image_filetype(image);
		err = floppy_open_choices(image, &mess_ioprocs, extension, floppy_options, floppy_flags, &flopimg->floppy);
		if (err)
			goto error;
	}

	/* if we can get head and track counts, then set the geometry accordingly */
	if (floppy_callbacks(flopimg->floppy)->get_heads_per_disk
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



static int device_load_floppy(mess_image *image)
{
	return internal_floppy_device_load(image, -1, NULL);
}



static int device_create_floppy(mess_image *image, int create_format, option_resolution *create_args)
{
	return internal_floppy_device_load(image, create_format, create_args);
}



static void device_unload_floppy(mess_image *image)
{
	struct mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);

	/* if we have one of our hacky unload procs, call it */
	if (flopimg->unload_proc)
		flopimg->unload_proc(image);

	floppy_close(flopimg->floppy);
	flopimg->floppy = NULL;
}



void specify_extension(char *extbuf, size_t extbuflen, const char *extension)
{
	char *s;

	/* loop through the extensions that we are adding */
	while(extension && *extension)
	{
		/* loop through the already specified extensions; and check for dupes */
		for (s = extbuf; *s; s += strlen(s) + 1)
		{
			if (!strcmp(extension, s))
				break;
		}

		/* only write if there are no dupes */
		if (*s == '\0')
		{
			/* out of room?  this should never happen */
			if ((s - extbuf + strlen(extension) + 1) >= extbuflen)
			{
				assert(FALSE);
				continue;
			}
	
			/* copy the extension */
			strcpy(s, extension);
			s[strlen(s) + 1] = '\0';
		}

		/* next extension */
		extension += strlen(extension) + 1;
	}
}



/*************************************
 *
 *	Hacks for specific systems
 *
 *************************************/

void floppy_install_unload_proc(mess_image *image, void (*proc)(mess_image *image))
{
	struct mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);
	flopimg->unload_proc = proc;
}



void floppy_install_tracktranslate_proc(mess_image *image, int (*proc)(mess_image *image, floppy_image *floppy, int physical_track))
{
	struct mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);
	flopimg->tracktranslate_proc = proc;
}



/*************************************
 *
 *	Device specification function
 *
 *************************************/

void floppy_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	char *s;
	int i, count;
	const struct FloppyFormat *floppy_options;

	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:				info->i = IO_FLOPPY; break;
		case DEVINFO_INT_READABLE:			info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:			info->i = 1; break;
		case DEVINFO_INT_CREATABLE:
			floppy_options = device_get_info_ptr(devclass, DEVINFO_PTR_FLOPPY_OPTIONS);
			info->i = floppy_options->param_guidelines ? 1 : 0;
			break;

		case DEVINFO_INT_CREATE_OPTCOUNT:
			/* count total floppy options */
			floppy_options = device_get_info_ptr(devclass, DEVINFO_PTR_FLOPPY_OPTIONS);
			for (count = 0; floppy_options[count].construct; count++)
				;
			info->i = count;
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:
			floppy_options = device_get_info_ptr(devclass, DEVINFO_PTR_FLOPPY_OPTIONS);
			s = device_temp_str();
			info->s = s;
			s[0] = '\0';
			s[1] = '\0';
			for (i = 0; floppy_options[i].construct; i++)
				specify_extension(s, 256, floppy_options[i].extensions);
			while(s[strlen(s) + 1] != '\0')
			{
				s += strlen(s);
				*(s++) = ',';
			}
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:				info->init = device_init_floppy; break;
		case DEVINFO_PTR_LOAD:				info->load = device_load_floppy; break;
		case DEVINFO_PTR_CREATE:			info->create = device_create_floppy; break;
		case DEVINFO_PTR_UNLOAD:			info->unload = device_unload_floppy; break;
		case DEVINFO_PTR_CREATE_OPTGUIDE:	info->p = (void *) floppy_option_guide; break;

		default:
			floppy_options = device_get_info_ptr(devclass, DEVINFO_PTR_FLOPPY_OPTIONS);
			if ((state >= DEVINFO_STR_CREATE_OPTNAME) && (state < DEVINFO_STR_CREATE_OPTNAME + DEVINFO_CREATE_OPTMAX))
			{
				info->s = (void *) floppy_options[state - DEVINFO_STR_CREATE_OPTNAME].name;
			}
			else if ((state >= DEVINFO_STR_CREATE_OPTDESC) && (state < DEVINFO_STR_CREATE_OPTDESC + DEVINFO_CREATE_OPTMAX))
			{
				info->s = (void *) floppy_options[state - DEVINFO_STR_CREATE_OPTDESC].description;
			}
			else if ((state >= DEVINFO_STR_CREATE_OPTEXTS) && (state < DEVINFO_STR_CREATE_OPTEXTS + DEVINFO_CREATE_OPTMAX))
			{
				info->s = (void *) floppy_options[state - DEVINFO_STR_CREATE_OPTEXTS].extensions;
			}
			else if ((state >= DEVINFO_PTR_CREATE_OPTSPEC) && (state < DEVINFO_PTR_CREATE_OPTSPEC + DEVINFO_CREATE_OPTMAX))
			{
				info->p = (void *) floppy_options[state - DEVINFO_PTR_CREATE_OPTSPEC].param_guidelines;
			}
			break;
	}
}

