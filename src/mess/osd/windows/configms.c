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
//	LOCAL VARIABLES
//============================================================

static char *dev_dirs[IO_COUNT];

static const options_entry win_mess_opts[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"WINDOWS MESS SPECIFIC OPTIONS" },
	{ "newui;nu",                   "1",    OPTION_BOOLEAN,		"use the new MESS UI" },
	{ "threads;thr",				"0",	0,					"number of threads to use for parallel operations" },
	{ "natural;nat",				"0",	OPTION_BOOLEAN,		"specifies whether to use a natural keyboard or not" },
	{ NULL }
};

//============================================================

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



void osd_mess_options_init(void)
{
	options_add_entries(win_mess_opts);

	win_task_count = options_get_int("threads");
	win_use_natural_keyboard = options_get_bool("natural");
}



void osd_mess_config_init(running_machine *machine)
{
	config_register("device_directories", device_dirs_load, device_dirs_save);
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
