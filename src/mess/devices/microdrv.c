/****************************************************************************

	microdrv.c

	Code for handling Microdrive devices

****************************************************************************/

#include "driver.h"
#include "microdrv.h"

static DEVICE_START(microdrv)
{
}

DEVICE_GET_INFO(microdrv)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 1; break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0; break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL; break;
		case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_CASSETTE; break;
		case DEVINFO_INT_IMAGE_READABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_CREATABLE:				info->i = 0; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(microdrv); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Sinclair QL Microdrive"; break;
		case DEVINFO_STR_FAMILY:						info->s = "Microdrive"; break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__; break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			info->s = "mdv"; break;
	}
}
