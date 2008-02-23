/*********************************************************************

	Code to interface the MESS image code with MAME's CHD-CD core.

	Based on harddriv.c by Raphael Nabet 2003

*********************************************************************/

#include "cdrom.h"
#include "chd_cd.h"


#define MAX_CDROMS	(4)	// up to this many drives

static const char *const error_strings[] =
{
	"no error",
	"no drive interface",
	"out of memory",
	"invalid file",
	"invalid parameter",
	"invalid data",
	"file not found",
	"requires parent",
	"file not writeable",
	"read error",
	"write error",
	"codec error",
	"invalid parent",
	"hunk out of range",
	"decompression error",
	"compression error",
	"can't create file",
	"can't verify file"
	"operation not supported",
	"can't find metadata",
	"invalid metadata size",
	"unsupported CHD version"
};

static cdrom_file *drive_handles[MAX_CDROMS];

static const char *chd_get_error_string(int chderr)
{
	if ((chderr < 0 ) || (chderr >= (sizeof(error_strings) / sizeof(error_strings[0]))))
		return NULL;
	return error_strings[chderr];
}



static OPTION_GUIDE_START(mess_cd_option_guide)
	OPTION_INT('K', "hunksize",			"Hunk Bytes")
OPTION_GUIDE_END

static const char mess_cd_option_spec[] =
	"K512/1024/2048/[4096]";


#define MESSCDTAG "mess_cd"

struct mess_cd
{
	cdrom_file *cdrom_handle;
};

static struct mess_cd *get_drive(mess_image *img)
{
	return image_lookuptag(img, MESSCDTAG);
}



/*************************************
 *
 *  chdcd_create_ref()/chdcd_open_ref()
 *
 *  These are a set of wrappers that wrap the chd_open()
 *  and chd_create() functions to provide a way to open
 *  the images with a filename.  This is just a stopgap
 *  measure until I get the core CHD code changed.  For
 *  now, this is an ugly hack but it works very well
 *
 *  When these functions get moved into the core, it will
 *  remove the need to specify an 'open' function in the
 *  CHD interface
 *
 *************************************/

/*************************************
 *
 *	device_init_mess_cd()
 *
 *	Device init
 *
 *************************************/

static int device_init_mess_cd(mess_image *image)
{
	struct mess_cd *cd;

	cd = image_alloctag(image, MESSCDTAG, sizeof(struct mess_cd));
	if (!cd)
		return INIT_FAIL;

        cd->cdrom_handle = NULL;

	return INIT_PASS;
}



/*************************************
 *
 *	device_load_mess_cd()
 *	device_create_mess_cd()
 *
 *	Device load and create
 *
 *************************************/

static int internal_load_mess_cd(mess_image *image, const char *metadata)
{
	chd_error err = 0;
	struct mess_cd *cd;
	chd_file *chd;
	int id = image_index_in_device(image);

	cd = get_drive(image);

	/* open the CHD file */
	err = chd_open_file(image_core_file(image), CHD_OPEN_READ, NULL, &chd);	/* CDs are never writable */
	if (err)
		goto error;

	/* open the CD-ROM file */
	cd->cdrom_handle = cdrom_open(chd);
	if (!cd->cdrom_handle)
		goto error;

	drive_handles[id] = cd->cdrom_handle;
	return INIT_PASS;

error:
	if (chd)
		chd_close(chd);
	if (err)
		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, chd_get_error_string(err));
	return INIT_FAIL;
}



static int device_load_mess_cd(mess_image *image)
{
	return internal_load_mess_cd(image, NULL);
}



/*************************************
 *
 *	device_unload_mess_cd()
 *
 *	Device unload
 *
 *************************************/

static void device_unload_mess_cd(mess_image *image)
{
	struct mess_cd *cd = get_drive(image);
	assert(cd->cdrom_handle);
	cdrom_close(cd->cdrom_handle);
	cd->cdrom_handle = NULL;
}



/*************************************
 *
 *  Get the MESS/MAME cdrom handle (from the src/cdrom.c core)
 *  after an image has been opened with the mess_cd core
 *
 *************************************/

cdrom_file *mess_cd_get_cdrom_file(mess_image *image)
{
	struct mess_cd *cd = get_drive(image);
	return cd->cdrom_handle;
}



/*************************************
 *
 *	Get the MESS/MAME CHD file (from the src/chd.c core)
 *  after an image has been opened with the mess_cd core
 *
 *************************************/

chd_file *mess_cd_get_chd_file(mess_image *image)
{
	return NULL;	// not supported by the src/cdrom.c core at this time
}



/*************************************
 *
 *	Device specification function
 *
 *************************************/

void cdrom_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:						info->i = IO_CDROM; break;
		case MESS_DEVINFO_INT_READABLE:					info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:					info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:					info->i = 0; break;
		case MESS_DEVINFO_INT_CREATE_OPTCOUNT:			info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_INIT:						info->init = device_init_mess_cd; break;
		case MESS_DEVINFO_PTR_LOAD:						info->load = device_load_mess_cd; break;
		case MESS_DEVINFO_PTR_UNLOAD:					info->unload = device_unload_mess_cd; break;
		case MESS_DEVINFO_PTR_CREATE_OPTGUIDE:			info->p = (void *) mess_cd_option_guide; break;
		case MESS_DEVINFO_PTR_CREATE_OPTSPEC+0:			info->p = (void *) mess_cd_option_spec;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "chd"); break;
		case MESS_DEVINFO_STR_CREATE_OPTNAME+0:			strcpy(info->s = device_temp_str(), "chdcd"); break;
		case MESS_DEVINFO_STR_CREATE_OPTDESC+0:			strcpy(info->s = device_temp_str(), "MAME/MESS CHD CD-ROM drive"); break;
		case MESS_DEVINFO_STR_CREATE_OPTEXTS+0:			strcpy(info->s = device_temp_str(), "chd"); break;
	}
}

cdrom_file *mess_cd_get_cdrom_file_by_number(int drivenum)
{
	if ((drivenum < 0) || (drivenum > MAX_CDROMS))
	{
		return NULL;
	}

	return drive_handles[drivenum];
}
