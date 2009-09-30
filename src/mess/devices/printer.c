/****************************************************************************

    printer.c

    Code for handling printer devices

****************************************************************************/

#include "driver.h"
#include "printer.h"



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    printer_is_ready - checks to see if a printer
    is ready
-------------------------------------------------*/

int printer_is_ready(const device_config *printer)
{
	/* if there is a file attached to it, it's online */
	return image_exists(printer) != 0;
}



/*-------------------------------------------------
    printer_output - outputs data to a printer
-------------------------------------------------*/

void printer_output(const device_config *printer, UINT8 data)
{
	if (image_exists(printer))
	{
		image_fwrite(printer, &data, 1);
	}
}


/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( printer )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( printer )
{
	const printer_config *conf = image->inline_config;

	/* send notify that the printer is now online */
	if (conf != NULL && conf->online != NULL)
		conf->online(image, TRUE);

	/* we don't need to do anything special */
	return INIT_PASS;
}


/*-------------------------------------------------
    DEVICE_IMAGE_UNLOAD( printer )
-------------------------------------------------*/

static DEVICE_IMAGE_UNLOAD( printer )
{
	const printer_config *conf = image->inline_config;

	/* send notify that the printer is now offline */
	if (conf != NULL && conf->online != NULL)
		conf->online(image, FALSE);
}


/*-------------------------------------------------
    DEVICE_START(printer)
-------------------------------------------------*/

static DEVICE_START(printer)
{
}



/*-------------------------------------------------
    DEVICE_GET_INFO(printer)
-------------------------------------------------*/

DEVICE_GET_INFO(printer)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 1; break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(printer_config); break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL; break;
		case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_PRINTER; break;
		case DEVINFO_INT_IMAGE_READABLE:				info->i = 0; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_CREATABLE:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(printer); break;
		case DEVINFO_FCT_IMAGE_LOAD:					info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(printer);		break;
		case DEVINFO_FCT_IMAGE_UNLOAD:					info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(printer);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Printer"); break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Printer"); break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, "prn"); break;
	}
}
