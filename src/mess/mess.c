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
#include "uitext.h"
#include "state.h"
#include "image.h"
#include "inputx.h"
#include "hash.h"
#include "messopts.h"

/* Globals */
const char *mess_path;

UINT32 mess_ram_size;
UINT8 *mess_ram;
UINT8 mess_ram_default_value = 0xCD;

char mess_disclaimer[] =
		"MESS is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		"several computer and console systems. But hardware is useless without software\n"
		"so a file dump of the ROM, cartridges, discs, and cassettes which run on that\n"
		"hardware is required. Such files, like any other commercial software, are\n"
		"copyrighted material and it is therefore illegal to use them if you don't own\n"
		"the original media from which the files are derived. Needless to say, these\n"
		"files are not distributed together with MESS. Distribution of MESS together\n"
		"with these files is a violation of copyright law and should be promptly\n"
		"reported to the authors so that appropriate legal action can be taken.\n\n";



static void ram_init(const game_driver *gamedrv)
{
	int i;
	char buffer[1024];
	char *s;
	const char *ramsize_string;
	UINT32 specified_ram = 0;

	/* parse RAM option */
	ramsize_string = options_get_string(mame_options(), OPTION_RAMSIZE);
	if ((ramsize_string != NULL) && (ramsize_string[0] != '\0'))
	{
		specified_ram = ram_parse_string(ramsize_string);
		if (specified_ram == 0)
		{
			fatalerror_exitcode(MAMERR_DEVICE, "Cannot recognize the RAM option %s\n", ramsize_string);
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
				fatalerror_exitcode(MAMERR_DEVICE, "Driver '%s' does not support RAM configurations\n", gamedrv->name);
			}
			else
			{
				s = buffer;
				s += sprintf(s, "%s is not a valid RAM option for driver '%s' (valid choices are ",
					ram_string(buffer2, specified_ram), gamedrv->name);
				for (i = 0; i < opt_count; i++)
					s += sprintf(s, "%s%s",  i ? " or " : "", ram_string(buffer2, ram_option(gamedrv, i)));
				s += sprintf(s, ")\n");
				fatalerror_exitcode(MAMERR_DEVICE, "%s", buffer);
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
		mess_ram = (UINT8 *) auto_malloc(mess_ram_size);
		memset(mess_ram, mess_ram_default_value, mess_ram_size);

		state_save_register_item("mess", 0, mess_ram_size);
		state_save_register_item_pointer("mess", 0, mess_ram, mess_ram_size);
	}
	else
	{
		mess_ram = NULL;
	}
}



/*-------------------------------------------------
    devices_exit - tear down devices for a specific
	running_machine
-------------------------------------------------*/

static void devices_exit(running_machine *machine)
{
	devices_free(machine->devices);
	machine->devices = NULL;
}



/*-------------------------------------------------
    devices_init - initialize devices for a specific
	running_machine
-------------------------------------------------*/

void devices_init(running_machine *machine)
{
	const struct IODevice *dev;
	int id;
	int result = INIT_FAIL;
	const char *image_name;
	mess_image *image;

	/* convienient place to call this */
	{
		char buf[260];
		osd_getcurdir(buf, ARRAY_LENGTH(buf));
		mess_path = auto_strdup(buf);
	}

	/* initialize natural keyboard support */
	inputx_init();

	/* allocate the IODevice struct */
	machine->devices = (struct IODevice *) devices_allocate(machine->gamedrv);
	if (!machine->devices)
		fatalerror_exitcode(MAMERR_DEVICE, "devices_allocate() failed");
	add_exit_callback(machine, devices_exit);

	/* initialize RAM code */
	ram_init(machine->gamedrv);

	/* init all devices */
	image_init();

	/* make sure that any required devices have been allocated */
	for (dev = machine->devices; dev->type < IO_COUNT; dev++)
	{
		for (id = 0; id < dev->count; id++)
		{
			/* identify the image */
			image = image_from_device_and_index(dev, id);

			/* is an image specified for this image */
			image_name = mess_get_device_option(&dev->devclass, id);
			if ((image_name != NULL) && (image_name[0] != '\0'))
			{
				/* try to load this image */
				result = image_load(image, image_name);

				/* did the image load fail? */
				if (result)
				{
					fatalerror_exitcode(MAMERR_DEVICE, "Device %s load (%s) failed: %s\n",
						device_typename(dev->type),
						osd_basename((char *) image_name),
						image_error(image));
				}
			}
			else
			{
				/* no image... must this device be loaded? */
				if (dev->must_be_loaded)
				{
					fatalerror_exitcode(MAMERR_DEVICE, "Driver requires that device %s must have an image to load\n", device_typename(dev->type));
				}
			}
		}
	}
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


/***************************************************************************

	Dummy read handlers

***************************************************************************/

READ8_HANDLER( return8_00 )	{ return 0x00; }
READ8_HANDLER( return8_FE )	{ return 0xFE; }
READ8_HANDLER( return8_FF )	{ return 0xFF; }
READ16_HANDLER( return16_FFFF ) { return 0xFFFF; }
