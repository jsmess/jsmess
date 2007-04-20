/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  options.c

  Stores global options and per-game options;

***************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <winreg.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>
#include <direct.h>
#include <driver.h>
#include <stddef.h>

#include "screenshot.h"
#include "bitmask.h"
#include "mame32.h"
#include "m32util.h"
#include "resource.h"
#include "treeview.h"
#include "file.h"
#include "splitters.h"
#include "dijoystick.h"
#include "audit.h"
#include "m32opts.h"
#include "picker.h"
#include "winmain.h"
#include "pool.h"
#include "windows/winutil.h"
#include "windows/strconv.h"

#ifdef MESS
#include "messopts.h"
#include "osd/windows/ui/optionsms.h"
#include "osd/windows/configms.h"
#endif // MESS

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

static void LoadFolderFilter(int folder_index,int filters);

static file_error LoadSettingsFile(core_options *opts, const char *filename);
static file_error SaveSettingsFile(core_options *opts, const char *filename);

static void LoadOptionsAndSettings(void);

static void  KeySeqEncodeString(void *data, char* str);
static void  KeySeqDecodeString(const char *str, void* data);

static void  ColumnDecodeWidths(const char *ptr, void* data);

static void  CusColorEncodeString(const COLORREF *value, char* str);
static void  CusColorDecodeString(const char* str, COLORREF *value);

static void  SplitterEncodeString(const int *value, char* str);
static void  SplitterDecodeString(const char *str, int *value);

static void  ListEncodeString(void* data, char* str);
static void  ListDecodeString(const char* str, void* data);

static void  FontEncodeString(const LOGFONT *f, char *str);
static void  FontDecodeString(const char* str, LOGFONT *f);

static void FolderFlagsEncodeString(void *data,char *str);
static void FolderFlagsDecodeString(const char *str,void *data);

static void TabFlagsEncodeString(int data, char *str);
static void TabFlagsDecodeString(const char *str, int *data);

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/***************************************************************************
    Internal defines
 ***************************************************************************/

#define UI_INI_FILENAME MAME32NAME "ui.ini"
#define DEFAULT_OPTIONS_INI_FILENAME MAME32NAME ".ini"

#define M32OPTION_LIST_MODE						"list_mode"
#define M32OPTION_CHECK_GAME					"check_game"
#define M32OPTION_JOYSTICK_IN_INTERFACE			"joystick_in_interface"
#define M32OPTION_KEYBOARD_IN_INTERFACE			"keyboard_in_interface"
#define M32OPTION_CYCLE_SCREENSHOT				"cycle_screenshot"
#define M32OPTION_STRETCH_SCREENSHOT_LARGER		"stretch_screenshot_larger"
#define M32OPTION_SCREENSHOT_BORDER_SIZE		"screenshot_bordersize"
#define M32OPTION_SCREENSHOT_BORDER_COLOR		"screenshot_bordercolor"
#define M32OPTION_INHERIT_FILTER				"inherit_filter"
#define M32OPTION_OFFSET_CLONES					"offset_clones"
#define M32OPTION_GAME_CAPTION					"game_caption"
#define M32OPTION_BROADCAST_GAME_NAME			"broadcast_game_name"
#define M32OPTION_RANDOM_BACKGROUND				"random_background"
#define M32OPTION_DEFAULT_FOLDER_ID				"default_folder_id"
#define M32OPTION_SHOW_IMAGE_SECTION			"show_image_section"
#define M32OPTION_SHOW_FOLDER_SECTION			"show_folder_section"
#define M32OPTION_HIDE_FOLDERS					"hide_folders"
#define M32OPTION_SHOW_STATUS_BAR				"show_status_bar"
#define M32OPTION_SHOW_TABS						"show_tabs"
#define M32OPTION_SHOW_TOOLBAR					"show_tool_bar"
#define M32OPTION_CURRENT_TAB					"current_tab"
#define M32OPTION_WINDOW_X						"window_x"
#define M32OPTION_WINDOW_Y						"window_y"
#define M32OPTION_WINDOW_WIDTH					"window_width"
#define M32OPTION_WINDOW_HEIGHT					"window_height"
#define M32OPTION_WINDOW_STATE					"window_state"
#define M32OPTION_CUSTOM_COLOR					"custom_color"
#define M32OPTION_LIST_FONT						"list_font"
#define M32OPTION_TEXT_COLOR					"text_color"
#define M32OPTION_CLONE_COLOR					"clone_color"
#define M32OPTION_HIDE_TABS						"hide_tabs"
#define M32OPTION_HISTORY_TAB					"history_tab"
#define M32OPTION_COLUMN_WIDTHS					"column_widths"
#define M32OPTION_COLUMN_ORDER					"column_order"
#define M32OPTION_COLUMN_SHOWN					"column_shown"
#define M32OPTION_SPLITTERS						"splitters"
#define M32OPTION_SORT_COLUMN					"sort_column"
#define M32OPTION_SORT_REVERSED					"sort_reversed"
#define M32OPTION_LANGUAGE						"language"
#define M32OPTION_FLYER_DIRECTORY				"flyer_directory"
#define M32OPTION_CABINET_DIRECTORY				"cabinet_directory"
#define M32OPTION_MARQUEE_DIRECTORY				"marquee_directory"
#define M32OPTION_TITLE_DIRECTORY				"title_directory"
#define M32OPTION_CPANEL_DIRECTORY				"cpanel_directory"
#define M32OPTION_ICONS_DIRECTORY				"icons_directory"
#define M32OPTION_BACKGROUND_DIRECTORY			"background_directory"
#define M32OPTION_FOLDER_DIRECTORY				"folder_directory"

#ifdef MESS
#define M32OPTION_DEFAULT_GAME					"default_system"
#define M32OPTION_HISTORY_FILE					"sysinfo_file"
#define M32OPTION_MAMEINFO_FILE					"messinfo_file"
#else
#define M32OPTION_DEFAULT_GAME					"default_game"
#define M32OPTION_HISTORY_FILE					"history_file"
#define M32OPTION_MAMEINFO_FILE					"mameinfo_file"
#endif



/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	int folder_index;
	int filters;
} folder_filter_type;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static memory_pool *options_memory_pool;

static core_options *settings;

static core_options *global;				// Global 'default' options
static core_options **game_options;			// Array of Game specific options
static core_options **folder_options;		// Array of Folder specific options
static int *folder_options_redirect;		// Array of Folder Ids for redirecting Folder specific options

static BOOL OnlyOnGame(int driver_index)
{
	return driver_index >= 0;
}

