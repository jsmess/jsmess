/*****************************************************************************
 *
 * Image device
 *
 ****************************************************************************/

#include "emu.h"
#include "imagedev.h"
#include <ctype.h>
#include "hash.h"
#include "unzip.h"
#include "utils.h"
#include "hashfile.h"
#include "ui.h"
#include "device.h"
#include "zippath.h"
#include "softlist.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _image_t image_t;
struct _image_t
{
    core_file *file;

	const image_interface *intf;
};

/*****************************************************************************
    GLOBAL VARIABLES
*****************************************************************************/


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

INLINE image_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == IMAGE);

	return (image_t *)device->token;
}

/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

static DEVICE_START( image )
{
	image_t *image = get_safe_token(device);

	image->intf = (const image_interface*)device->baseconfig().static_config;

}

static DEVICE_RESET( image )
{
}


DEVICE_GET_INFO( image )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(image_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;								break;
		case DEVINFO_INT_CLASS:					info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(image);	break;
		case DEVINFO_FCT_STOP:					/* Nothing */								break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(image);	break;


		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "Image device");				break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Image device");				break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");		break;
	}
}
