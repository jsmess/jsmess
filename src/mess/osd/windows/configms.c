//============================================================
//
//	configms.c - Win32 MESS specific options
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>

// MESS headers
#include "driver.h"
#include "windows/config.h"
#include "parallel.h"
#include "menu.h"
#include "device.h"
#include "configms.h"
#include "mscommon.h"
#include "pool.h"
#include "config.h"

//============================================================
//	GLOBAL VARIABLES
//============================================================

int win_write_config;

//============================================================
//	LOCAL VARIABLES
//============================================================

static int added_device_options;
static char *dev_dirs[IO_COUNT];
static memory_pool mess_options_pool;

static const options_entry mess_opts[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"MESS SPECIFIC OPTIONS" },
	{ "newui;nu",                   "1",    OPTION_BOOLEAN,		"use the new MESS UI" },
    { "ramsize;ram",				NULL,	0,					"size of RAM (if supported by driver)" },
	{ "threads;thr",				"0",	0,					"number of threads to use for parallel operations" },
	{ "natural;nat",				"0",	OPTION_BOOLEAN,		"specifies whether to use a natural keyboard or not" },
	{ "min_width;mw",				"200",	0,					"specifies the minimum width for the display" },
	{ "min_height;mh",				"200",	0,					"specifies the minimum height for the display" },
	{ "writeconfig;wc",				"0",	OPTION_BOOLEAN,		"writes configuration to (driver).ini on exit" },
	{ "skip_warnings",				"0",    OPTION_BOOLEAN,		"skip displaying the warnings screen" },
	{ NULL }
};

//============================================================



/*
 * gamedrv  = NULL --> write named configfile
 * gamedrv != NULL --> write gamename.ini and all parent.ini's (recursively)
 * return 0 --> no problem
 * return 1 --> something went wrong
 */
int write_config(const char* filename, const game_driver *gamedrv)
{
	mame_file_error filerr;
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



const char *get_devicedirectory(int dev)
{
	assert(dev >= 0);
	assert(dev < IO_COUNT);
	return dev_dirs[dev];
}



void set_devicedirectory(int dev, const char *dir)
{
	assert(dev >= 0);
	assert(dev < IO_COUNT);
	if (dev_dirs[dev])
		free(dev_dirs[dev]);
	dev_dirs[dev] = mame_strdup(dir);
}



//============================================================
//	Device options
//============================================================

typedef struct _device_rc_option device_rc_option;
struct _device_rc_option
{
	// options for the RC system
	options_entry opts[2];

	// device information
	iodevice_t devtype;
	const char *tag;
	int index;

	// mounted file
	char *filename;
};

typedef struct _device_type_options device_type_options;
struct _device_type_options
{
	int count;
	device_rc_option *opts[MAX_DEV_INSTANCES];
};

device_type_options *device_options;



void device_dirs_load(int config_type, xml_data_node *parentnode)
{
	iodevice_t dev;

	// on init, reset the directories
	if (config_type == CONFIG_TYPE_INIT)
		memset(dev_dirs, 0, sizeof(dev_dirs));

	// only care about game-specific data
	if (config_type != CONFIG_TYPE_GAME)
		return;

	// might not have any data
	if (!parentnode)
		return;

	for (dev = 0; dev < IO_COUNT; dev++)
	{
	}
}



void device_dirs_save(int config_type, xml_data_node *parentnode)
{
	xml_data_node *node;
	iodevice_t dev;

	// only care about game-specific data
	if (config_type != CONFIG_TYPE_GAME)
		return;

	for (dev = 0; dev < IO_COUNT; dev++)
	{
		if (dev_dirs[dev])
		{
			node = xml_add_child(parentnode, "device", NULL);
			if (node)
			{
				xml_set_attribute(node, "type", device_typename(dev));
				xml_set_attribute(node, "directory", dev_dirs[dev]);
			}
		}
	}
}



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
		opts = pool_malloc(&mess_options_pool, sizeof(*opts) * 2);
		memset(opts, 0, sizeof(*opts) * 2);
		opts[0].description = "MESS DEVICES";
		opts[0].flags = OPTION_HEADER;
		options_add_entries(opts);

		// we need to save all options
		device_options = pool_malloc(&mess_options_pool, sizeof(*device_options) * (dev_count + 1));
		memset(device_options, 0, sizeof(*device_options) * (dev_count + 1));

		// list all options
		for (dev = 0; dev < dev_count; dev++)
		{
			devclass.gamedrv = gamedrv;
			devclass.get_info = handlers[dev];

			// retrieve info about the device
			devtype = (iodevice_t) (int) device_get_info_int(&devclass, DEVINFO_INT_TYPE);
			count = (int) device_get_info_int(&devclass, DEVINFO_INT_COUNT);
			dev_tag = pool_strdup(&mess_options_pool, device_get_info_string(&devclass, DEVINFO_STR_DEV_TAG));

			device_options[dev].count = count;

			for (id = 0; id < count; id++)
			{
				// retrieve info about hte device instance
				dev_name = device_instancename(&devclass, id);
				dev_short_name = device_briefinstancename(&devclass, id);
				snprintf(dev_full_name, sizeof(dev_full_name) / sizeof(dev_full_name[0]),
					"%s;%s", dev_name, dev_short_name);

				// dynamically allocate the option
				dev_option = pool_malloc(&mess_options_pool, sizeof(*dev_option));
				memset(dev_option, 0, sizeof(*dev_option));

				// populate the options
				dev_option->opts[0].name = pool_strdup(&mess_options_pool, dev_full_name);
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



static void win_mess_exit(running_machine *machine)
{
	if (win_write_config)
		write_config(NULL, machine->gamedrv);
}



void win_mess_config_init(running_machine *machine)
{
	config_register("device_directories", device_dirs_load, device_dirs_save);
	add_exit_callback(machine, win_mess_exit);
}



void osd_begin_final_unloading(void)
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
					pool_freeptr(&mess_options_pool, *filename_ptr);
					*filename_ptr = NULL;
				}

				// locate image
				image = image_from_device_and_index(dev, i);

				// place new filename, if there
				if (image)
					*filename_ptr = pool_strdup(&mess_options_pool, image_filename(image));
			}
			opt++;
		}
	}

	// free up all of the allocated options
	pool_exit(&mess_options_pool);
}



int osd_select_file(mess_image *img, char *filename)
{
	return 0;
}



void osd_image_load_status_changed(mess_image *img, int is_final_unload)
{
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

	options.skip_warnings = options_get_bool("skip_warnings");
	options.disable_normal_ui = options_get_bool("newui");
	options.ram = specified_ram;
	options.min_width = options_get_int("min_width");
	options.min_height = options_get_int("min_height");

	win_task_count = options_get_int("threads");
	win_use_natural_keyboard = options_get_bool("natural");
	win_write_config = options_get_bool("writeconfig");

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
					options.image_files[options.image_count].name = pool_strdup(&mess_options_pool, filename);
					options.image_count++;
				}
			}
		}
	}
}



static void win_mess_driver_name_callback(const char *arg)
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



void win_mess_options_init(void)
{
	added_device_options = FALSE;
	options_add_entries(mess_opts);
	options_set_option_callback(OPTION_UNADORNED(0), win_mess_driver_name_callback);
	pool_init(&mess_options_pool);
}
