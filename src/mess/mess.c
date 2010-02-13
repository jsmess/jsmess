/***************************************************************************

    mess.c

    This file is a set of function calls and defs required for MESS

***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <assert.h>

#include "emu.h"
#include "utils.h"
#include "image.h"
#include "messopts.h"

#include "lcd.lh"
#include "lcd_rot.lh"

/* Globals */
const char mess_disclaimer[] =
		"MESS is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		"several computer and console systems. But hardware is useless without software\n"
		"so a file dump of the ROM, cartridges, discs, and cassettes which run on that\n"
		"hardware is required. Such files, like any other commercial software, are\n"
		"copyrighted material and it is therefore illegal to use them if you don't own\n"
		"the original media from which the files are derived. Needless to say, these\n"
		"files are not distributed together with MESS. Distribution of MESS together\n"
		"with these files is a violation of copyright law and should be promptly\n"
		"reported to the authors so that appropriate legal action can be taken.\n\n";

static char *filename_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}
		
/*-------------------------------------------------
    mess_predevice_init - initialize devices for a specific
    running_machine
-------------------------------------------------*/

void mess_predevice_init(running_machine *machine)
{	
	const char *image_name;
	running_device *image;
	image_device_info info;
	device_get_image_devices_func get_image_devices;

	/* initialize natural keyboard support */
	inputx_init(machine);

	/* init all devices */
	image_init(machine);

	/* make sure that any required devices have been allocated */
    for (image = machine->devicelist.first(); image != NULL; image = image->next)
    {
        if (is_image_device(image))
		{
			/* get the device info */
			info = image_device_getinfo(machine->config, image);

			/* is an image specified for this image */
			image_name = mess_get_device_option(&info);

			if ((image_name != NULL) && (image_name[0] != '\0'))
			{
				int result = INIT_FAIL;
				
				/* mark init state */
				set_init_phase(image);
				
				/* try to load this image */
				result = image_load(image, image_name);

				/* did the image load fail? */
				if (result)
				{
					/* retrieve image error message */
					const char *image_err = image_error(image);

					/* unload all images */
					image_unload_all(machine);					
					/* FIXME: image_name is always empty in this message because of the image_unload_all() call */
					fatalerror_exitcode(machine, MAMERR_DEVICE, "Device %s load (%s) failed: %s\n",
						info.name,
						filename_basename((char *) image_name),
						image_err);
				}
			}
			else
			{
				/* no image... must this device be loaded? */
				if (info.must_be_loaded)
				{
					fatalerror_exitcode(machine, MAMERR_DEVICE, "Driver requires that device \"%s\" must have an image to load\n", info.instance_name);
				}
			}

			/* get image-specific hardware */
			get_image_devices = (device_get_image_devices_func) image->get_config_fct(DEVINFO_FCT_GET_IMAGE_DEVICES);
			if (get_image_devices != NULL)
				(*get_image_devices)(image);
		}	
	}
}



/*-------------------------------------------------
    mess_postdevice_init - initialize devices for a specific
    running_machine
-------------------------------------------------*/

void mess_postdevice_init(running_machine *machine)
{
	running_device *device;	

	/* make sure that any required devices have been allocated */
    for (device = machine->devicelist.first(); device != NULL; device = device->next)
    {
        if (is_image_device(device))
		{
			image_finish_load(device);
		}
	}

	/* add a callback for when we shut down */
	add_exit_callback(machine, image_unload_all);
}