// UI options in mame32ui.ini
static const options_entry regSettings[] =
{
	// UI options
	{ NULL,                       NULL,       OPTION_HEADER,     "UI OPTIONS" },
#ifdef MESS
	{ M32OPTION_DEFAULT_GAME,     "nes",      0,                 NULL },
#else
	{ M32OPTION_DEFAULT_GAME,     "puckman",  0,                 NULL },
#endif
	{ "default_folder_id",        "0",        0,                 NULL },
	{ "show_image_section",       "1",        OPTION_BOOLEAN,    NULL },
	{ "current_tab",              "0",        0,                 NULL },
	{ "show_tool_bar",            "1",        OPTION_BOOLEAN,    NULL },
	{ "show_status_bar",          "1",        OPTION_BOOLEAN,    NULL },
#ifdef MESS
	{ "show_folder_section",      "0",        OPTION_BOOLEAN,    NULL },
#else
	{ "show_folder_section",      "1",        OPTION_BOOLEAN,    NULL },
#endif
	{ "hide_folders",             "",         0,                 NULL },

#ifdef MESS
	{ "show_tabs",                "0",        OPTION_BOOLEAN,    NULL },
	{ "hide_tabs",                "flyer, cabinet, marquee, title, cpanel", 0, NULL },
	{ M32OPTION_HISTORY_TAB,      "1",        0,                 NULL },
#else
	{ "show_tabs",                "1",        OPTION_BOOLEAN,    NULL },
	{ "hide_tabs",                "marquee, title, cpanel, history", 0, NULL },
	{ M32OPTION_HISTORY_TAB,      "0",        0,                 NULL },
#endif

	{ "check_game",               "1",        OPTION_BOOLEAN,    NULL },
	{ "joystick_in_interface",    "0",        OPTION_BOOLEAN,    NULL },
	{ "keyboard_in_interface",    "0",        OPTION_BOOLEAN,    NULL },
	{ "broadcast_game_name",      "0",        OPTION_BOOLEAN,    NULL },
	{ "random_background",        "0",        OPTION_BOOLEAN,    NULL },

	{ M32OPTION_SORT_COLUMN,      "0",        0,                 NULL },
	{ M32OPTION_SORT_REVERSED,    "0",        OPTION_BOOLEAN,    NULL },
	{ "window_x",                 "0",        0,                 NULL },
	{ "window_y",                 "0",        0,                 NULL },
	{ "window_width",             "640",      0,                 NULL },
	{ "window_height",            "400",      0,                 NULL },
	{ "window_state",             "1",        0,                 NULL },

	{ "text_color",               "-1",       0,                 NULL },
	{ "clone_color",              "-1",       0,                 NULL },
	{ "custom_color",             "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", 0, NULL },
	/* ListMode needs to be before ColumnWidths settings */
	{ "list_mode",                "5",        0,                 NULL },
#ifdef MESS
	{ M32OPTION_SPLITTERS,        "152,310,468", 0,              NULL },
#else
	{ M32OPTION_SPLITTERS,        "152,362",  0,                 NULL },
#endif
	{ "list_font",                "-8,0,0,0,400,0,0,0,0,0,0,0,0,MS Sans Serif", 0, NULL },
	{ M32OPTION_COLUMN_WIDTHS,    "185,68,84,84,64,88,74,108,60,144,84,60", 0, NULL },
	{ M32OPTION_COLUMN_ORDER,     "0,2,3,4,5,6,7,8,9,1,10,11", 0, NULL },
	{ M32OPTION_COLUMN_SHOWN,     "1,0,1,1,1,1,1,1,1,1,0,0", 0,  NULL },

	{ "ui_key_up",                "KEYCODE_UP", 0,               NULL },
	{ "ui_key_down",              "KEYCODE_DOWN", 0,             NULL },
	{ "ui_key_left",              "KEYCODE_LEFT", 0,             NULL },
	{ "ui_key_right",             "KEYCODE_RIGHT", 0,            NULL },
	{ "ui_key_start",             "KEYCODE_ENTER NOT KEYCODE_LALT", 0, NULL },
	{ "ui_key_pgup",              "KEYCODE_PGUP", 0,             NULL },
	{ "ui_key_pgdwn",             "KEYCODE_PGDN", 0,             NULL },
	{ "ui_key_home",              "KEYCODE_HOME", 0,             NULL },
	{ "ui_key_end",               "KEYCODE_END", 0,              NULL },
	{ "ui_key_ss_change",         "KEYCODE_INSERT", 0,           NULL },
	{ "ui_key_history_up",        "KEYCODE_DEL", 0,              NULL },
	{ "ui_key_history_down",      "KEYCODE_LALT KEYCODE_0", 0,   NULL },

	{ "ui_key_context_filters",   "KEYCODE_LCONTROL KEYCODE_F", 0, NULL },
	{ "ui_key_select_random",     "KEYCODE_LCONTROL KEYCODE_R", 0, NULL },
	{ "ui_key_game_audit",        "KEYCODE_LALT KEYCODE_A",     0, NULL },
	{ "ui_key_game_properties",   "KEYCODE_LALT KEYCODE_ENTER", 0, NULL },
	{ "ui_key_help_contents",     "KEYCODE_F1",                 0, NULL },
	{ "ui_key_update_gamelist",   "KEYCODE_F5",                 0, NULL },
	{ "ui_key_view_folders",      "KEYCODE_LALT KEYCODE_D",     0, NULL },
	{ "ui_key_view_fullscreen",   "KEYCODE_F11",                0, NULL },
	{ "ui_key_view_pagetab",      "KEYCODE_LALT KEYCODE_B",     0, NULL },
	{ "ui_key_view_picture_area", "KEYCODE_LALT KEYCODE_P",     0, NULL },
	{ "ui_key_view_status",       "KEYCODE_LALT KEYCODE_S",     0, NULL },
    { "ui_key_view_toolbars",     "KEYCODE_LALT KEYCODE_T",     0, NULL },

    { "ui_key_view_tab_cabinet",  "KEYCODE_LALT KEYCODE_3",     0, NULL },
    { "ui_key_view_tab_cpanel",   "KEYCODE_LALT KEYCODE_6",     0, NULL },
    { "ui_key_view_tab_flyer",    "KEYCODE_LALT KEYCODE_2",     0, NULL },
    { "ui_key_view_tab_history",  "KEYCODE_LALT KEYCODE_7",     0, NULL },
    { "ui_key_view_tab_marquee",  "KEYCODE_LALT KEYCODE_4",     0, NULL },
    { "ui_key_view_tab_screenshot", "KEYCODE_LALT KEYCODE_1",   0, NULL },
    { "ui_key_view_tab_title",    "KEYCODE_LALT KEYCODE_5",     0, NULL },
    { "ui_key_quit",              "KEYCODE_LALT KEYCODE_Q",     0, NULL },

	{ "ui_joy_up",                "",         0,                 NULL },
	{ "ui_joy_down",              "",         0,                 NULL },
	{ "ui_joy_left",              "",         0,                 NULL },
	{ "ui_joy_right",             "",         0,                 NULL },
	{ "ui_joy_start",             "",         0,                 NULL },
	{ "ui_joy_pgup",              "",         0,                 NULL },
	{ "ui_joy_pgdwn",             "",         0,                 NULL },
	{ "ui_joy_home",              "0,0,0,0",  0,                 NULL },
	{ "ui_joy_end",               "0,0,0,0",  0,                 NULL },
	{ "ui_joy_ss_change",         "",         0,                 NULL },
	{ "ui_joy_history_up",        "",         0,                 NULL },
	{ "ui_joy_history_down",      "",         0,                 NULL },
	{ "ui_joy_exec",              "0,0,0,0",  0,                 NULL },
	{ "exec_command",             "",         0,                 NULL },
	{ "exec_wait",                "0",        0,                 NULL },
	{ "hide_mouse",               "0",        OPTION_BOOLEAN,    NULL },
	{ "full_screen",              "0",        OPTION_BOOLEAN,    NULL },
	{ "cycle_screenshot",         "0",        0,                 NULL },
	{ "stretch_screenshot_larger","0",        OPTION_BOOLEAN,    NULL },
 	{ "screenshot_bordersize",    "11",       0,                 NULL },
 	{ "screenshot_bordercolor",   "-1",       0,                 NULL },
	{ "inherit_filter",           "0",        OPTION_BOOLEAN,    NULL },
	{ "offset_clones",            "0",        OPTION_BOOLEAN,    NULL },
	{ "game_caption",             "1",        OPTION_BOOLEAN,    NULL },

	{ M32OPTION_LANGUAGE,                 "english",  0,                 NULL },
	{ M32OPTION_FLYER_DIRECTORY,          "flyers",   0,                 NULL },
	{ M32OPTION_CABINET_DIRECTORY,        "cabinets", 0,                 NULL },
	{ M32OPTION_MARQUEE_DIRECTORY,        "marquees", 0,                 NULL },
	{ M32OPTION_TITLE_DIRECTORY,          "titles",   0,                 NULL },
	{ M32OPTION_CPANEL_DIRECTORY,         "cpanel",   0,                 NULL },
	{ M32OPTION_BACKGROUND_DIRECTORY,     "bkground", 0,                 NULL },
	{ M32OPTION_FOLDER_DIRECTORY,         "folders",  0,                 NULL },
	{ M32OPTION_ICONS_DIRECTORY,          "icons",    0,                 NULL },
#ifdef MESS
	{ M32OPTION_HISTORY_FILE,             "sysinfo.dat", 0,              NULL },
	{ M32OPTION_MAMEINFO_FILE,            "messinfo.dat", 0,             NULL },
#else
	{ M32OPTION_HISTORY_FILE,             "history.dat", 0,              NULL },
	{ M32OPTION_MAMEINFO_FILE,            "mameinfo.dat", 0,             NULL },
#endif
	{ NULL }
};

static const options_entry perGameOptions[] =
{
	// per game options
	{ "_play_count",             "0",        0,                 NULL },
	{ "_play_time",              "0",        0,                 NULL },
	{ "_rom_audit",              "-1",       0,                 NULL },
	{ "_samples_audit",          "-1",       0,                 NULL },
#ifdef MESS
	{ "_extra_software",         "",         0,                 NULL },
#endif

	// filters
	{ "_filters",                "0",        0,                 NULL },

	{ NULL }
};



// Screen shot Page tab control text
// these must match the order of the options flags in options.h
// (TAB_...)
const char* image_tabs_long_name[MAX_TAB_TYPES] =
{
	"Snapshot ",
	"Flyer ",
	"Cabinet ",
	"Marquee",
	"Title",
	"Control Panel",
	"History ",
};

const char* image_tabs_short_name[MAX_TAB_TYPES] =
{
	"snapshot",
	"flyer",
	"cabinet",
	"marquee",
	"title",
	"cpanel",
	"history"
};


static BOOL save_gui_settings = TRUE;
static BOOL save_default_options = TRUE;

static const char *view_modes[VIEW_MAX] = { "Large Icons", "Small Icons", "List", "Details", "Grouped" };

folder_filter_type *folder_filters;
int size_folder_filters;
int num_folder_filters;

/***************************************************************************
    External functions  
 ***************************************************************************/

static void memory_error(const char *message)
{
	win_message_box_utf8(NULL, message, NULL, MB_OK);
	exit(-1);
}



static void AddOptions(core_options *opts, const options_entry *entrylist, BOOL is_global)
{
	static const char *blacklist[] =
	{
		WINOPTION_SCREEN,
		WINOPTION_ASPECT,
		WINOPTION_RESOLUTION,
		WINOPTION_VIEW
	};
	options_entry entries[2];
	BOOL good_option;
	int i;

	for ( ; entrylist->name != NULL || (entrylist->flags & OPTION_HEADER); entrylist++)
	{
		good_option = TRUE;

		// check blacklist
		if (entrylist->name != NULL)
		{
			for (i = 0; i < ARRAY_LENGTH(blacklist); i++)
			{
				if (!strcmp(entrylist->name, blacklist[i]))
				{
					good_option = FALSE;
					break;
				}
			}
		}

		// if is_global is FALSE, blacklist global options
		if (entrylist->name && !is_global && IsGlobalOption(entrylist->name))
			good_option = FALSE;

		if (good_option)
		{
			memcpy(entries, entrylist, sizeof(options_entry));
			memset(&entries[1], 0, sizeof(entries[1]));
			options_add_entries(opts, entries);
		}
	}
}



core_options *CreateGameOptions(BOOL is_global)
{
	core_options *opts;
	extern const options_entry mame_win_options[];

	// create the options
	opts = options_create(memory_error);

	// add the options
	AddOptions(opts, mame_core_options, is_global);
	AddOptions(opts, mame_win_options, is_global);
#ifdef MESS
	AddOptions(opts, mess_core_options, is_global);
	AddOptions(opts, mess_win_options, is_global);
#endif // MESS

	// customize certain options
	if (is_global)
		options_set_option_default_value(opts, OPTION_INIPATH, "ini");

	return opts;
}



