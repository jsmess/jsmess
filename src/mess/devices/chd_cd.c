/*********************************************************************

    Code to interface the MESS image code with MAME's CHD-CD core.

    Based on harddriv.c by Raphael Nabet 2003

*********************************************************************/

#include "driver.h"
#include "cdrom.h"
#include "chd_cd.h"


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


static const char *chd_get_error_string(int chderr)
{
	if ((chderr < 0 ) || (chderr >= ARRAY_LENGTH(error_strings)))
		return NULL;
	return error_strings[chderr];
}


static OPTION_GUIDE_START(mess_cd_option_guide)
	OPTION_INT('K', "hunksize",			"Hunk Bytes")
OPTION_GUIDE_END

static const char mess_cd_option_spec[] =
	"K512/1024/2048/[4096]";


typedef struct _dev_cdrom_t	dev_cdrom_t;
struct _dev_cdrom_t
{
	cdrom_file	*cdrom_handle;
};


INLINE dev_cdrom_t *get_safe_token(const device_config *device) {
	assert( device != NULL );
	assert( device->token != NULL );
	assert( ( device->type == DEVICE_GET_INFO_NAME(cdrom) ) );
	return (dev_cdrom_t *)  device->token;
}


static DEVICE_IMAGE_LOAD(cdrom)
{
	dev_cdrom_t	*cdrom = get_safe_token(image);
	chd_error	err = 0;
	chd_file	*chd = NULL;

	err = chd_open_file( image_core_file( image ), CHD_OPEN_READ, NULL, &chd );	/* CDs are never writeable */
	if ( err )
		goto error;

	/* open the CHD file */
	cdrom->cdrom_handle = cdrom_open( chd );
	if ( ! cdrom->cdrom_handle )
		goto error;

	return INIT_PASS;

error:
	if ( chd )
		chd_close( chd );
	if ( err )
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, chd_get_error_string( err ) );
	return INIT_FAIL;
}


static DEVICE_IMAGE_UNLOAD(cdrom)
{
	dev_cdrom_t	*cdrom = get_safe_token( image );

	assert( cdrom->cdrom_handle );
	cdrom_close( cdrom->cdrom_handle );
	cdrom->cdrom_handle = NULL;
}


/*************************************
 *
 *  Get the MESS/MAME cdrom handle (from the src/cdrom.c core)
 *  after an image has been opened with the mess_cd core
 *
 *************************************/

cdrom_file *mess_cd_get_cdrom_file(const device_config *image)
{
	dev_cdrom_t	*cdrom = get_safe_token( image );

	return cdrom->cdrom_handle;
}


/*-------------------------------------------------
    DEVICE_START(cdrom)
-------------------------------------------------*/

static DEVICE_START(cdrom)
{
	dev_cdrom_t	*cdrom = get_safe_token( device );

	cdrom->cdrom_handle = NULL;
}


/*-------------------------------------------------
    DEVICE_GET_INFO(cdrom)
-------------------------------------------------*/

DEVICE_GET_INFO(cdrom)
{
	switch( state )
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(dev_cdrom_t); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0; break;
		case DEVINFO_INT_CLASS:						info->i = DEVICE_CLASS_PERIPHERAL; break;
		case DEVINFO_INT_IMAGE_TYPE:				info->i = IO_CDROM; break;
		case DEVINFO_INT_IMAGE_READABLE:			info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_CREATABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_CREATE_OPTCOUNT:		info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME(cdrom); break;
		case DEVINFO_FCT_IMAGE_LOAD:				info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(cdrom); break;
		case DEVINFO_FCT_IMAGE_UNLOAD:				info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(cdrom); break;
		case DEVINFO_PTR_IMAGE_CREATE_OPTGUIDE:		info->p = (void *) mess_cd_option_guide; break;
		case DEVINFO_PTR_IMAGE_CREATE_OPTSPEC+0:	info->p = (void *) mess_cd_option_spec; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:						strcpy(info->s, "Cdrom"); break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Cdrom"); break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:		strcpy(info->s, "chd"); break;
		case DEVINFO_STR_IMAGE_CREATE_OPTNAME+0:	strcpy(info->s, "chdcd"); break;
		case DEVINFO_STR_IMAGE_CREATE_OPTDESC+0:	strcpy(info->s, "MAME/MESS CHD CD-ROM drive"); break;
		case DEVINFO_STR_IMAGE_CREATE_OPTEXTS+0:	strcpy(info->s, "chd"); break;
	}
}

