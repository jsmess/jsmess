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

#define LOG_SOFTWARE	0

static void MessColumnEncodeString(void* data, char *str);
static void MessColumnDecodeString(const char* str, void* data);
static void MessColumnDecodeWidths(const char* str, void* data);

#include "ui/options.c"

static void MessColumnEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, MESS_COLUMN_MAX);
}

static void MessColumnDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, MESS_COLUMN_MAX);
}

static void MessColumnDecodeWidths(const char* str, void* data)
{
	if (settings.view == VIEW_REPORT || settings.view == VIEW_GROUPED)
		MessColumnDecodeString(str, data);
}

void SetMessColumnWidths(int width[])
{
    int i;

    for (i=0; i < MESS_COLUMN_MAX; i++)
        settings.mess.mess_column_width[i] = width[i];
}

void GetMessColumnWidths(int width[])
{
    int i;

    for (i=0; i < MESS_COLUMN_MAX; i++)
        width[i] = settings.mess.mess_column_width[i];
}

void SetMessColumnOrder(int order[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        settings.mess.mess_column_order[i] = order[i];
}

void GetMessColumnOrder(int order[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        order[i] = settings.mess.mess_column_order[i];
}

void SetMessColumnShown(int shown[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        settings.mess.mess_column_shown[i] = shown[i];
}

void GetMessColumnShown(int shown[])
{
    int i;

    for (i = 0; i < MESS_COLUMN_MAX; i++)
        shown[i] = settings.mess.mess_column_shown[i];
}

void SetMessSortColumn(int column)
{
	settings.mess.mess_sort_column = column;
}

int GetMessSortColumn(void)
{
	return settings.mess.mess_sort_column;
}

void SetMessSortReverse(BOOL reverse)
{
	settings.mess.mess_sort_reverse = reverse;
}

BOOL GetMessSortReverse(void)
{
	return settings.mess.mess_sort_reverse;
}

const char* GetSoftwareDirs(void)
{
    return settings.mess.softwaredirs;
}

void SetSoftwareDirs(const char* paths)
{
	FreeIfAllocated(&settings.mess.softwaredirs);
    if (paths != NULL)
        settings.mess.softwaredirs = mame_strdup(paths);
}

const char *GetCrcDir(void)
{
	return settings.mess.hashdir;
}

void SetCrcDir(const char *hashdir)
{
	FreeIfAllocated(&settings.mess.hashdir);
    if (hashdir != NULL)
        settings.mess.hashdir = mame_strdup(hashdir);
}

BOOL GetUseNewUI(int driver_index)
{
    assert(0 <= driver_index && driver_index < driver_index);
    return GetGameOptions(driver_index, -1)->mess.use_new_ui;
}

void SetSelectedSoftware(int driver_index, int device_inst_index, const char *software)
{
	char *newsoftware;
	options_type *o;

	assert(device_inst_index >= 0);
	assert(device_inst_index < (sizeof(o->mess.software) / sizeof(o->mess.software[0])));

	if (LOG_SOFTWARE)
	{
		dprintf("SetSelectedSoftware(): driver_index=%d (\'%s\') device_inst_index=%d software='%s'\n",
			driver_index, drivers[driver_index]->name, device_inst_index, software);
	}

	newsoftware = mame_strdup(software ? software : "");
	if (!newsoftware)
		return;

	o = GetGameOptions(driver_index, -1);
	FreeIfAllocated(&o->mess.software[device_inst_index]);
	o->mess.software[device_inst_index] = newsoftware;
}

const char *GetSelectedSoftware(int driver_index, int device_inst_index)
{
	const char *software;
	options_type *o;

	assert(device_inst_index >= 0);
	assert(device_inst_index < (sizeof(o->mess.software) / sizeof(o->mess.software[0])));

	o = GetGameOptions(driver_index, -1);
	software = o->mess.software[device_inst_index];
	return software ? software : "";
}

void SetExtraSoftwarePaths(int driver_index, const char *extra_paths)
{
	char *new_extra_paths = NULL;

	assert(driver_index >= 0);
	assert(driver_index < num_games);

	if (extra_paths && *extra_paths)
	{
		new_extra_paths = mame_strdup(extra_paths);
		if (!new_extra_paths)
			return;
	}
	FreeIfAllocated(&game_variables[driver_index].mess.extra_software_paths);
	game_variables[driver_index].mess.extra_software_paths = new_extra_paths;
}

const char *GetExtraSoftwarePaths(int driver_index)
{
	const char *paths;

	assert(driver_index >= 0);
	assert(driver_index < num_games);

	paths = game_variables[driver_index].mess.extra_software_paths;
	return paths ? paths : "";
}

void SetCurrentSoftwareTab(const char *shortname)
{
	FreeIfAllocated(&settings.mess.software_tab);
	if (shortname != NULL)
		settings.mess.software_tab = mame_strdup(shortname);
}

const char *GetCurrentSoftwareTab(void)
{
	return settings.mess.software_tab;
}



BOOL LoadDeviceOption(DWORD nSettingsFile, char *key, const char *value_str)
{
	const game_driver *gamedrv;
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	device_class devclass;
	int game_index, dev, count, id, pos;

	// locate the driver
	game_index = nSettingsFile & ~SETTINGS_FILE_TYPEMASK;
	gamedrv = drivers[game_index];

	// retrieve getinfo handlers
	memset(&cfg, 0, sizeof(cfg));
	memset(handlers, 0, sizeof(handlers));
	cfg.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
	cfg.device_handlers = handlers;
	cfg.device_countoverrides = count_overrides;
	if (gamedrv->sysconfig_ctor)
		gamedrv->sysconfig_ctor(&cfg);

	pos = 0;

	for (dev = 0; handlers[dev]; dev++)
	{
		devclass.gamedrv = gamedrv;
		devclass.get_info = handlers[dev];

		count = (int) device_get_info_int(&devclass, DEVINFO_INT_COUNT);

		for (id = 0; id < count; id++)
		{
			if (!mame_stricmp(key, device_instancename(&devclass, id))
				|| !mame_stricmp(key, device_briefinstancename(&devclass, id)))
			{
				FreeIfAllocated(&game_options[game_index].mess.software[pos]);
				game_options[game_index].mess.software[pos] = mame_strdup(value_str);
				return TRUE;
			}
			pos++;
		}
	}
	return FALSE;
}



void SaveDeviceOption(DWORD nSettingsFile, void (*emit_callback)(void *param_, const char *key, const char *value_str, const char *comment), void *param)
{
	const game_driver *gamedrv;
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	device_class devclass;
	int game_index, dev, count, id, pos;
	const char *software;

	// locate the driver
	game_index = nSettingsFile & ~SETTINGS_FILE_TYPEMASK;
	gamedrv = drivers[game_index];

	// retrieve getinfo handlers
	memset(&cfg, 0, sizeof(cfg));
	memset(handlers, 0, sizeof(handlers));
	cfg.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
	cfg.device_handlers = handlers;
	cfg.device_countoverrides = count_overrides;
	if (gamedrv->sysconfig_ctor)
		gamedrv->sysconfig_ctor(&cfg);

	pos = 0;

	for (dev = 0; handlers[dev]; dev++)
	{
		devclass.gamedrv = gamedrv;
		devclass.get_info = handlers[dev];

		count = (int) device_get_info_int(&devclass, DEVINFO_INT_COUNT);

		for (id = 0; id < count; id++)
		{
			software = game_options[game_index].mess.software[pos++];
			if (software)
			{
				emit_callback(param, device_instancename(&devclass, id), software, NULL);
			}
		}
	}
}

