/***************************************************************************

	mess.c

	This file is a set of function calls and defs required for MESS

***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <assert.h>

#include "driver.h"
#include "devices/flopdrv.h"
#include "utils.h"
#include "image.h"
#include "hash.h"
#include "messopts.h"

#include "lcd.lh"

/* Globals */
UINT32 mess_ram_size;
UINT8 *mess_ram;
UINT8 mess_ram_default_value = 0xCD;

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



static void ram_init(running_machine *machine, const game_driver *gamedrv)
{
	int i;
	char buffer[1024];
	char *s;
	const char *ramsize_string;
	UINT32 specified_ram = 0;
	const device_config *ram_dev = devtag_get_device(machine, "messram");

	/* parse RAM option */
	ramsize_string = options_get_string(mame_options(), OPTION_RAMSIZE);
	if ((ramsize_string != NULL) && (ramsize_string[0] != '\0') && (ram_dev == NULL))
	{
		specified_ram = ram_parse_string(ramsize_string);
		if (specified_ram == 0)
		{
			fatalerror_exitcode(machine, MAMERR_DEVICE, "Cannot recognize the RAM option %s\n", ramsize_string);
		}

		/* is this option actually valid? */
		if (!ram_is_valid_option(gamedrv, specified_ram))
		{
			char buffer2[RAM_STRING_BUFLEN];
			int opt_count;

			opt_count = ram_option_count(gamedrv);
			if (opt_count == 0)
			{
				/* this driver doesn't support RAM configurations */
				fatalerror_exitcode(machine, MAMERR_DEVICE, "Driver '%s' does not support RAM configurations\n", gamedrv->name);
			}
			else
			{
				s = buffer;
				s += sprintf(s, "%s is not a valid RAM option for driver '%s' (valid choices are ",
					ram_string(buffer2, specified_ram), gamedrv->name);
				for (i = 0; i < opt_count; i++)
					s += sprintf(s, "%s%s",  i ? " or " : "", ram_string(buffer2, ram_option(gamedrv, i)));
				s += sprintf(s, ")\n");
				fatalerror_exitcode(machine, MAMERR_DEVICE, "%s", buffer);
			}
		}
		mess_ram_size = specified_ram;
	}
	else
	{
		/* none specified; chose default */
		mess_ram_size = ram_default(gamedrv);
	}

	/* if we have RAM, allocate it */
	if (mess_ram_size > 0)
	{
		mess_ram = auto_alloc_array_clear(machine, UINT8, mess_ram_size);
		memset(mess_ram, mess_ram_default_value, mess_ram_size);

		state_save_register_item(machine, "mess", NULL, 0, mess_ram_size);
		state_save_register_item_pointer(machine, "mess", NULL, 0, mess_ram, mess_ram_size);
	}
	else
	{
		mess_ram = NULL;
	}
}



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

	/* initialize natural keyboard support */
	inputx_init(machine);

	/* allocate the IODevice struct */
	mess_devices_setup(machine, (machine_config *) machine->config, machine->gamedrv);

	/* initialize RAM code */
	ram_init(machine, machine->gamedrv);

	/* init all devices */
	image_init(machine);

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
			(*get_image_devices)(image, (device_config **) &machine->config->devicelist);
	}

	/* ensure that any added devices have a machine */
	for (dev = (device_config *) machine->config->devicelist; dev != NULL; dev = dev->next)
	{
		if (dev->machine == NULL)
		{
			const machine_config_token *tokens;
			machine_config *config;
			device_config *config_dev;
			device_config *new_dev;
			astring *tempstring = astring_alloc();

			dev->machine = machine;

			tokens = device_get_info_ptr(dev, DEVINFO_PTR_MACHINE_CONFIG);
			if (tokens != NULL)
			{
				config = machine_config_alloc(tokens);
				for (config_dev = config->devicelist; config_dev != NULL; config_dev = config_dev->next)
				{
					new_dev = device_list_add(
						(device_config **) &machine->config->devicelist,
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
			astring_free(tempstring);
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



void ram_dump(const char *filename)
{
	file_error filerr;
	mame_file *file;

	/* use a default filename */
	if (!filename)
		filename = "ram.bin";

	/* open the file */
	filerr = mame_fopen(NULL, filename, OPEN_FLAG_WRITE, &file);
	if (filerr == FILERR_NONE)
	{
		/* write the data */
		mame_fwrite(file, mess_ram, mess_ram_size);

		/* close file */
		mame_fclose(file);
	}
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



UINT32 hash_data_extract_crc32(const char *d)
{
	UINT32 crc = 0;
	UINT8 crc_bytes[4];

	if (hash_data_extract_binary_checksum(d, HASH_CRC, crc_bytes) == 1)
	{
		crc = (((UINT32) crc_bytes[0]) << 24)
			| (((UINT32) crc_bytes[1]) << 16)
			| (((UINT32) crc_bytes[2]) << 8)
			| (((UINT32) crc_bytes[3]) << 0);
	}
	return crc;
}