BOOL OptionsInit()
{
	extern const options_entry mame_win_options[];

	// initialize core options
	mame_options_init(mame_win_options);

	// create a memory pool for our data
	options_memory_pool = pool_create(memory_error);
	if (!options_memory_pool)
		return FALSE;

	// set up the MAME32 settings (these get placed in MAME32ui.ini
	settings = options_create(memory_error);
	options_add_entries(settings, regSettings);
#ifdef MESS
	options_add_entries(settings, mess_wingui_settings);
#endif

	// set up per game options
	{
		char buffer[128];
		int i, j;
		int game_option_count = 0;
		options_entry *driver_per_game_options;
		options_entry *ent;

		while(perGameOptions[game_option_count].name)
			game_option_count++;

		driver_per_game_options = (options_entry *) pool_malloc(options_memory_pool,
			(game_option_count * driver_get_count() + 1) * sizeof(*driver_per_game_options));

		for (i = 0; i < driver_get_count(); i++)
		{
			for (j = 0; j < game_option_count; j++)
			{
				ent = &driver_per_game_options[i * game_option_count + j];

				snprintf(buffer, ARRAY_LENGTH(buffer), "%s%s", drivers[i]->name, perGameOptions[j].name);

				memset(ent, 0, sizeof(*ent));
				ent->name = pool_strdup(options_memory_pool, buffer);
				ent->defvalue = perGameOptions[j].defvalue;
				ent->flags = perGameOptions[j].flags;
				ent->description = perGameOptions[j].description;
			}
		}
		ent = &driver_per_game_options[driver_get_count() * game_option_count];
		memset(ent, 0, sizeof(*ent));
		options_add_entries(settings, driver_per_game_options);
	}

	// set up global options
	global = CreateGameOptions(TRUE);

	// set up game options
	game_options = (core_options **) pool_malloc(options_memory_pool, driver_get_count() * sizeof(*game_options));
	memset(game_options, 0, driver_get_count() * sizeof(*game_options));

	// set up folders
	size_folder_filters = 1;
	num_folder_filters = 0;
	folder_filters = (folder_filter_type *) pool_malloc(options_memory_pool, size_folder_filters * sizeof(*folder_filters));

	// now load the options and settings
	LoadOptionsAndSettings();

	// have our MAME core (file code) know about our rom path
	options_set_string(mame_options(), SEARCHPATH_ROM, GetRomDirs());
	options_set_string(mame_options(), SEARCHPATH_SAMPLE, GetSampleDirs());
#ifdef MESS
	options_set_string(mame_options(), SEARCHPATH_HASH, GetHashDirs());
#endif // MESS

	return TRUE;

}

void FolderOptionsInit()
{
	//Get Number of Folder Entries with options from folder_options_redirect
	int i=GetNumOptionFolders();

	folder_options = (core_options **) pool_malloc(options_memory_pool, i * sizeof(*folder_options));
	memset(folder_options, 0, i * sizeof(*folder_options));
}

void OptionsExit(void)
{
	int i;

	// free all game specific options
	for (i = 0; i < driver_get_count(); i++)
	{
		if (game_options[i] != NULL)
		{
			options_free(game_options[i]);
			game_options[i] = NULL;
		}
	}

	// free all folder specific options
	for (i = 0; i < GetNumOptionFolders(); i++)
	{
		if (folder_options[i] != NULL)
		{
			options_free(folder_options[i]);
			folder_options[i] = NULL;
		}
	}

	// free global options
	options_free(global);
	global = NULL;

	// free settings
	options_free(settings);
	settings = NULL;

	// free the memory pool
	pool_free(options_memory_pool);
	options_memory_pool = NULL;
}



int GetRedirectIndex(int folder_index)
{
	int i=0;
	int *redirect_arr = GetFolderOptionsRedirectArr();
	//Use the redirect array, to get correct folder
	for( i=0; i<GetNumOptionFolders();i++)
	{
		
		if( redirect_arr[i] == folder_index)
		{
			//found correct folderId
			return i;
		}
	}
	return -1;
}

int GetRedirectValue(int index)
{
	int *redirect_arr = GetFolderOptionsRedirectArr();
	//Use the redirect array, to get correct folder
	if( GetNumOptionFolders() < index )
		return redirect_arr[index];
	else
		return -1;
}



static void GetDriverSettingsFileName(int driver_index, char *filename, size_t filename_len)
{
	char title[32];
	char *s;

	snprintf(title, ARRAY_LENGTH(title), "%s", GetDriverFilename(driver_index));
	s = strrchr(title, '.');
	if (s)
		*s = '\0';

	snprintf(filename, filename_len, "%s\\drivers\\%s.ini", GetIniDir(), title);
}



static BOOL GetFolderSettingsFileName(int folder_index, char *filename, size_t filename_len)
{
	extern FOLDERDATA g_folderData[];
	extern LPEXFOLDERDATA ExtraFolderData[];
	extern int numExtraFolders;
	int i, j;
	const char *pParent;
	BOOL result = FALSE;

	filename[0] = '\0';

	if (folder_index < MAX_FOLDERS)
	{
		for (i = 0; g_folderData[i].m_lpTitle; i++)
		{
			if (g_folderData[i].m_nFolderId == folder_index)
			{
				snprintf(filename, filename_len, "%s\\%s.ini", GetIniDir(), g_folderData[i].m_lpTitle);
				result = TRUE;
				break;
			}
		}
	}
	else if (folder_index < MAX_FOLDERS + numExtraFolders)
	{
		for (i = 0; i < MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS; i++)
		{
			if (ExtraFolderData[i])
			{
				snprintf(filename, filename_len, "%s\\%s.ini", GetIniDir(), ExtraFolderData[i]->m_szTitle);
				result = TRUE;
				break;
			}
		}
	}
	else
	{
		//we jump to the corresponding folderData
		if( ExtraFolderData[folder_index] )
		{
			//SubDirName is ParentFolderName
			pParent = GetFolderNameByID(ExtraFolderData[folder_index]->m_nParent);
			if( pParent )
			{
				if( ExtraFolderData[folder_index]->m_nParent == FOLDER_SOURCE )
				{
					for (j = 0; drivers[j]; j++)
					{
						if (!strcmp(GetDriverFilename(j), ExtraFolderData[folder_index]->m_szTitle))
						{
							GetDriverSettingsFileName(j, filename, filename_len);
							result = TRUE;
							break;
						}
					}
				}
				else
				{
					pParent = GetFolderNameByID(ExtraFolderData[folder_index]->m_nParent);
					snprintf(filename, filename_len, "%s\\%s\\%s.ini", GetIniDir(), pParent, ExtraFolderData[folder_index]->m_szTitle );
					result = TRUE;
				}
			}
		}
	}
	return result;
}

// sync in the options specified in filename
void SyncInFolderOptions(core_options *opts, int folder_index)
{
	char filename[MAX_PATH];

	if (GetFolderSettingsFileName(folder_index, filename, ARRAY_LENGTH(filename)))
	{
		LoadSettingsFile(opts, filename);
	}
}


core_options * GetDefaultOptions(int iProperty, BOOL bVectorFolder )
{
	if( iProperty == GLOBAL_OPTIONS)
		return global;
	else if( iProperty == FOLDER_OPTIONS)
	{
		if (bVectorFolder)
			return global;
		else
			return GetVectorOptions();
	}
	else
	{
		assert(0 <= iProperty && iProperty < driver_get_count());
		return GetSourceOptions( iProperty );
	}
}

core_options * GetFolderOptions(int folder_index, BOOL bIsVector)
{
	int redirect_index=0;
	if( folder_index >= (MAX_FOLDERS + (MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS) ) )
	{
		dprintf("Folder Index out of bounds");
		return NULL;
	}
	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(folder_index);
	if( redirect_index < 0)
		return NULL;
	options_copy(folder_options[redirect_index], global);
	LoadFolderOptions(folder_index);
	if( bIsVector)
	{
		SyncInFolderOptions(folder_options[redirect_index], FOLDER_VECTOR);
	}
	return folder_options[redirect_index];
}

int * GetFolderOptionsRedirectArr(void)
{
	return &folder_options_redirect[0];
}

void SetFolderOptionsRedirectArr(int *redirect_arr)
{
	folder_options_redirect = redirect_arr;
}


core_options * GetVectorOptions(void)
{
	static core_options *vector_ops;

	//initialize vector_ops with global settings, we accumulate all settings there and copy them 
	//to game_options[driver_index] in the end
	options_copy(vector_ops, global);
	
	//If it is a Vector game sync in the Vector.ini settings
	SyncInFolderOptions(vector_ops, FOLDER_VECTOR);

	return vector_ops;
}

core_options * GetSourceOptions(int driver_index )
{
	static core_options *source_opts;
	char filename[MAX_PATH];

	assert(0 <= driver_index && driver_index < driver_get_count());

	if (source_opts != NULL)
	{
		options_free(source_opts);
		source_opts = NULL;
	}

	source_opts = CreateGameOptions(FALSE);

	//initialize source_opts with global settings, we accumulate all settings there and copy them 
	//to game_options[driver_index] in the end
	options_copy(source_opts, global);
	
	if( DriverIsVector(driver_index) )
	{
		//If it is a Vector game sync in the Vector.ini settings
		SyncInFolderOptions(source_opts, FOLDER_VECTOR);
	}

	//If it has source folder settings sync in the source\sourcefile.ini settings
	GetDriverSettingsFileName(driver_index, filename, ARRAY_LENGTH(filename));
	LoadSettingsFile(source_opts, filename);

	return source_opts;
}

core_options * GetGameOptions(int driver_index, int folder_index )
{
	int parent_index;
	char filename[MAX_PATH];

	assert(0 <= driver_index && driver_index < driver_get_count());

	// do we have to load these options?
	if (game_options[driver_index] == NULL)
	{
		game_options[driver_index] = CreateGameOptions(FALSE);
		options_copy(game_options[driver_index], global);
	
		if (DriverIsVector(driver_index))
		{
			//If it is a Vector game sync in the Vector.ini settings
			SyncInFolderOptions(game_options[driver_index], FOLDER_VECTOR);
		}
	
		//If it has source folder settings sync in the source\sourcefile.ini settings
		GetDriverSettingsFileName(driver_index, filename, ARRAY_LENGTH(filename));
		LoadSettingsFile(game_options[driver_index], filename);
	
		//Sync in parent settings if it has one
		if( DriverIsClone(driver_index))
		{
			parent_index = GetParentIndex(drivers[driver_index]);
			snprintf(filename, ARRAY_LENGTH(filename), "%s\\%s.ini", GetIniDir(), drivers[parent_index]->name);
			LoadSettingsFile(game_options[driver_index], filename);
		}

		// last but not least, sync in game specific settings
		snprintf(filename, ARRAY_LENGTH(filename), "%s\\%s.ini", GetIniDir(), drivers[driver_index]->name);
		LoadSettingsFile(game_options[driver_index], filename);
	}

	// return the result
	return game_options[driver_index];
}

core_options * Mame32Settings(void)
{
	return settings;
}

core_options * Mame32Global(void)
{
	return global;
}

