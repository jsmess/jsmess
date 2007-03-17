/***************************************************************************

	messopts.c - MESS specific options

****************************************************************************/

#include "mame.h"
#include "options.h"
#include "driver.h"
#include "pool.h"
#include "config.h"
#include "xmlfile.h"

#include "osd/windows/configms.h"


/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

static int added_device_options;
static memory_pool *mess_options_pool;
static int mess_write_config;

static const options_entry mess_opts[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"MESS SPECIFIC OPTIONS" },
    { "ramsize;ram",				NULL,	0,					"size of RAM (if supported by driver)" },
	{ "writeconfig;wc",				"0",	OPTION_BOOLEAN,		"writes configuration to (driver).ini on exit" },
	{ "skip_warnings",				"0",    OPTION_BOOLEAN,		"skip displaying the warnings screen" },
	{ NULL }
};



/*
 * gamedrv  = NULL --> write named configfile
 * gamedrv != NULL --> write gamename.ini and all parent.ini's (recursively)
 * return 0 --> no problem
 * return 1 --> something went wrong
 */
int write_config(const char* filename, const game_driver *gamedrv)
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



/***************************************************************************
    DEVICE OPTIONS
***************************************************************************/

typedef struct _device_rc_option device_rc_option;
struct _device_rc_option
{
	/* options for the RC system */
	options_entry opts[2];

	/* device information */
	iodevice_t devtype;
	const char *tag;
	int index;

	/* mounted file */
	char *filename;
};



typedef struct _device_type_options device_type_options;
struct _device_type_options
{
	int count;
	device_rc_option *opts[MAX_DEV_INSTANCES];
};

device_type_options *device_options;



void win_add_mess_device_options(const game_driver *gamedrv)
{
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	device_class devclass;
	iodevice_t devtype;
	int dev_count, dev, id, count;
	device_rc_option *dev_option;
	options_entry *opts;
	const char *dev_name;
	const char *dev_short_name;
	const char *dev_tag_original;
	const char *dev_tag;
	char dev_full_name[128];

	// retrieve getinfo handlers
	memset(&cfg, 0, sizeof(cfg));
	memset(handlers, 0, sizeof(handlers));
	cfg.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
	cfg.device_handlers = handlers;
	cfg.device_countoverrides = count_overrides;
	if (gamedrv->sysconfig_ctor)
		gamedrv->sysconfig_ctor(&cfg);

	// count devides
	for (dev_count = 0; handlers[dev_count]; dev_count++)
		;

	if (dev_count > 0)
	{
		// add a separator
		opts = pool_malloc(mess_options_pool, sizeof(*opts) * 2);
		memset(opts, 0, sizeof(*opts) * 2);
		opts[0].description = "MESS DEVICES";
		opts[0].flags = OPTION_HEADER;
		options_add_entries(opts);

		// we need to save all options
		device_options = pool_malloc(mess_options_pool, sizeof(*device_options) * (dev_count + 1));
		memset(device_options, 0, sizeof(*device_options) * (dev_count + 1));

		// list all options
		for (dev = 0; dev < dev_count; dev++)
		{
			devclass.gamedrv = gamedrv;
			devclass.get_info = handlers[dev];

			// retrieve info about the device
			devtype = (iodevice_t) (int) device_get_info_int(&devclass, DEVINFO_INT_TYPE);
			count = (int) device_get_info_int(&devclass, DEVINFO_INT_COUNT);
			dev_tag_original = device_get_info_string(&devclass, DEVINFO_STR_DEV_TAG);
			dev_tag = dev_tag_original ? pool_strdup(mess_options_pool, dev_tag_original) : dev_tag_original;

			device_options[dev].count = count;

			for (id = 0; id < count; id++)
			{
				// retrieve info about hte device instance
				dev_name = device_instancename(&devclass, id);
				dev_short_name = device_briefinstancename(&devclass, id);
				snprintf(dev_full_name, sizeof(dev_full_name) / sizeof(dev_full_name[0]),
					"%s;%s", dev_name, dev_short_name);

				// dynamically allocate the option
				dev_option = pool_malloc(mess_options_pool, sizeof(*dev_option));
				memset(dev_option, 0, sizeof(*dev_option));

				// populate the options
				dev_option->opts[0].name = pool_strdup(mess_options_pool, dev_full_name);
				dev_option->devtype = devtype;
				dev_option->tag = dev_tag;
				dev_option->index = id;

				// register these options
				device_options[dev].opts[id] = dev_option;
				options_add_entries(dev_option->opts);
			}
		}
	}
}



