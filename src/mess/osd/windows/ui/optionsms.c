#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>
#include <winuser.h>
#include <tchar.h>

#include "ui/m32util.h"
#include "ui/m32opts.h"
#include "optionsms.h"
#include "emuopts.h"
#include "driver.h"

#define WINGUIOPTION_SOFTWARE_COLUMN_SHOWN		"mess_column_shown"
#define WINGUIOPTION_SOFTWARE_COLUMN_WIDTHS		"mess_column_widths"
#define WINGUIOPTION_SOFTWARE_COLUMN_ORDER		"mess_column_order"
#define WINGUIOPTION_SOFTWARE_SORT_REVERSED		"mess_sort_reversed"
#define WINGUIOPTION_SOFTWARE_SORT_COLUMN		"mess_sort_column"
#define WINGUIOPTION_SOFTWARE_TAB				"current_software_tab"
#define WINGUIOPTION_SOFTWAREPATH				"softwarepath"

#define LOG_SOFTWARE	0

const options_entry mess_wingui_settings[] =
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

void SetMessColumnOrder(int order[])
{
	char column_order_string[10000];
	ColumnEncodeStringWithCount(order, column_order_string, MESS_COLUMN_MAX);
	options_set_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_COLUMN_ORDER, column_order_string);
}

void GetMessColumnOrder(int order[])
{
	const char *column_order_string;
	column_order_string = options_get_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_COLUMN_ORDER);
	ColumnDecodeStringWithCount(column_order_string, order, MESS_COLUMN_MAX);
}

void SetMessColumnShown(int shown[])
{
	char column_shown_string[10000];
	ColumnEncodeStringWithCount(shown, column_shown_string, MESS_COLUMN_MAX);
	options_set_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_COLUMN_SHOWN, column_shown_string);
}

void GetMessColumnShown(int shown[])
{
	const char *column_shown_string;
	column_shown_string = options_get_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_COLUMN_SHOWN);
	ColumnDecodeStringWithCount(column_shown_string, shown, MESS_COLUMN_MAX);
}

void SetMessColumnWidths(int width[])
{
	char column_width_string[10000];
	ColumnEncodeStringWithCount(width, column_width_string, MESS_COLUMN_MAX);
	options_set_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_COLUMN_WIDTHS, column_width_string);
}

void GetMessColumnWidths(int width[])
{
	const char *column_width_string;
	column_width_string = options_get_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_COLUMN_WIDTHS);
	ColumnDecodeStringWithCount(column_width_string, width, MESS_COLUMN_MAX);
}

void SetMessSortColumn(int column)
{
	options_set_int(Mame32Settings(), WINGUIOPTION_SOFTWARE_SORT_COLUMN, column);
}

int GetMessSortColumn(void)
{
	return options_get_int(Mame32Settings(), WINGUIOPTION_SOFTWARE_SORT_COLUMN);
}

void SetMessSortReverse(BOOL reverse)
{
	options_set_bool(Mame32Settings(), WINGUIOPTION_SOFTWARE_SORT_REVERSED, reverse);
}

BOOL GetMessSortReverse(void)
{
	return options_get_bool(Mame32Settings(), WINGUIOPTION_SOFTWARE_SORT_REVERSED);
}

const char* GetSoftwareDirs(void)
{
	return options_get_string(Mame32Settings(), WINGUIOPTION_SOFTWAREPATH);
}

void SetSoftwareDirs(const char* paths)
{
	options_set_string(Mame32Settings(), WINGUIOPTION_SOFTWAREPATH, paths);
}

const char *GetHashDirs(void)
{
	return options_get_string(Mame32Global(), OPTION_HASHPATH);
}

void SetHashDirs(const char *paths)
{
	options_set_string(Mame32Global(), OPTION_HASHPATH, paths);
	options_set_string(mame_options(), OPTION_HASHPATH, paths);
}

void SetSelectedSoftware(int driver_index, const device_class *devclass, int device_inst, const char *software)
{
	const char *opt_name = device_instancename(devclass, device_inst);
	core_options *o;

	if (LOG_SOFTWARE)
	{
		dprintf("SetSelectedSoftware(): driver_index=%d (\'%s\') devclass=%p device_inst=%d software='%s'\n",
			driver_index, drivers[driver_index]->name, devclass, device_inst, software);
	}

	o = GetGameOptions(driver_index, -1);
	opt_name = device_instancename(devclass, device_inst);
	// FIXME
	//options_set_string(o, opt_name, software);
}

const char *GetSelectedSoftware(int driver_index, const device_class *devclass, int device_inst)
{
	const char *opt_name = device_instancename(devclass, device_inst);
	const char *software;
	core_options *o;

	o = GetGameOptions(driver_index, -1);
	opt_name = device_instancename(devclass, device_inst);
	software = NULL; // FIXME - options_get_string(o, opt_name);
	return software ? software : "";
}

void SetExtraSoftwarePaths(int driver_index, const char *extra_paths)
{
	char *new_extra_paths = NULL;

	assert(driver_index >= 0);
	assert(driver_index < driver_get_count());

	if (extra_paths && *extra_paths)
	{
		new_extra_paths = mame_strdup(extra_paths);
		if (!new_extra_paths)
			return;
	}
	//	TODO: FIXME
	//FreeIfAllocated(&game_variables[driver_index].mess.extra_software_paths);
	//game_variables[driver_index].mess.extra_software_paths = new_extra_paths;
}

const char *GetExtraSoftwarePaths(int driver_index)
{
	const char *paths;

	assert(driver_index >= 0);
	assert(driver_index < driver_get_count());

	//	TODO: FIXME
	//paths = game_variables[driver_index].mess.extra_software_paths;
	paths = NULL;
	return paths ? paths : "";
}

void SetCurrentSoftwareTab(const char *shortname)
{
	options_set_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_TAB, shortname);
}

const char *GetCurrentSoftwareTab(void)
{
	return options_get_string(Mame32Settings(), WINGUIOPTION_SOFTWARE_TAB);
}