void SetGameUsesDefaults(int driver_index,BOOL use_defaults)
{
	assert(0 <= driver_index < driver_get_count());

	if (use_defaults && (game_options[driver_index] != NULL))
	{
		options_free(game_options[driver_index]);
		game_options[driver_index] = NULL;
	}
}

void LoadFolderFilter(int folder_index,int filters)
{
	//dprintf("loaded folder filter %i %i",folder_index,filters);

	if (num_folder_filters == size_folder_filters)
	{
		size_folder_filters *= 2;
		folder_filters = (folder_filter_type *)realloc(
			folder_filters,size_folder_filters * sizeof(folder_filter_type));
	}
	folder_filters[num_folder_filters].folder_index = folder_index;
	folder_filters[num_folder_filters].filters = filters;

	num_folder_filters++;
}

void ResetGUI(void)
{
	save_gui_settings = FALSE;
}

const char * GetImageTabLongName(int tab_index)
{
	assert(0 <= tab_index < ARRAY_LENGTH(image_tabs_long_name));
	return image_tabs_long_name[tab_index];
}

const char * GetImageTabShortName(int tab_index)
{
	assert(0 <= tab_index < ARRAY_LENGTH(image_tabs_short_name));
	return image_tabs_short_name[tab_index];
}

//============================================================
//  OPTIONS WRAPPERS
//============================================================

static COLORREF options_get_color(core_options *opts, const char *name)
{
	const char *value_str;
	unsigned int r, g, b;
	COLORREF value;

	value_str = options_get_string(opts, name);

	if (sscanf(value_str, "%u,%u,%u", &r, &g, &b) == 3)
		value = RGB(r,g,b);
	else
		value = (COLORREF) - 1;
	return value;
}

static void options_set_color(core_options *opts, const char *name, COLORREF value)
{
	char value_str[32];

	if (value == (COLORREF) -1)
	{
		snprintf(value_str, ARRAY_LENGTH(value_str), "%d", (int) value);
	}
	else
	{
		snprintf(value_str, ARRAY_LENGTH(value_str), "%d,%d,%d",
			(((int) value) >>  0) & 0xFF,
			(((int) value) >>  8) & 0xFF,
			(((int) value) >> 16) & 0xFF);
	}
	options_set_string(opts, name, value_str);
}

static COLORREF options_get_color_default(core_options *opts, const char *name, int default_color)
{
	COLORREF value = options_get_color(opts, name);
	if (value == (COLORREF) -1)
		value = GetSysColor(default_color);
	return value;
}

static void options_set_color_default(core_options *opts, const char *name, COLORREF value, int default_color)
{
	if (value == GetSysColor(default_color))
		options_set_color(settings, name, (COLORREF) -1);
	else
		options_set_color(settings, name, value);
}

static input_seq *options_get_input_seq(core_options *opts, const char *name)
{
	static input_seq seq;
	const char *seq_string;

	seq_string = options_get_string(opts, name);
	string_to_seq(seq_string, &seq);
	return &seq;
}

static void options_set_input_seq(core_options *opts, const char *name, const input_seq *seq)
{
	char buffer[1024];
	seq_to_string(seq, buffer, ARRAY_LENGTH(buffer));
	options_set_string(opts, name, buffer);
}



//============================================================
//  OPTIONS CALLS
//============================================================

void SetViewMode(int val)
{
	options_set_int(settings, M32OPTION_LIST_MODE, val);
}

int GetViewMode(void)
{
	return options_get_int(settings, M32OPTION_LIST_MODE);
}

void SetGameCheck(BOOL game_check)
{
	options_set_bool(settings, M32OPTION_CHECK_GAME, game_check);
}

BOOL GetGameCheck(void)
{
	return options_get_bool(settings,M32OPTION_CHECK_GAME);
}

void SetJoyGUI(BOOL use_joygui)
{
	options_set_bool(settings, M32OPTION_JOYSTICK_IN_INTERFACE, use_joygui);
}

BOOL GetJoyGUI(void)
{
	return options_get_bool(settings, M32OPTION_JOYSTICK_IN_INTERFACE);
}

void SetKeyGUI(BOOL use_keygui)
{
	options_set_bool(settings, M32OPTION_KEYBOARD_IN_INTERFACE, use_keygui);
}

BOOL GetKeyGUI(void)
{
	return options_get_bool(settings, M32OPTION_KEYBOARD_IN_INTERFACE);
}

void SetCycleScreenshot(int cycle_screenshot)
{
	options_set_int(settings, M32OPTION_CYCLE_SCREENSHOT, cycle_screenshot);
}

int GetCycleScreenshot(void)
{
	return options_get_int(settings, M32OPTION_CYCLE_SCREENSHOT);
}

void SetStretchScreenShotLarger(BOOL stretch)
{
	options_set_bool(settings, M32OPTION_STRETCH_SCREENSHOT_LARGER, stretch);
}

BOOL GetStretchScreenShotLarger(void)
{
	return options_get_bool(settings, M32OPTION_STRETCH_SCREENSHOT_LARGER);
}

void SetScreenshotBorderSize(int size)
{
	options_set_int(settings, M32OPTION_SCREENSHOT_BORDER_SIZE, size);
}

int GetScreenshotBorderSize(void)
{
	return options_get_int(settings, M32OPTION_SCREENSHOT_BORDER_SIZE);
}

void SetScreenshotBorderColor(COLORREF uColor)
{
	options_set_color_default(settings, M32OPTION_SCREENSHOT_BORDER_COLOR, uColor, COLOR_3DFACE);
}

COLORREF GetScreenshotBorderColor(void)
{
	return options_get_color_default(settings, M32OPTION_SCREENSHOT_BORDER_COLOR, COLOR_3DFACE);
}

void SetFilterInherit(BOOL inherit)
{
	options_set_bool(settings, M32OPTION_INHERIT_FILTER, inherit);
}

BOOL GetFilterInherit(void)
{
	return options_get_bool(settings, M32OPTION_INHERIT_FILTER);
}

void SetOffsetClones(BOOL offset)
{
	options_set_bool(settings, M32OPTION_OFFSET_CLONES, offset);
}

BOOL GetOffsetClones(void)
{
	return options_get_bool(settings, M32OPTION_OFFSET_CLONES);
}

void SetGameCaption(BOOL caption)
{
	options_set_bool(settings, M32OPTION_GAME_CAPTION, caption);
}

BOOL GetGameCaption(void)
{
	return options_get_bool(settings, M32OPTION_GAME_CAPTION);
}

void SetBroadcast(BOOL broadcast)
{
	options_set_bool(settings, M32OPTION_BROADCAST_GAME_NAME, broadcast);
}

BOOL GetBroadcast(void)
{
	return options_get_bool(settings, M32OPTION_BROADCAST_GAME_NAME);
}

void SetRandomBackground(BOOL random_bg)
{
	options_set_bool(settings, M32OPTION_RANDOM_BACKGROUND, random_bg);
}

BOOL GetRandomBackground(void)
{
	return options_get_bool(settings, M32OPTION_RANDOM_BACKGROUND);
}

void SetSavedFolderID(UINT val)
{
	options_set_int(settings, M32OPTION_DEFAULT_FOLDER_ID, (int) val);
}

UINT GetSavedFolderID(void)
{
	return (UINT) options_get_int(settings, M32OPTION_DEFAULT_FOLDER_ID);
}

void SetShowScreenShot(BOOL val)
{
	options_set_bool(settings, M32OPTION_SHOW_IMAGE_SECTION, val);
}

BOOL GetShowScreenShot(void)
{
	return options_get_bool(settings, M32OPTION_SHOW_IMAGE_SECTION);
}

void SetShowFolderList(BOOL val)
{
	options_set_bool(settings, M32OPTION_SHOW_FOLDER_SECTION, val);
}

BOOL GetShowFolderList(void)
{
	return options_get_bool(settings, M32OPTION_SHOW_FOLDER_SECTION);
}

static void GetsShowFolderFlags(LPBITS bits)
{
	char s[2000];
	extern FOLDERDATA g_folderData[];
	char *token;

	snprintf(s, ARRAY_LENGTH(s), "%s", options_get_string(settings, M32OPTION_HIDE_FOLDERS));

	SetAllBits(bits, TRUE);

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;g_folderData[j].m_lpTitle != NULL;j++)
		{
			if (strcmp(g_folderData[j].short_name,token) == 0)
			{
				ClearBit(bits, g_folderData[j].m_nFolderId);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}
}

BOOL GetShowFolder(int folder)
{
	BOOL result;
	LPBITS show_folder_flags = NewBits(MAX_FOLDERS);
	GetsShowFolderFlags(show_folder_flags);
	result = TestBit(show_folder_flags, folder);
	DeleteBits(show_folder_flags);
	return result;
}

void SetShowFolder(int folder,BOOL show)
{
	LPBITS show_folder_flags = NewBits(MAX_FOLDERS);
	int i;
	int num_saved = 0;
	char str[10000];
	extern FOLDERDATA g_folderData[];

	GetsShowFolderFlags(show_folder_flags);

	if (show)
		SetBit(show_folder_flags, folder);
	else
		ClearBit(show_folder_flags, folder);

	strcpy(str, "");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_FOLDERS;i++)
	{
		if (TestBit(show_folder_flags, i) == FALSE)
		{
			int j;

			if (num_saved != 0)
				strcat(str,", ");

			for (j=0;g_folderData[j].m_lpTitle != NULL;j++)
			{
				if (g_folderData[j].m_nFolderId == i)
				{
					strcat(str,g_folderData[j].short_name);
					num_saved++;
					break;
				}
			}
		}
	}
	options_set_string(settings, M32OPTION_HIDE_FOLDERS, str);
	DeleteBits(show_folder_flags);
}

void SetShowStatusBar(BOOL val)
{
	options_set_bool(settings, M32OPTION_SHOW_STATUS_BAR, val);
}

BOOL GetShowStatusBar(void)
{
	return options_get_bool(settings, M32OPTION_SHOW_STATUS_BAR);
}

void SetShowTabCtrl (BOOL val)
{
	options_set_bool(settings, M32OPTION_SHOW_TABS, val);
}

BOOL GetShowTabCtrl (void)
{
	return options_get_bool(settings, M32OPTION_SHOW_TABS);
}

void SetShowToolBar(BOOL val)
{
	options_set_bool(settings, M32OPTION_SHOW_TOOLBAR, val);
}

