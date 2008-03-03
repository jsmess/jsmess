/***************************************************************************

	messopts.c - MESS specific option handling

****************************************************************************/

#include "mame.h"
#include "deprecat.h"
#include "options.h"
#include "driver.h"
#include "pool.h"
#include "config.h"
#include "xmlfile.h"
#include "messopts.h"
#include "osdmess.h"

#define OPTION_ADDED_DEVICE_OPTIONS	"added_device_options"


/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

const options_entry mess_core_options[] =
{
	{ NULL,							NULL,   OPTION_HEADER,						"MESS SPECIFIC OPTIONS" },
    { "ramsize;ram",				NULL,	0,									"size of RAM (if supported by driver)" },
	{ "writeconfig;wc",				"0",	OPTION_BOOLEAN,						"writes configuration to (driver).ini on exit" },
	{ OPTION_SKIP_WARNINGS,			"0",    OPTION_BOOLEAN,						"skip displaying the warnings screen" },
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

const char *mess_get_device_option(const mess_device_class *devclass, int device_index)
{
	const char *result = NULL;
	const char *dev_name;

	if (options_get_bool(mame_options(), OPTION_ADDED_DEVICE_OPTIONS))
	{
		/* identify the option name */
		dev_name = device_instancename(devclass, device_index);

		/* access the option */
		result = options_get_string(mame_options(), dev_name);
	}
	return result;
}



/*-------------------------------------------------
    mess_enumerate_devices - runs a specified proc
	for all devices on a driver
-------------------------------------------------*/

static void mess_enumerate_devices(core_options *opts, const game_driver *gamedrv,
	void (*proc)(core_options *opts, const game_driver *gamedrv, const mess_device_class *devclass, int device_index, int global_index))
{
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	int count_overrides[ARRAY_LENGTH(handlers)];
	int i, j, count, index = 0;
	mess_device_class devclass;

	/* retrieve getinfo handlers */
	memset(&cfg, 0, sizeof(cfg));
	memset(handlers, 0, sizeof(handlers));
	cfg.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
	cfg.device_handlers = handlers;
	cfg.device_countoverrides = count_overrides;
	if (gamedrv->sysconfig_ctor)
		gamedrv->sysconfig_ctor(&cfg);

	/* run this on all devices */
	for (i = 0; handlers[i]; i++)
	{
		/* create the device class */
		memset(&devclass, 0, sizeof(devclass));
		devclass.gamedrv = gamedrv;
		devclass.get_info = handlers[i];

		/* how many of this device exist? */
		count = (int) mess_device_get_info_int(&devclass, MESS_DEVINFO_INT_COUNT);

		/* loop on each device instance */
		for (j = 0; j < count; j++)
		{
			proc(opts, gamedrv, &devclass, j, index++);
		}
	}
}



/*-------------------------------------------------
    add_device_options_for_device - adds options
	for this device on a driver
-------------------------------------------------*/

static void add_device_options_for_device(core_options *opts, const game_driver *gamedrv,
	const mess_device_class *devclass, int device_index, int global_index)
{
	options_entry this_entry[2];
	const char *dev_name;
	const char *dev_short_name;
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
	dev_name = device_instancename(devclass, device_index);
	dev_short_name = device_briefinstancename(devclass, device_index);
	snprintf(dev_full_name, sizeof(dev_full_name) / sizeof(dev_full_name[0]),
		"%s;%s", dev_name, dev_short_name);

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
	/* enumerate our callback for every device */
	mess_enumerate_devices(opts, driver, add_device_options_for_device);

	/* record that we've added device options */
	options_set_bool(opts, OPTION_ADDED_DEVICE_OPTIONS, TRUE, OPTION_PRIORITY_CMDLINE);
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

	/* add OSD-MESS specific options (hack!) */
	osd_mess_options_init(opts);

	/* we need to dynamically add options when the device name is parsed */
	options_set_option_callback(opts, OPTION_GAMENAME, mess_driver_name_callback);
}



/*-------------------------------------------------
    extract_device_options_for_device - extracts
	options for this device on a driver
-------------------------------------------------*/

static void extract_device_options_for_device(core_options *opts, const game_driver *gamedrv,
	const mess_device_class *devclass, int device_index, int global_index)
{
	mess_image *image;
	iodevice_t dev_type;
	const char *dev_tag;
	const char *dev_name;
	const char *filename;

	assert_always(options_get_bool(opts, OPTION_ADDED_DEVICE_OPTIONS), "extract_device_options_for_device() called without device options");

	/* identify the correct image */
	dev_tag = mess_device_get_info_string(devclass, MESS_DEVINFO_STR_DEV_TAG);
	if (dev_tag != NULL)
	{
		image = image_from_devtag_and_index(dev_tag, device_index);
	}
	else
	{
		dev_type = (iodevice_t) (int) mess_device_get_info_int(devclass, MESS_DEVINFO_INT_TYPE);
		image = image_from_devtype_and_index(dev_type, device_index);
	}

	/* access the filename */
	filename = image_filename(image);

	/* identify the option name */
	dev_name = device_instancename(devclass, device_index);

	/* and set the option */
	options_set_string(opts, dev_name, filename ? filename : "", OPTION_PRIORITY_CMDLINE);
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

	if (gamedrv)
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
	if (f)
		mame_fclose(f);
	return retval;
}



/*-------------------------------------------------
    mess_options_extract - extract device options
	out of core into the options
-------------------------------------------------*/

void mess_options_extract(void)
{
	/* only extract the device options if we've added them */
	if (options_get_bool(mame_options(), OPTION_ADDED_DEVICE_OPTIONS))
		mess_enumerate_devices(mame_options(), Machine->gamedrv, extract_device_options_for_device);

	/* write the config, if appropriate */
	if (options_get_bool(mame_options(), OPTION_WRITECONFIG))
		write_config(NULL, Machine->gamedrv);
}
