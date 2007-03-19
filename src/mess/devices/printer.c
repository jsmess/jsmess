/*
 * printer.c : simple printer port implementation
 * "online/offline" and output only.
 */

#include "driver.h"
#include "devices/printer.h"
#include "image.h"



int printer_status(mess_image *img, int newstatus)
{
	/* if there is a file attached to it, it's online */
	return image_exists(img) != 0;
}



void printer_output(mess_image *img, int data)
{
	UINT8 d = data;
	image_fwrite(img, &d, 1);
}



void printer_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_PRINTER; break;
		case DEVINFO_INT_READABLE:						info->i = 0; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:						strcpy(info->s = device_temp_str(), __FILE__); break;
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "prn"); break;
	}
}