BOOL GetShowToolBar(void)
{
	return options_get_bool(settings, M32OPTION_SHOW_TOOLBAR);
}

void SetCurrentTab(const char *shortname)
{
	options_set_string(settings, M32OPTION_CURRENT_TAB, shortname);
}

const char *GetCurrentTab(void)
{
	return options_get_string(settings, M32OPTION_CURRENT_TAB);
}

void SetDefaultGame(const char *name)
{
	options_set_string(settings, M32OPTION_DEFAULT_GAME, name);
}

const char *GetDefaultGame(void)
{
	return options_get_string(settings, M32OPTION_DEFAULT_GAME);
}

void SetWindowArea(const AREA *area)
{
	options_set_int(settings, M32OPTION_WINDOW_X,		area->x);
	options_set_int(settings, M32OPTION_WINDOW_Y,		area->y);
	options_set_int(settings, M32OPTION_WINDOW_WIDTH,	area->width);
	options_set_int(settings, "window_height",	area->height);
}

void GetWindowArea(AREA *area)
{
	area->x      = options_get_int(settings, M32OPTION_WINDOW_X);
	area->y      = options_get_int(settings, M32OPTION_WINDOW_Y);
	area->width  = options_get_int(settings, M32OPTION_WINDOW_WIDTH);
	area->height = options_get_int(settings, M32OPTION_WINDOW_HEIGHT);
}

void SetWindowState(UINT state)
{
	options_set_int(settings, M32OPTION_WINDOW_STATE, state);
}

UINT GetWindowState(void)
{
	return options_get_int(settings, M32OPTION_WINDOW_STATE);
}

void SetCustomColor(int iIndex, COLORREF uColor)
{
	const char *custom_color_string;
	COLORREF custom_color[256];
	char buffer[10000];

	custom_color_string = options_get_string(settings, M32OPTION_CUSTOM_COLOR);
	CusColorDecodeString(custom_color_string, custom_color);

	custom_color[iIndex] = uColor;

	CusColorEncodeString(custom_color, buffer);
	options_set_string(settings, M32OPTION_CUSTOM_COLOR, buffer);
}

COLORREF GetCustomColor(int iIndex)
{
	const char *custom_color_string;
	COLORREF custom_color[256];

	custom_color_string = options_get_string(settings, M32OPTION_CUSTOM_COLOR);
	CusColorDecodeString(custom_color_string, custom_color);

	if (custom_color[iIndex] == (COLORREF)-1)
		return (COLORREF)RGB(0,0,0);

	return custom_color[iIndex];
}

void SetListFont(const LOGFONT *font)
{
	char font_string[10000];
	FontEncodeString(font, font_string);
	options_set_string(settings, M32OPTION_LIST_FONT, font_string);
}

void GetListFont(LOGFONT *font)
{
	const char *font_string = options_get_string(settings, M32OPTION_LIST_FONT);
	FontDecodeString(font_string, font);
}

void SetListFontColor(COLORREF uColor)
{
	options_set_color_default(settings, M32OPTION_TEXT_COLOR, uColor, COLOR_WINDOWTEXT);
}

COLORREF GetListFontColor(void)
{
	return options_get_color_default(settings, M32OPTION_TEXT_COLOR, COLOR_WINDOWTEXT);
}

void SetListCloneColor(COLORREF uColor)
{
	options_set_color_default(settings, M32OPTION_CLONE_COLOR, uColor, COLOR_WINDOWTEXT);
}

COLORREF GetListCloneColor(void)
{
	return options_get_color_default(settings, M32OPTION_CLONE_COLOR, COLOR_WINDOWTEXT);
}

int GetShowTab(int tab)
{
	const char *show_tabs_string;
	int show_tab_flags;
	
	show_tabs_string = options_get_string(settings, M32OPTION_HIDE_TABS);
	TabFlagsDecodeString(show_tabs_string, &show_tab_flags);
	return (show_tab_flags & (1 << tab)) != 0;
}

void SetShowTab(int tab,BOOL show)
{
	const char *show_tabs_string;
	int show_tab_flags;
	char buffer[10000];
	
	show_tabs_string = options_get_string(settings, M32OPTION_HIDE_TABS);
	TabFlagsDecodeString(show_tabs_string, &show_tab_flags);

	if (show)
		show_tab_flags |= 1 << tab;
	else
		show_tab_flags &= ~(1 << tab);

	TabFlagsEncodeString(show_tab_flags, buffer);
	options_set_string(settings, M32OPTION_HIDE_TABS, buffer);
}

// don't delete the last one
BOOL AllowedToSetShowTab(int tab,BOOL show)
{
	const char *show_tabs_string;
	int show_tab_flags;

	if (show == TRUE)
		return TRUE;

	show_tabs_string = options_get_string(settings, M32OPTION_HIDE_TABS);
	TabFlagsDecodeString(show_tabs_string, &show_tab_flags);

	show_tab_flags &= ~(1 << tab);
	return show_tab_flags != 0;
}

int GetHistoryTab(void)
{
	return options_get_int(settings, M32OPTION_HISTORY_TAB);
}

void SetHistoryTab(int tab, BOOL show)
{
	if (show)
		options_set_int(settings, M32OPTION_HISTORY_TAB, tab);
	else
		options_set_int(settings, M32OPTION_HISTORY_TAB, TAB_NONE);
}

void SetColumnWidths(int width[])
{
	char column_width_string[10000];
	ColumnEncodeStringWithCount(width, column_width_string, COLUMN_MAX);
	options_set_string(settings, M32OPTION_COLUMN_WIDTHS, column_width_string);
}

void GetColumnWidths(int width[])
{
	const char *column_width_string;
	column_width_string = options_get_string(settings, M32OPTION_COLUMN_WIDTHS);
	ColumnDecodeStringWithCount(column_width_string, width, COLUMN_MAX);
}

void SetSplitterPos(int splitterId, int pos)
{
	const char *splitter_string;
	int *splitter;
	char buffer[10000];

	if (splitterId < GetSplitterCount())
	{
		splitter_string = options_get_string(settings, M32OPTION_SPLITTERS);
		splitter = (int *) alloca(GetSplitterCount() * sizeof(*splitter));
		SplitterDecodeString(splitter_string, splitter);

		splitter[splitterId] = pos;

		SplitterEncodeString(splitter, buffer);
		options_set_string(settings, M32OPTION_SPLITTERS, buffer);
	}
}

int  GetSplitterPos(int splitterId)
{
	const char *splitter_string;
	int *splitter;

	splitter_string = options_get_string(settings, M32OPTION_SPLITTERS);
	splitter = (int *) alloca(GetSplitterCount() * sizeof(*splitter));
	SplitterDecodeString(splitter_string, splitter);

	if (splitterId < GetSplitterCount())
		return splitter[splitterId];

	return -1; /* Error */
}

void SetColumnOrder(int order[])
{
	char column_order_string[10000];
	ColumnEncodeStringWithCount(order, column_order_string, COLUMN_MAX);
	options_set_string(settings, M32OPTION_COLUMN_ORDER, column_order_string);
}

void GetColumnOrder(int order[])
{
	const char *column_order_string;
	column_order_string = options_get_string(settings, M32OPTION_COLUMN_ORDER);
	ColumnDecodeStringWithCount(column_order_string, order, COLUMN_MAX);
}

void SetColumnShown(int shown[])
{
	char column_shown_string[10000];
	ColumnEncodeStringWithCount(shown, column_shown_string, COLUMN_MAX);
	options_set_string(settings, M32OPTION_COLUMN_SHOWN, column_shown_string);
}

void GetColumnShown(int shown[])
{
	const char *column_shown_string;
	column_shown_string = options_get_string(settings, M32OPTION_COLUMN_SHOWN);
	ColumnDecodeStringWithCount(column_shown_string, shown, COLUMN_MAX);
}

void SetSortColumn(int column)
{
	options_set_int(settings, M32OPTION_SORT_COLUMN, column);
}

int GetSortColumn(void)
{
	return options_get_int(settings, M32OPTION_SORT_COLUMN);
}

void SetSortReverse(BOOL reverse)
{
	options_set_bool(settings, M32OPTION_SORT_REVERSED, reverse);
}

BOOL GetSortReverse(void)
{
	return options_get_bool(settings, M32OPTION_SORT_REVERSED);
}

const char* GetLanguage(void)
{
	return options_get_string(settings, M32OPTION_LANGUAGE);
}

void SetLanguage(const char* lang)
{
	options_set_string(settings, M32OPTION_LANGUAGE, lang);
}

const char* GetRomDirs(void)
{
	return options_get_string(global, OPTION_ROMPATH);
}

void SetRomDirs(const char* paths)
{
	options_set_string(global, OPTION_ROMPATH, paths);
	options_set_string(mame_options(), OPTION_ROMPATH, paths);
}

const char* GetSampleDirs(void)
{
	return options_get_string(global, OPTION_SAMPLEPATH);
}

void SetSampleDirs(const char* paths)
{
	options_set_string(global, OPTION_SAMPLEPATH, paths);
	options_set_string(mame_options(), OPTION_SAMPLEPATH, paths);
}

const char * GetIniDir(void)
{
	const char *ini_dir;
	const char *s;
	
	ini_dir = options_get_string(global, OPTION_INIPATH);
	while((s = strchr(ini_dir, ';')) != NULL)
	{
		ini_dir = s + 1;
	}
	return ini_dir;

}

void SetIniDir(const char *path)
{
	options_set_string(global, OPTION_INIPATH, path);
}

const char* GetCtrlrDir(void)
{
	return options_get_string(global, OPTION_CTRLRPATH);
}

void SetCtrlrDir(const char* path)
{
	options_set_string(global, OPTION_CTRLRPATH, path);
}

const char* GetCommentDir(void)
{
	return options_get_string(global, OPTION_COMMENT_DIRECTORY);
}

void SetCommentDir(const char* path)
{
	options_set_string(global, OPTION_COMMENT_DIRECTORY, path);
}

const char* GetCfgDir(void)
{
	return options_get_string(global, OPTION_CFG_DIRECTORY);
}

void SetCfgDir(const char* path)
{
	options_set_string(global, OPTION_CFG_DIRECTORY, path);
}