static void mess_exit(running_machine *machine)
{
	if (mess_write_config)
		write_config(NULL, machine->gamedrv);
}



void mess_config_init(running_machine *machine)
{
	win_mess_config_init(machine);
	add_exit_callback(machine, mess_exit);
}



void mess_begin_final_unloading(void)
{
	int opt = 0, i;
	const struct IODevice *dev;
	mess_image *image;
	char **filename_ptr;

	if (Machine->devices)
	{
		for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		{
			for (i = 0; i < device_options[opt].count; i++)
			{
				// free existing string, if there
				filename_ptr = &device_options[opt].opts[i]->filename;
				if (*filename_ptr)
				{
					pool_realloc(mess_options_pool, *filename_ptr, 0);
					*filename_ptr = NULL;
				}

				// locate image
				image = image_from_device_and_index(dev, i);

				// place new filename, if there
				if (image_exists(image))
					*filename_ptr = pool_strdup(mess_options_pool, image_filename(image));
			}
			opt++;
		}
	}
}



void win_mess_extract_options(void)
{
	UINT32 specified_ram = 0;
	const char *arg;
	device_rc_option *dev_option;
	const char *optionname;
	const char *filename;
	const char *s;
	int i, j;

	arg = options_get_string("ramsize");
	if (arg)
	{
		specified_ram = ram_parse_string(arg);
		if (specified_ram == 0)
		{
			fprintf(stderr, "Cannot recognize the RAM option %s\n", arg);
			exit(-1);
		}
	}

	#ifdef WIN32
	options.disable_normal_ui = options_get_bool("newui");
	#else
	options.disable_normal_ui = 0;
	#endif
	options.ram = specified_ram;
	#ifdef WIN32
	options.min_width = options_get_int("min_width");
	options.min_height = options_get_int("min_height");
	#else
	options.min_width = options.min_height = 0;
	#endif

	mess_write_config = options_get_bool("writeconfig");

	if (device_options)
	{
		for (i = 0; device_options[i].count > 0; i++)
		{
			for (j = 0; j < device_options[i].count; j++)
			{
				dev_option = device_options[i].opts[j];

				// get the correct option name
				optionname = dev_option->opts[0].name;
				while((s = strchr(optionname, ';')) != NULL)
					optionname = s + 1;

				filename = options_get_string(optionname);

				if (filename)
				{
					// the user specified a device type
					options.image_files[options.image_count].device_type = dev_option->devtype;
					options.image_files[options.image_count].device_tag = dev_option->tag;
					options.image_files[options.image_count].device_index = dev_option->index;
					options.image_files[options.image_count].name = pool_strdup(mess_options_pool, filename);
					options.image_count++;
				}
			}
		}
	}
}



static void mess_driver_name_callback(const char *arg)
{
	int drvnum;

	if (!added_device_options)
	{
		drvnum = driver_get_index(arg);
		if (drvnum >= 0)
			win_add_mess_device_options(drivers[drvnum]);
		added_device_options = TRUE;
	}
}



void mess_options_init(void)
{
	added_device_options = FALSE;
	options_add_entries(mess_opts);
	options_set_option_callback(OPTION_UNADORNED(0), mess_driver_name_callback);
	mess_options_pool = pool_create(NULL);

#ifdef WIN32
	win_mess_options_init();
#endif
}
