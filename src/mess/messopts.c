/***************************************************************************

	messopts.c - MESS specific option handling

****************************************************************************/

#include "mame.h"
#include "options.h"
#include "driver.h"
#include "pool.h"
#include "config.h"
#include "xmlfile.h"
#include "messopts.h"
#include "osdmess.h"



/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static int added_device_options;

static const options_entry mess_opts[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"MESS SPECIFIC OPTIONS" },
    { "ramsize;ram",				NULL,	0,					"size of RAM (if supported by driver)" },
	{ "writeconfig;wc",				"0",	OPTION_BOOLEAN,		"writes configuration to (driver).ini on exit" },
	{ "skip_warnings",				"0",    OPTION_BOOLEAN,		"skip displaying the warnings screen" },
	{ NULL }
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    enumerate_devices - runs a specified proc
	for all devices on a driver
-------------------------------------------------*/

static void enumerate_devices(const game_driver *gamedrv, void (*proc)(const game_driver *gamedrv,
	const device_class *devclass, int device_index, int global_index))
{
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	int count_overrides[ARRAY_LENGTH(handlers)];
	int i, j, count, index = 0;
	device_class devclass;

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
		count = (int) device_get_info_int(&devclass, DEVINFO_INT_COUNT);

		/* loop on each device instance */
		for (j = 0; j < count; j++)
		{
			proc(gamedrv, &devclass, j, index++);
		}
	}
}



/*-------------------------------------------------
    add_device_options_for_device - adds options
	for this device on a driver
-------------------------------------------------*/

static void add_device_options_for_device(const game_driver *gamedrv, const device_class *devclass,
	int device_index, int global_index)
{
	options_entry opts[2];
	const char *dev_name;
	const char *dev_short_name;
	char dev_full_name[128];

	/* first device? add the header as to be pretty */
	if (global_index == 0)
	{
		memset(opts, 0, sizeof(opts));
		opts[0].description = "MESS DEVICES";
		opts[0].flags = OPTION_HEADER;
		options_add_entries(opts);
	}

	/* retrieve info about the device instance */
	dev_name = device_instancename(devclass, device_index);
	dev_short_name = device_briefinstancename(devclass, device_index);
	snprintf(dev_full_name, sizeof(dev_full_name) / sizeof(dev_full_name[0]),
		"%s;%s", dev_name, dev_short_name);

	/* add the option */
	memset(opts, 0, sizeof(opts));
	opts[0].name = dev_full_name;
	options_add_entries(opts);
}



/*-------------------------------------------------
    mess_driver_name_callback - called when we
	parse the driver name, so we can add options
	specific to that driver
-------------------------------------------------*/

static void mess_driver_name_callback(const char *arg)
{
	int drvnum;

	/* only add these options if we have not yet added them */
	if (!added_device_options)
	{
		drvnum = driver_get_index(arg);
		if (drvnum >= 0)
		{
			/* add all of the device options for this driver */
			enumerate_devices(drivers[drvnum], add_device_options_for_device);
		}
		added_device_options = TRUE;
	}
}



/*-------------------------------------------------
    mess_options_init - called from core to add
	MESS specific options
-------------------------------------------------*/

void mess_options_init(void)
{
	/* add MESS-specific options */
	options_add_entries(mess_opts);

	/* add OSD-MESS specific options (hack!) */
	osd_mess_options_init();

	/* hacky global variable to track whether we have added device options */
	added_device_options = FALSE;

	/* we need to dynamically add options when the device name is parsed */
	options_set_option_callback(OPTION_UNADORNED(0), mess_driver_name_callback);
}



/*-------------------------------------------------
    extract_device_options_for_device - extracts
	options for this device on a driver
-------------------------------------------------*/

static void extract_device_options_for_device(const game_driver *gamedrv, const device_class *devclass,
	int device_index, int global_index)
{
	mess_image *image;
	iodevice_t dev_type;
	const char *dev_tag;
	const char *dev_name;
	const char *filename;

	/* identify the correct image */
	dev_tag = device_get_info_string(devclass, DEVINFO_STR_DEV_TAG);
	if (dev_tag != NULL)
	{
		image = image_from_devtag_and_index(dev_tag, device_index);
	}
	else
	{
		dev_type = (iodevice_t) (int) device_get_info_int(devclass, DEVINFO_INT_TYPE);
		image = image_from_devtype_and_index(dev_type, device_index);
	}

	/* access the filename */
	filename = image_filename(image);

	/* identify the option name */
	dev_name = device_instancename(devclass, device_index);

	/* and set the option */
	options_set_string(dev_name, filename ? filename : "");
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

	options_output_ini_mame_file(f);
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
	enumerate_devices(Machine->gamedrv, extract_device_options_for_device);

	if (options_get_bool(OPTION_WRITECONFIG))
	{
		write_config(NULL, Machine->gamedrv);
	}
}