const char* GetNvramDir(void)
{
	return options_get_string(global, OPTION_NVRAM_DIRECTORY);
}

void SetNvramDir(const char* path)
{
	options_set_string(global, OPTION_NVRAM_DIRECTORY, path);
}

const char* GetInpDir(void)
{
	return options_get_string(global, OPTION_INPUT_DIRECTORY);
}

void SetInpDir(const char* path)
{
	options_set_string(global, OPTION_INPUT_DIRECTORY, path);
}

const char* GetImgDir(void)
{
	return options_get_string(global, OPTION_SNAPSHOT_DIRECTORY);
}

void SetImgDir(const char* path)
{
	options_set_string(global, OPTION_SNAPSHOT_DIRECTORY, path);
}

const char* GetStateDir(void)
{
	return options_get_string(global, OPTION_STATE_DIRECTORY);
}

void SetStateDir(const char* path)
{
	options_set_string(global, OPTION_STATE_DIRECTORY, path);
}

const char* GetArtDir(void)
{
	return options_get_string(global, OPTION_ARTPATH);
}

void SetArtDir(const char* path)
{
	options_set_string(global, OPTION_ARTPATH, path);
}

const char* GetMemcardDir(void)
{
	return options_get_string(global, OPTION_MEMCARD_DIRECTORY);
}

void SetMemcardDir(const char* path)
{
	options_set_string(global, OPTION_MEMCARD_DIRECTORY, path);
}

const char* GetFlyerDir(void)
{
	return options_get_string(settings, M32OPTION_FLYER_DIRECTORY);
}

void SetFlyerDir(const char* path)
{
	options_set_string(settings, M32OPTION_FLYER_DIRECTORY, path);
}

const char* GetCabinetDir(void)
{
	return options_get_string(settings, M32OPTION_CABINET_DIRECTORY);
}

void SetCabinetDir(const char* path)
{
	options_set_string(settings, M32OPTION_CABINET_DIRECTORY, path);
}

const char* GetMarqueeDir(void)
{
	return options_get_string(settings, M32OPTION_MARQUEE_DIRECTORY);
}

void SetMarqueeDir(const char* path)
{
	options_set_string(settings, M32OPTION_MARQUEE_DIRECTORY, path);
}

const char* GetTitlesDir(void)
{
	return options_get_string(settings, M32OPTION_TITLE_DIRECTORY);
}

void SetTitlesDir(const char* path)
{
	options_set_string(settings, M32OPTION_TITLE_DIRECTORY, path);
}

const char * GetControlPanelDir(void)
{
	return options_get_string(settings, M32OPTION_CPANEL_DIRECTORY);
}

void SetControlPanelDir(const char *path)
{
	options_set_string(settings, M32OPTION_CPANEL_DIRECTORY, path);
}

const char * GetDiffDir(void)
{
	return options_get_string(global, OPTION_DIFF_DIRECTORY);
}

void SetDiffDir(const char* path)
{
	options_set_string(global, OPTION_DIFF_DIRECTORY, path);
}

const char* GetIconsDir(void)
{
	return options_get_string(settings, M32OPTION_ICONS_DIRECTORY);
}

void SetIconsDir(const char* path)
{
	options_set_string(settings, M32OPTION_ICONS_DIRECTORY, path);
}

const char* GetBgDir (void)
{
	return options_get_string(settings, M32OPTION_BACKGROUND_DIRECTORY);
}

void SetBgDir (const char* path)
{
	options_set_string(settings, M32OPTION_BACKGROUND_DIRECTORY, path);
}

const char* GetFolderDir(void)
{
	return options_get_string(settings, M32OPTION_FOLDER_DIRECTORY);
}

void SetFolderDir(const char* path)
{
	options_set_string(settings, M32OPTION_FOLDER_DIRECTORY, path);
}

const char* GetCheatFileName(void)
{
	return options_get_string(global, OPTION_CHEAT_FILE);
}

void SetCheatFileName(const char* path)
{
	options_set_string(global, OPTION_CHEAT_FILE, path);
}

const char* GetHistoryFileName(void)
{
	return options_get_string(settings, M32OPTION_HISTORY_FILE);
}

void SetHistoryFileName(const char* path)
{
	options_set_string(settings, M32OPTION_HISTORY_FILE, path);
}


const char* GetMAMEInfoFileName(void)
{
	return options_get_string(settings, M32OPTION_MAMEINFO_FILE);
}

void SetMAMEInfoFileName(const char* path)
{
	options_set_string(settings, M32OPTION_MAMEINFO_FILE, path);
}

void ResetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < driver_get_count());

	// make sure it's all loaded up.
	GetGameOptions(driver_index, GLOBAL_OPTIONS);

	if (game_options[driver_index] != NULL)
	{
		options_free(game_options[driver_index]);
		game_options[driver_index] = NULL;
		
		// this will delete the custom file
		SaveGameOptions(driver_index);
	}
}

void ResetGameDefaults(void)
{
	save_default_options = FALSE;
}

void ResetAllGameOptions(void)
{
	int i;
	int redirect_value = 0;

	for (i = 0; i < driver_get_count(); i++)
	{
		ResetGameOptions(i);
	}

	//with our new code we also need to delete the Source Folder Options and the Vector Options
	//Reset the Folder Options, can be done in one step
	for (i=0;i<GetNumOptionFolders();i++)
	{
		//Use the redirect array, to get correct folder
		redirect_value = GetRedirectValue(i);
		if( redirect_value < 0)
			continue;
 		options_copy(folder_options[i], GetDefaultOptions(GLOBAL_OPTIONS, FALSE));
 		SaveFolderOptions(redirect_value, 0);
	}
}

static void GetDriverOptionName(int driver_index, const char *option_name, char *buffer, size_t buffer_len)
{
	assert(0 <= driver_index && driver_index < driver_get_count());
	snprintf(buffer, buffer_len, "%s_%s", drivers[driver_index]->name, option_name);
}

int GetRomAuditResults(int driver_index)
{
	char buffer[128];
	GetDriverOptionName(driver_index, "rom_audit", buffer, ARRAY_LENGTH(buffer));
	return options_get_int(settings, buffer);
}

void SetRomAuditResults(int driver_index, int audit_results)
{
	char buffer[128];
	GetDriverOptionName(driver_index, "rom_audit", buffer, ARRAY_LENGTH(buffer));
	options_set_int(settings, buffer, audit_results);
}

int  GetSampleAuditResults(int driver_index)
{
	char buffer[128];
	GetDriverOptionName(driver_index, "samples_audit", buffer, ARRAY_LENGTH(buffer));
	return options_get_int(settings, buffer);
}

void SetSampleAuditResults(int driver_index, int audit_results)
{
	char buffer[128];
	GetDriverOptionName(driver_index, "samples_audit", buffer, ARRAY_LENGTH(buffer));
	options_set_int(settings, buffer, audit_results);
}

static void IncrementPlayVariable(int driver_index, const char *play_variable, int increment)
{
	char buffer[128];
	int count;

	GetDriverOptionName(driver_index, play_variable, buffer, ARRAY_LENGTH(buffer));
	count = options_get_int(settings, buffer);
	options_set_int(settings, buffer, count + increment);
}

void IncrementPlayCount(int driver_index)
{
	IncrementPlayVariable(driver_index, "play_count", 1);
}

int GetPlayCount(int driver_index)
{
	char buffer[128];
	GetDriverOptionName(driver_index, "play_count", buffer, ARRAY_LENGTH(buffer));
	return options_get_int(settings, buffer);
}

static void ResetPlayVariable(int driver_index, const char *play_variable)
{
	int i;
	if (driver_index < 0)
	{
		// all games
		for (i = 0; i< driver_get_count(); i++)
		{
			ResetPlayVariable(driver_index, play_variable);
		}
	}
	else
	{
		char buffer[128];
		GetDriverOptionName(driver_index, play_variable, buffer, ARRAY_LENGTH(buffer));
		options_set_int(settings, buffer, 0);
	}
}

void ResetPlayCount(int driver_index)
{
	ResetPlayVariable(driver_index, "play_count");
}

void ResetPlayTime(int driver_index)
{
	ResetPlayVariable(driver_index, "play_time");
}

int GetPlayTime(int driver_index)
{
	char buffer[128];
	GetDriverOptionName(driver_index, "play_time", buffer, ARRAY_LENGTH(buffer));
	return options_get_int(settings, buffer);
}

void IncrementPlayTime(int driver_index,int playtime)
{
	IncrementPlayVariable(driver_index, "play_time", playtime);
}

void GetTextPlayTime(int driver_index,char *buf)
{
	int hour, minute, second;
	int temp = GetPlayTime(driver_index);

	assert(0 <= driver_index && driver_index < driver_get_count());

	hour = temp / 3600;
	temp = temp - 3600*hour;
	minute = temp / 60; //Calc Minutes
	second = temp - 60*minute;

	if (hour == 0)
		sprintf(buf, "%d:%02d", minute, second );
	else
		sprintf(buf, "%d:%02d:%02d", hour, minute, second );
}


input_seq* Get_ui_key_up(void)
{
	return options_get_input_seq(settings, "ui_key_up");
}
input_seq* Get_ui_key_down(void)
{
	return options_get_input_seq(settings, "ui_key_down");
}
input_seq* Get_ui_key_left(void)
{
	return options_get_input_seq(settings, "ui_key_left");
}
input_seq* Get_ui_key_right(void)
{
	return options_get_input_seq(settings, "ui_key_right");
}
input_seq* Get_ui_key_start(void)
{
	return options_get_input_seq(settings, "ui_key_start");
}
input_seq* Get_ui_key_pgup(void)
{
	return options_get_input_seq(settings, "ui_key_pgup");
}
input_seq* Get_ui_key_pgdwn(void)
{
	return options_get_input_seq(settings, "ui_key_pgdwn");
}
input_seq* Get_ui_key_home(void)
{
	return options_get_input_seq(settings, "ui_key_home");
}
input_seq* Get_ui_key_end(void)
{
	return options_get_input_seq(settings, "ui_key_end");
}
input_seq* Get_ui_key_ss_change(void)
{
	return options_get_input_seq(settings, "ui_key_ss_change");
}
input_seq* Get_ui_key_history_up(void)
{
	return options_get_input_seq(settings, "ui_key_history_up");
}
input_seq* Get_ui_key_history_down(void)
{
	return options_get_input_seq(settings, "ui_key_history_down");
}


