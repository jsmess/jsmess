#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>
#include <winuser.h>

#include "emu.h"
#include "mui_util.h"
#include "mui_opts.h"
#include "optionsms.h"
#include "emuopts.h"
#include "messopts.h"
#include "winmain.h"

#define WINGUIOPTION_SOFTWARE_COLUMN_SHOWN		"mess_column_shown"
#define WINGUIOPTION_SOFTWARE_COLUMN_WIDTHS		"mess_column_widths"
#define WINGUIOPTION_SOFTWARE_COLUMN_ORDER		"mess_column_order"
#define WINGUIOPTION_SOFTWARE_SORT_REVERSED		"mess_sort_reversed"
#define WINGUIOPTION_SOFTWARE_SORT_COLUMN		"mess_sort_column"
#define WINGUIOPTION_SOFTWARE_TAB				"current_software_tab"
#define WINGUIOPTION_SOFTWAREPATH				"softwarepath"

#define LOG_SOFTWARE	0

static const options_entry mess_wingui_settings[] =
{
	{ WINGUIOPTION_SOFTWARE_COLUMN_WIDTHS,	"186, 230, 88, 84, 84, 68, 248, 248",	0,				NULL },
	{ WINGUIOPTION_SOFTWARE_COLUMN_ORDER,	"0,   1,    2,  3,  4,  5,   6,   7",	0,				NULL },
	{ WINGUIOPTION_SOFTWARE_COLUMN_SHOWN,	"1,   1,    1,  1,  1,  0,   0,   0",	0,				NULL },

	{ WINGUIOPTION_SOFTWARE_SORT_COLUMN,	"0",									0,				NULL },
	{ WINGUIOPTION_SOFTWARE_SORT_REVERSED,	"0",									OPTION_BOOLEAN,	NULL },

	{ WINGUIOPTION_SOFTWARE_TAB,			"0",									0,				NULL },
	{ WINGUIOPTION_SOFTWAREPATH,			"software",								0,				NULL },
	{ NULL }
};

void MessSetupSettings(core_options *settings)
{
	options_add_entries(settings, mess_wingui_settings);
}

void MessSetupGameOptions(core_options *opts, int driver_index)
{
	BOOL is_global = (driver_index == OPTIONS_TYPE_GLOBAL);
	AddOptions(opts, mess_core_options, is_global);

	if (driver_index >= 0)
	{
		image_add_device_options(opts, drivers[driver_index]);
	}
}

void SetMessColumnOrder(int order[])
{
	char column_order_string[10000];
	ColumnEncodeStringWithCount(order, column_order_string, MESS_COLUMN_MAX);
	options_set_string(MameUISettings(), WINGUIOPTION_SOFTWARE_COLUMN_ORDER, column_order_string, OPTION_PRIORITY_CMDLINE);
}

void GetMessColumnOrder(int order[])
{
	const char *column_order_string;
	column_order_string = options_get_string(MameUISettings(), WINGUIOPTION_SOFTWARE_COLUMN_ORDER);
	ColumnDecodeStringWithCount(column_order_string, order, MESS_COLUMN_MAX);
}

void SetMessColumnShown(int shown[])
{
	char column_shown_string[10000];
	ColumnEncodeStringWithCount(shown, column_shown_string, MESS_COLUMN_MAX);
	options_set_string(MameUISettings(), WINGUIOPTION_SOFTWARE_COLUMN_SHOWN, column_shown_string, OPTION_PRIORITY_CMDLINE);
}

void GetMessColumnShown(int shown[])
{
	const char *column_shown_string;
	column_shown_string = options_get_string(MameUISettings(), WINGUIOPTION_SOFTWARE_COLUMN_SHOWN);
	ColumnDecodeStringWithCount(column_shown_string, shown, MESS_COLUMN_MAX);
}

void SetMessColumnWidths(int width[])
{
	char column_width_string[10000];
	ColumnEncodeStringWithCount(width, column_width_string, MESS_COLUMN_MAX);
	options_set_string(MameUISettings(), WINGUIOPTION_SOFTWARE_COLUMN_WIDTHS, column_width_string, OPTION_PRIORITY_CMDLINE);
}

