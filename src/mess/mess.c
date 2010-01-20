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

/*-------------------------------------------------
    mess_predevice_init - initialize devices for a specific
    running_machine
-------------------------------------------------*/

void mess_predevice_init(running_machine *machine)
{
	int result = INIT_FAIL;
	const char *image_name;
	const device_config *image;
	device_config *dev;
	image_device_info info;
	device_get_image_devices_func get_image_devices;
	device_list *devlist;

	/* initialize natural keyboard support */
	inputx_init(machine);

	/* init all devices */
	image_init(machine);

	devlist = &((machine_config *)machine->config)->devicelist;

	/* make sure that any required devices have been allocated */
	for (image = image_device_first(machine->config); image != NULL; image = image_device_next(image))
	{
		/* get the device info */
		info = image_device_getinfo(machine->config, image);

		/* is an image specified for this image */
		image_name = mess_get_device_option(&info);

		if ((image_name != NULL) && (image_name[0] != '\0'))
		{
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
					osd_basename((char *) image_name),
					image_err);
			}
		}
		else
		{
			/* no image... must this device be loaded? */
			if (info.must_be_loaded)
			{
				/* unload all images */
				image_unload_all(machine);

				fatalerror_exitcode(machine, MAMERR_DEVICE, "Driver requires that device \"%s\" must have an image to load\n", info.instance_name);
			}
		}

		/* get image-specific hardware */
		get_image_devices = (device_get_image_devices_func) device_get_info_fct(image, DEVINFO_FCT_GET_IMAGE_DEVICES);
		if (get_image_devices != NULL)
			(*get_image_devices)(image, devlist);
	}

	/* ensure that any added devices have a machine */
	for (dev = machine->config->devicelist.head; dev != NULL; dev = dev->next)
	{
		if (dev->machine == NULL)
		{
			const machine_config_token *tokens;
			machine_config *config;
			device_config *config_dev;
			device_config *new_dev;
			astring tempstring;

			dev->machine = machine;

			tokens = (const machine_config_token *)device_get_info_ptr(dev, DEVINFO_PTR_MACHINE_CONFIG);
			if (tokens != NULL)
			{
				config = machine_config_alloc(tokens);
				for (config_dev = config->devicelist.head; config_dev != NULL; config_dev = config_dev->next)
				{
					new_dev = device_list_add(
						devlist,
						dev,
						config_dev->type,
						device_build_tag(tempstring, dev, config_dev->tag),
						config_dev->clock);

					new_dev->static_config = config_dev->static_config;
					memcpy(
						new_dev->inline_config,
						config_dev->inline_config,
						device_get_info_int(new_dev, DEVINFO_INT_INLINE_CONFIG_BYTES));
				}
				machine_config_free(config);
			}
		}
	}
}



/*-------------------------------------------------
    mess_postdevice_init - initialize devices for a specific
    running_machine
-------------------------------------------------*/

void mess_postdevice_init(running_machine *machine)
{
	const device_config *device;

	/* make sure that any required devices have been allocated */
	for (device = image_device_first(machine->config); device != NULL; device = image_device_next(device))
	{
		image_finish_load(device);
	}

	/* add a callback for when we shut down */
	add_exit_callback(machine, image_unload_all);
}

const game_driver *mess_next_compatible_driver(const game_driver *drv)
{
	if (driver_get_clone(drv))
		drv = driver_get_clone(drv);
	else if (drv->compatible_with)
		drv = driver_get_name(drv->compatible_with);
	else
		drv = NULL;
	return drv;
}



int mess_count_compatible_drivers(const game_driver *drv)
{
	int count = 0;
	while(drv)
	{
		count++;
		drv = mess_next_compatible_driver(drv);
	}
	return count;
}