input_seq* Get_ui_key_context_filters(void)
{
	return options_get_input_seq(settings, "ui_key_context_filters");
}
input_seq* Get_ui_key_select_random(void)
{
	return options_get_input_seq(settings, "ui_key_select_random");
}
input_seq* Get_ui_key_game_audit(void)
{
	return options_get_input_seq(settings, "ui_key_game_audit");
}
input_seq* Get_ui_key_game_properties(void)
{
	return options_get_input_seq(settings, "ui_key_game_properties");
}
input_seq* Get_ui_key_help_contents(void)
{
	return options_get_input_seq(settings, "ui_key_help_contents");
}
input_seq* Get_ui_key_update_gamelist(void)
{
	return options_get_input_seq(settings, "ui_key_update_gamelist");
}
input_seq* Get_ui_key_view_folders(void)
{
	return options_get_input_seq(settings, "ui_key_view_folders");
}
input_seq* Get_ui_key_view_fullscreen(void)
{
	return options_get_input_seq(settings, "ui_key_view_fullscreen");
}
input_seq* Get_ui_key_view_pagetab(void)
{
	return options_get_input_seq(settings, "ui_key_view_pagetab");
}
input_seq* Get_ui_key_view_picture_area(void)
{
	return options_get_input_seq(settings, "ui_key_view_picture_area");
}
input_seq* Get_ui_key_view_status(void)
{
	return options_get_input_seq(settings, "ui_key_view_status");
}
input_seq* Get_ui_key_view_toolbars(void)
{
	return options_get_input_seq(settings, "ui_key_view_toolbars");
}

input_seq* Get_ui_key_view_tab_cabinet(void)
{
	return options_get_input_seq(settings, "ui_key_view_tab_cabinet");
}
input_seq* Get_ui_key_view_tab_cpanel(void)
{
	return options_get_input_seq(settings, "ui_key_view_tab_cpanel");
}
input_seq* Get_ui_key_view_tab_flyer(void)
{
	return options_get_input_seq(settings, "ui_key_view_tab_flyer");
}
input_seq* Get_ui_key_view_tab_history(void)
{
	return options_get_input_seq(settings, "ui_key_view_tab_history");
}
input_seq* Get_ui_key_view_tab_marquee(void)
{
	return options_get_input_seq(settings, "ui_key_view_tab_marquee");
}
input_seq* Get_ui_key_view_tab_screenshot(void)
{
	return options_get_input_seq(settings, "ui_key_view_tab_screenshot");
}
input_seq* Get_ui_key_view_tab_title(void)
{
	return options_get_input_seq(settings, "ui_key_view_tab_title");
}
input_seq* Get_ui_key_quit(void)
{
	return options_get_input_seq(settings, "ui_key_quit");
}



static int GetUIJoy(const char *option_name, int joycodeIndex)
{
	const char *joycodes_string;
	int joycodes[4];

	assert(0 <= joycodeIndex && joycodeIndex < 4);
	joycodes_string = options_get_string(settings, option_name);
	ColumnDecodeStringWithCount(joycodes_string, joycodes, ARRAY_LENGTH(joycodes));
	return joycodes[joycodeIndex];
}

static void SetUIJoy(const char *option_name, int joycodeIndex, int val)
{
	const char *joycodes_string;
	int joycodes[4];
	char buffer[1024];

	assert(0 <= joycodeIndex && joycodeIndex < 4);
	joycodes_string = options_get_string(settings, option_name);
	ColumnDecodeStringWithCount(joycodes_string, joycodes, ARRAY_LENGTH(joycodes));

	joycodes[joycodeIndex] = val;
	ColumnEncodeStringWithCount(joycodes, buffer, ARRAY_LENGTH(joycodes));
	options_set_string(settings, option_name, buffer);


}

int GetUIJoyUp(int joycodeIndex)
{
	return GetUIJoy("ui_joy_up", joycodeIndex);
}

void SetUIJoyUp(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_up", joycodeIndex, val);
}

int GetUIJoyDown(int joycodeIndex)
{
	return GetUIJoy("ui_joy_down", joycodeIndex);
}

void SetUIJoyDown(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_down", joycodeIndex, val);
}

int GetUIJoyLeft(int joycodeIndex)
{
	return GetUIJoy("ui_joy_left", joycodeIndex);
}

void SetUIJoyLeft(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_left", joycodeIndex, val);
}

int GetUIJoyRight(int joycodeIndex)
{
	return GetUIJoy("ui_joy_right", joycodeIndex);
}

void SetUIJoyRight(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_right", joycodeIndex, val);
}

int GetUIJoyStart(int joycodeIndex)
{
	return GetUIJoy("ui_joy_start", joycodeIndex);
}

void SetUIJoyStart(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_start", joycodeIndex, val);
}

int GetUIJoyPageUp(int joycodeIndex)
{
	return GetUIJoy("ui_joy_pgup", joycodeIndex);
}

void SetUIJoyPageUp(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_pgup", joycodeIndex, val);
}

int GetUIJoyPageDown(int joycodeIndex)
{
	return GetUIJoy("ui_joy_pgdwn", joycodeIndex);
}

void SetUIJoyPageDown(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_pgdwn", joycodeIndex, val);
}

int GetUIJoyHome(int joycodeIndex)
{
	return GetUIJoy("ui_joy_home", joycodeIndex);
}

void SetUIJoyHome(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_home", joycodeIndex, val);
}

int GetUIJoyEnd(int joycodeIndex)
{
	return GetUIJoy("ui_joy_end", joycodeIndex);
}

void SetUIJoyEnd(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_end", joycodeIndex, val);
}

int GetUIJoySSChange(int joycodeIndex)
{
	return GetUIJoy("ui_joy_ss_change", joycodeIndex);
}

void SetUIJoySSChange(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_ss_change", joycodeIndex, val);
}

int GetUIJoyHistoryUp(int joycodeIndex)
{
	return GetUIJoy("ui_joy_history_up", joycodeIndex);
}

void SetUIJoyHistoryUp(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_history_up", joycodeIndex, val);
}

int GetUIJoyHistoryDown(int joycodeIndex)
{
	return GetUIJoy("ui_joy_history_down", joycodeIndex);
}

void SetUIJoyHistoryDown(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_history_down", joycodeIndex, val);
}

void SetUIJoyExec(int joycodeIndex, int val)
{
	SetUIJoy("ui_joy_exec", joycodeIndex, val);
}

int GetUIJoyExec(int joycodeIndex)
{
	return GetUIJoy("ui_joy_exec", joycodeIndex);
}

const char * GetExecCommand(void)
{
	return options_get_string(settings, "exec_command");
}

void SetExecCommand(char *cmd)
{
	options_set_string(settings, "exec_command", cmd);
}

int GetExecWait(void)
{
	return options_get_int(settings, "exec_wait");
}

void SetExecWait(int wait)
{
	options_set_int(settings, "exec_wait", wait);
}
 
BOOL GetHideMouseOnStartup(void)
{
	return options_get_bool(settings, "hide_mouse");
}

void SetHideMouseOnStartup(BOOL hide)
{
	options_set_bool(settings, "hide_mouse", hide);
}

BOOL GetRunFullScreen(void)
{
	return options_get_bool(settings, "full_screen");
}

void SetRunFullScreen(BOOL fullScreen)
{
	options_set_bool(settings, "full_screen", fullScreen);
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void  CusColorEncodeString(const COLORREF *value, char* str)
{
	int  i;
	char tmpStr[256];

	sprintf(tmpStr, "%u", (int) value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < 16; i++)
	{
		sprintf(tmpStr, ",%u", (unsigned) value[i]);
		strcat(str, tmpStr);
	}
}

static void CusColorDecodeString(const char* str, COLORREF *value)
{
	int  i;
	char *s, *p;
	char tmpStr[256];

	strcpy(tmpStr, str);
	p = tmpStr;
	
	for (i = 0; p && i < 16; i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}


void ColumnEncodeStringWithCount(const int *value, char *str, int count)
{
	int  i;
	char buffer[100];

	snprintf(buffer,sizeof(buffer),"%d",value[0]);
	
	strcpy(str,buffer);

    for (i = 1; i < count; i++)
	{
		snprintf(buffer,sizeof(buffer),",%d",value[i]);
		strcat(str,buffer);
	}
}

void ColumnDecodeStringWithCount(const char* str, int *value, int count)
{
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
    for (i = 0; p && i < count; i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
    }
	}

static void KeySeqEncodeString(void *data, char* str)
{
	KeySeq* ks = (KeySeq*)data;

	sprintf(str, "%s", ks->seq_string);
}

static void KeySeqDecodeString(const char *str, void* data)
{
	KeySeq *ks = (KeySeq*)data;
	input_seq *is = &(ks->is);

	FreeIfAllocated(&ks->seq_string);
	ks->seq_string = mame_strdup(str);

	//get the new input sequence
	string_to_seq(str,is);
	//dprintf("seq=%s,,,%04i %04i %04i %04i \n",str,(*is)[0],(*is)[1],(*is)[2],(*is)[3]);
}

static void SplitterEncodeString(const int *value, char* str)
{
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < GetSplitterCount(); i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void SplitterDecodeString(const char *str, int *value)
{
	int  i;
	char *s, *p;
	char tmpStr[100];

	strcpy(tmpStr, str);
	p = tmpStr;
	
	for (i = 0; p && i < GetSplitterCount(); i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}

static void ListDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int i;

	*value = VIEW_GROUPED;

	for (i = VIEW_LARGE_ICONS; i < VIEW_MAX; i++)
	{
		if (strcmp(str, view_modes[i]) == 0)
		{
			*value = i;
			return;
		}
	}
}

static void ListEncodeString(void* data, char *str)
{
	int value = *((int*)data);
	const char* view_mode_string = "";

	if ((value >= 0) && (value < sizeof(view_modes) / sizeof(view_modes[0])))
		view_mode_string = view_modes[value];
	strcpy(str, view_mode_string);
}

/* Parse the given comma-delimited string into a LOGFONT structure */
static void FontDecodeString(const char* str, LOGFONT *f)
{
	char*	 ptr;
	
	sscanf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i",
		   &f->lfHeight,
		   &f->lfWidth,
		   &f->lfEscapement,
		   &f->lfOrientation,
		   &f->lfWeight,
		   (int*)&f->lfItalic,
		   (int*)&f->lfUnderline,
		   (int*)&f->lfStrikeOut,
		   (int*)&f->lfCharSet,
		   (int*)&f->lfOutPrecision,
		   (int*)&f->lfClipPrecision,
		   (int*)&f->lfQuality,
		   (int*)&f->lfPitchAndFamily);
	ptr = strrchr(str, ',');
	if (ptr != NULL)
		strcpy(f->lfFaceName, ptr + 1);
}

/* Encode the given LOGFONT structure into a comma-delimited string */
static void FontEncodeString(const LOGFONT *f, char *str)
{
	sprintf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i,%s",
			f->lfHeight,
			f->lfWidth,
			f->lfEscapement,
			f->lfOrientation,
			f->lfWeight,
			f->lfItalic,
			f->lfUnderline,
			f->lfStrikeOut,
			f->lfCharSet,
			f->lfOutPrecision,
			f->lfClipPrecision,
			f->lfQuality,
			f->lfPitchAndFamily,
			f->lfFaceName);
}

static void TabFlagsEncodeString(int data, char *str)
{
	int i;
	int num_saved = 0;

	strcpy(str,"");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if (((data & (1 << i)) == 0) && GetImageTabShortName(i))
		{
			if (num_saved != 0)
				strcat(str, ", ");

			strcat(str,GetImageTabShortName(i));
			num_saved++;
		}
	}
}