void GetMessColumnWidths(int width[])
{
	const char *column_width_string;
	column_width_string = options_get_string(MameUISettings(), WINGUIOPTION_SOFTWARE_COLUMN_WIDTHS);
	ColumnDecodeStringWithCount(column_width_string, width, MESS_COLUMN_MAX);
}

void SetMessSortColumn(int column)
{
	options_set_int(MameUISettings(), WINGUIOPTION_SOFTWARE_SORT_COLUMN, column, OPTION_PRIORITY_CMDLINE);
}

int GetMessSortColumn(void)
{
	return options_get_int(MameUISettings(), WINGUIOPTION_SOFTWARE_SORT_COLUMN);
}

void SetMessSortReverse(BOOL reverse)
{
	options_set_bool(MameUISettings(), WINGUIOPTION_SOFTWARE_SORT_REVERSED, reverse, OPTION_PRIORITY_CMDLINE);
}

BOOL GetMessSortReverse(void)
{
	return options_get_bool(MameUISettings(), WINGUIOPTION_SOFTWARE_SORT_REVERSED);
}

const char* GetSoftwareDirs(void)
{
	return options_get_string(MameUISettings(), WINGUIOPTION_SOFTWAREPATH);
}

void SetSoftwareDirs(const char* paths)
{
	options_set_string(MameUISettings(), WINGUIOPTION_SOFTWAREPATH, paths, OPTION_PRIORITY_CMDLINE);
}

const char *GetHashDirs(void)
{
	return options_get_string(MameUIGlobal(), OPTION_HASHPATH);
}

void SetHashDirs(const char *paths)
{
	options_set_string(MameUIGlobal(), OPTION_HASHPATH, paths, OPTION_PRIORITY_CMDLINE);
}

void SetSelectedSoftware(int driver_index, const machine_config *config, const device_config_image_interface *dev, const char *software)
{
	const char *opt_name = dev->instance_name();
	core_options *o;

	if (LOG_SOFTWARE)
	{
		dprintf("SetSelectedSoftware(): dev=%p (\'%s\') software='%s'\n",
			dev, drivers[driver_index]->name, software);
	}

	o = load_options(OPTIONS_GAME, driver_index);
	options_set_string(o, opt_name, software, OPTION_PRIORITY_CMDLINE);
	save_options(OPTIONS_GAME, o, driver_index);
	options_free(o);
}

const char *GetSelectedSoftware(int driver_index, const machine_config *config, const device_config_image_interface *dev)
{
	const char *opt_name = dev->instance_name();
	const char *software;
	core_options *o;

	o = load_options(OPTIONS_GAME, driver_index);
	software = options_get_string(o, opt_name);
	return software ? software : "";
}

void SetExtraSoftwarePaths(int driver_index, const char *extra_paths)
{
	char opt_name[32];

	assert(0 <= driver_index && driver_index < driver_list_get_count(drivers));

	snprintf(opt_name, ARRAY_LENGTH(opt_name), "%s_extra_software", drivers[driver_index]->name);
	options_set_string(MameUISettings(), opt_name, extra_paths, OPTION_PRIORITY_CMDLINE);
}

const char *GetExtraSoftwarePaths(int driver_index)
{
	char opt_name[32];
	const char *paths;

	assert(0 <= driver_index && driver_index < driver_list_get_count(drivers));

	snprintf(opt_name, ARRAY_LENGTH(opt_name), "%s_extra_software", drivers[driver_index]->name);
	paths = options_get_string(MameUISettings(), opt_name);
	return paths ? paths : "";
}

void SetCurrentSoftwareTab(const char *shortname)
{
	options_set_string(MameUISettings(), WINGUIOPTION_SOFTWARE_TAB, shortname, OPTION_PRIORITY_CMDLINE);
}

const char *GetCurrentSoftwareTab(void)
{
	return options_get_string(MameUISettings(), WINGUIOPTION_SOFTWARE_TAB);
}
