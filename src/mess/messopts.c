/***************************************************************************

    messopts.c - MESS specific option handling

****************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "options.h"
#include "pool.h"
#include "config.h"
#include "xmlfile.h"
#include "messopts.h"

#define OPTION_ADDED_DEVICE_OPTIONS	"added_device_options"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*device_enum_func)(core_options *opts, const game_driver *gamedrv,
	const machine_config *config, const device_config *device, int global_index, running_device *dev);



/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

const options_entry mess_core_options[] =
{
	{ NULL,							NULL,   OPTION_HEADER,						"MESS SPECIFIC OPTIONS" },
	{ "ramsize;ram",				NULL,	0,									"size of RAM (if supported by driver)" },
	{ "writeconfig;wc",				"0",	OPTION_BOOLEAN,						"writes configuration to (driver).ini on exit" },
	{ "newui;nu",                   "0",    OPTION_BOOLEAN,						"use the new MESS UI" },
	{ OPTION_ADDED_DEVICE_OPTIONS,	"0",	OPTION_BOOLEAN | OPTION_INTERNAL,	"device-specific options have been added" },
	{ NULL }
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    mess_get_device_option - accesses a device
    option, by device and index
-------------------------------------------------*/

const char *mess_get_device_option(const image_device_info *info)
{
	const char *result = NULL;

	if (options_get_bool(mame_options(), OPTION_ADDED_DEVICE_OPTIONS))
	{
		/* access the option */
		result = options_get_string(mame_options(), info->instance_name);
	}
	return result;
}



/*-------------------------------------------------
    mess_enumerate_devices - runs a specified proc
    for all devices on a driver
-------------------------------------------------*/

static void mess_enumerate_devices(core_options *opts, const machine_config *config, const game_driver *gamedrv,
	device_enum_func proc,running_machine *machine)
{
	int index = 0;

	/* loop on each device instance */
	if (machine) {
		running_device *device;
		for (device = machine->devicelist.first(); device != NULL; device = device->next)
		{
			if (is_image_device(device))
			{
				(*proc)(opts, gamedrv, config, NULL, index++, device);
			}
		}
	} else {
		const device_config *device;
		for (device = config->devicelist.first(); device != NULL; device = device->next)
		{
			if (is_image_device(device))
			{
				(*proc)(opts, gamedrv, config, device, index++, NULL);
			}
		}
	}
}



/*-------------------------------------------------
    add_device_options_for_device - adds options
    for this device on a driver
-------------------------------------------------*/

static void add_device_options_for_device(core_options *opts, const game_driver *gamedrv,
	const machine_config *config, const device_config *device, int global_index, running_device *dev)
{
	image_device_info info;
	options_entry this_entry[2];
	char dev_full_name[128];

	/* first device? add the header as to be pretty */
	if (global_index == 0)
	{
		memset(this_entry, 0, sizeof(this_entry));
		this_entry[0].description = "MESS DEVICES";
		this_entry[0].flags = OPTION_HEADER;
		options_add_entries(opts, this_entry);
	}

	/* retrieve info about the device instance */
	info = image_device_getinfo(config, device);
	snprintf(dev_full_name, ARRAY_LENGTH(dev_full_name),
		"%s;%s", info.instance_name, info.brief_instance_name);

	/* add the option */
	memset(this_entry, 0, sizeof(this_entry));
	this_entry[0].name = dev_full_name;
	options_add_entries(opts, this_entry);
}



/*-------------------------------------------------
    mess_add_device_options - add all of the device
    options for a specified device
-------------------------------------------------*/

void mess_add_device_options(core_options *opts, const game_driver *driver)
{
	machine_config *config;

	/* create the configuration */
	config = machine_config_alloc(driver->machine_config);

	/* enumerate our callback for every device */
	mess_enumerate_devices(opts, config, driver, add_device_options_for_device, NULL);

	/* record that we've added device options */
	options_set_bool(opts, OPTION_ADDED_DEVICE_OPTIONS, TRUE, OPTION_PRIORITY_CMDLINE);

	/* free the configuration */
	machine_config_free(config);
}



/*-------------------------------------------------
    mess_driver_name_callback - called when we
    parse the driver name, so we can add options
    specific to that driver
-------------------------------------------------*/

static void mess_driver_name_callback(core_options *opts, const char *arg)
{
	const game_driver *driver;

	/* only add these options if we have not yet added them */
	if (!options_get_bool(opts, OPTION_ADDED_DEVICE_OPTIONS))
	{
		driver = driver_get_name(arg);
		if (driver != NULL)
		{
			mess_add_device_options(opts, driver);
		}
	}
}



/*-------------------------------------------------
    mess_options_init - called from core to add
    MESS specific options
-------------------------------------------------*/

void mess_options_init(core_options *opts)
{
	/* add MESS-specific options */
	options_add_entries(opts, mess_core_options);

	/* we need to dynamically add options when the device name is parsed */
	options_set_option_callback(opts, OPTION_GAMENAME, mess_driver_name_callback);
}



/*-------------------------------------------------
    extract_device_options_for_device - extracts
    options for this device on a driver
-------------------------------------------------*/

static void extract_device_options_for_device(core_options *opts, const game_driver *gamedrv,
	const machine_config *config, const device_config *device, int global_index, running_device *dev)
{
	image_device_info info;
	const char *filename;

	assert_always(options_get_bool(opts, OPTION_ADDED_DEVICE_OPTIONS), "extract_device_options_for_device() called without device options");

	/* access the filename */
	filename = image_filename(dev);

	/* access other info */
	info = image_device_getinfo(config, dev);

	/* and set the option */
	options_set_string(opts, info.instance_name, filename ? filename : "", OPTION_PRIORITY_CMDLINE);
}



/*-------------------------------------------------
    write_config - emit current option statuses as
    INI files
-------------------------------------------------*/

static int write_config(const char *filename, const game_driver *gamedrv)
{
	file_error filerr;
	mame_file *f;
	char buffer[128];
	int retval = 1;

	if (gamedrv != NULL)
	{
		sprintf(buffer, "%s.ini", gamedrv->name);
		filename = buffer;
	}

	filerr = mame_fopen(SEARCHPATH_INI, buffer, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &f);
	if (filerr != FILERR_NONE)
		goto done;

	options_output_ini_file(mame_options(), mame_core_file(f));
	retval = 0;

done:
	if (f != NULL)
		mame_fclose(f);
	return retval;
}



/*-------------------------------------------------
    mess_options_extract - extract device options
    out of core into the options
-------------------------------------------------*/

void mess_options_extract(running_machine *machine)
{
	/* only extract the device options if we've added them */
	if (options_get_bool(mame_options(), OPTION_ADDED_DEVICE_OPTIONS))
		mess_enumerate_devices(mame_options(), machine->config, machine->gamedrv, extract_device_options_for_device, machine);

	/* write the config, if appropriate */
	if (options_get_bool(mame_options(), OPTION_WRITECONFIG))
		write_config(NULL, machine->gamedrv);
}