static void TabFlagsDecodeString(const char *str, int *data)
{
	char s[2000];
	char *token;

	snprintf(s, ARRAY_LENGTH(s), "%s", str);

	// simple way to set all tab bits "on"
	*data = (1 << MAX_TAB_TYPES) - 1;

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;j<MAX_TAB_TYPES;j++)
		{
			if (!GetImageTabShortName(j) || (strcmp(GetImageTabShortName(j), token) == 0))
			{
				// turn off this bit
				*data &= ~(1 << j);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}

	if (*data == 0)
	{
		// not allowed to hide all tabs, because then why even show the area?
		*data = (1 << TAB_SCREENSHOT);
	}
}



static file_error LoadSettingsFile(core_options *opts, const char *filename)
{
	core_file *file;
	file_error filerr;

	filerr = core_fopen(filename, OPEN_FLAG_READ, &file);
	if (filerr == FILERR_NONE)
	{
		options_parse_ini_file(opts, file);
		core_fclose(file);
	}
	return filerr;
}



static file_error SaveSettingsFile(core_options *opts, const char *filename)
{
	core_file *file;
	file_error filerr = FILERR_NONE;
	TCHAR *t_filename;

	if (opts != NULL)
	{
		filerr = core_fopen(filename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &file);
		if (filerr == FILERR_NONE)
		{
			options_output_ini_file(opts, file);
			core_fclose(file);
		}
	}
	else
	{
		t_filename = tstring_from_utf8(filename);
		DeleteFile(t_filename);
		free(t_filename);
	}

	return filerr;
}



static void GetGlobalOptionsFileName(char *filename, size_t filename_size)
{
	char buffer[MAX_PATH];
	char *base;
	char *s;

	// retrieve filename of executable
	win_get_module_file_name_utf8(GetModuleHandle(NULL), buffer, ARRAY_LENGTH(buffer));

	// locate base filename
	base = strrchr(buffer, '\\');
	base = base ? base + 1 : buffer;

	// remove ".exe"
	s = strstr(base, ".exe");
	if (s)
		*s = '\0';

	// copy INI directory
	snprintf(filename, filename_size, "%s\\%s.ini", GetIniDir(), base);
}



/* Register access functions below */
static void LoadOptionsAndSettings(void)
{
	char buffer[MAX_PATH];

	// parse MAME32.ini
	LoadSettingsFile(settings, UI_INI_FILENAME);

	// parse global options
	GetGlobalOptionsFileName(buffer, ARRAY_LENGTH(buffer));
	LoadSettingsFile(global, buffer);
}

void LoadGameOptions(int driver_index)
{
	char filename[MAX_PATH];

	options_copy(game_options[driver_index], global);

	snprintf(filename, ARRAY_LENGTH(filename), "%s\\%s.ini", GetIniDir(), drivers[driver_index]->name);
	LoadSettingsFile(game_options[driver_index], filename);
}

const char * GetFolderNameByID(UINT nID)
{
	UINT i;
	extern FOLDERDATA g_folderData[];
	extern LPEXFOLDERDATA ExtraFolderData[];

	for (i = 0; i < MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS; i++)
	{
		if( ExtraFolderData[i] )
		{
			if (ExtraFolderData[i]->m_nFolderId == nID)
			{
				return ExtraFolderData[i]->m_szTitle;
			}
		}
	}
	for( i = 0; i < MAX_FOLDERS; i++)
	{
		if (g_folderData[i].m_nFolderId == nID)
		{
			
			return g_folderData[i].m_lpTitle;
		}
	}
	return NULL;
}

void LoadFolderOptions(int folder_index )
{
	int redirect_index;
	char filename[MAX_PATH];

	GetFolderSettingsFileName(folder_index, filename, ARRAY_LENGTH(filename));
	
	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(folder_index);
	if( redirect_index < 0)
		return;

	options_copy(folder_options[redirect_index], global);
	if (!LoadSettingsFile(folder_options[redirect_index], filename))
	{
		// uses globals
		options_copy(folder_options[redirect_index], global);
	}
}

DWORD GetFolderFlags(int folder_index)
{
	int i;

	for (i=0;i<num_folder_filters;i++)
	{
		if (folder_filters[i].folder_index == folder_index)
		{
			//dprintf("found folder filter %i %i",folder_index,folder_filters[i].filters);
			return folder_filters[i].filters;
		}
	}
	return 0;
}

static void EmitFolderFilters(DWORD nSettingsFile, void (*emit_callback)(void *param_, const char *key, const char *value_str, const char *comment), void *param)
{
	int i;
	char key[32];
	char value_str[32];

	for (i = 0; i < GetNumFolders(); i++)
	{
		LPTREEFOLDER lpFolder = GetFolder(i);

		if ((lpFolder->m_dwFlags & F_MASK) != 0)
		{
			sprintf(key, "%i_filters", i);
			sprintf(value_str, "%i", (int)(lpFolder->m_dwFlags & F_MASK));
			emit_callback(param, key, value_str, lpFolder->m_lpTitle);
		}
	}
}



void SaveOptions(void)
{
	SaveSettingsFile(settings, UI_INI_FILENAME);
}

//returns true if same
BOOL GetVectorUsesDefaults(void)
{
	int redirect_index = 0;

	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(FOLDER_VECTOR);
	if( redirect_index < 0)
		return TRUE;

	return options_equal(folder_options[redirect_index], global);
}

//returns true if same
BOOL GetFolderUsesDefaults(int folder_index, int driver_index)
{
	core_options *opts;
	int redirect_index;

	if( DriverIsVector(driver_index) )
		opts = GetVectorOptions();
	else
		opts = global;

	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(folder_index);
	if( redirect_index < 0)
		return TRUE;

	return options_equal(folder_options[redirect_index], opts);
}

//returns true if same
BOOL GetGameUsesDefaults(int driver_index)
{
	core_options *opts = NULL;
	int nParentIndex = -1;
	BOOL bUsesDefaults = TRUE;

	assert(0 <= driver_index && driver_index < driver_get_count());

	if (game_options[driver_index] != NULL)
	{
		if ((driver_index >= 0) && DriverIsClone(driver_index))
		{
			nParentIndex = GetParentIndex(drivers[driver_index]);
			if( nParentIndex >= 0)
				opts = GetGameOptions(nParentIndex, FALSE);
			else
				//No Parent found, use source
				opts = GetSourceOptions(driver_index);
		}
		else
			opts = GetSourceOptions(driver_index);

		bUsesDefaults = options_equal(game_options[driver_index], opts);
	}
	return bUsesDefaults;
}

void SaveGameOptions(int driver_index)
{
	char filename[MAX_PATH];

	// save out the actual settings file
	snprintf(filename, ARRAY_LENGTH(filename), "%s\\%s.ini", GetIniDir(), drivers[driver_index]->name);
	SaveSettingsFile(game_options[driver_index], filename);
}

void SaveFolderOptions(int folder_index, int game_index)
{
	int redirect_index;
	char filename[MAX_PATH];

	GetFolderSettingsFileName(folder_index, filename, ARRAY_LENGTH(filename));
	
	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(folder_index);
	if( redirect_index < 0)
		return;

	options_copy(folder_options[redirect_index], global);
	SaveSettingsFile(folder_options[redirect_index], filename);
}


void SaveDefaultOptions(void)
{
	char buffer[MAX_PATH];
	GetGlobalOptionsFileName(buffer, ARRAY_LENGTH(buffer));
	SaveSettingsFile(global, buffer);
}

char * GetVersionString(void)
{
	return build_version;
}


BOOL IsGlobalOption(const char *option_name)
{
	static const char *global_options[] =
	{
		OPTION_ROMPATH,
#ifdef MESS
		OPTION_HASHPATH,
#endif // MESS
		OPTION_SAMPLEPATH,
		OPTION_ARTPATH,
		OPTION_CTRLRPATH,
		OPTION_INIPATH,
		OPTION_FONTPATH,
		OPTION_CFG_DIRECTORY,
		OPTION_NVRAM_DIRECTORY,
		OPTION_MEMCARD_DIRECTORY,
		OPTION_INPUT_DIRECTORY,
		OPTION_STATE_DIRECTORY,
		OPTION_SNAPSHOT_DIRECTORY,
		OPTION_DIFF_DIRECTORY,
		OPTION_COMMENT_DIRECTORY
	};
	int i;
	char command_name[128];
	char *s;

	// determine command name
	snprintf(command_name, ARRAY_LENGTH(command_name), "%s", option_name);
	s = strchr(command_name, ';');
	if (s)
		*s = '\0';

	for (i = 0; i < ARRAY_LENGTH(global_options); i++)
	{
		if (!strcmp(command_name, global_options[i]))
			return TRUE;
	}
	return FALSE;
}

