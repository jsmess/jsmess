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
#include "optcore.h"
#include "options.h"
#include "picker.h"
#include "windows/config.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

#ifdef MAME_DEBUG
static BOOL CheckOptions(const REG_OPTION *opts, BOOL bPassedToMAME);
#endif // MAME_DEBUG

static void LoadFolderFilter(int folder_index,int filters);

static BOOL LoadGameVariableOrFolderFilter(DWORD nSettingsFile, char *key,const char *value);
static void LoadOptionsAndSettings(void);

static void  ColumnEncodeString(void* data, char* str);
static void  ColumnDecodeString(const char* str, void* data);

static void  KeySeqEncodeString(void *data, char* str);
static void  KeySeqDecodeString(const char *str, void* data);

static void  JoyInfoEncodeString(void *data, char* str);
static void  JoyInfoDecodeString(const char *str, void* data);

static void  ColumnDecodeWidths(const char *ptr, void* data);

static void  CusColorEncodeString(void* data, char* str);
static void  CusColorDecodeString(const char* str, void* data);

static void  SplitterEncodeString(void* data, char* str);
static void  SplitterDecodeString(const char* str, void* data);

static void  ListEncodeString(void* data, char* str);
static void  ListDecodeString(const char* str, void* data);

static void  FontEncodeString(void* data, char* str);
static void  FontDecodeString(const char* str, void* data);

static void FolderFlagsEncodeString(void *data,char *str);
static void FolderFlagsDecodeString(const char *str,void *data);

static void TabFlagsEncodeString(void *data,char *str);
static void TabFlagsDecodeString(const char *str,void *data);

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/***************************************************************************
    Internal defines
 ***************************************************************************/

#define UI_INI_FILENAME MAME32NAME "ui.ini"
#define DEFAULT_OPTIONS_INI_FILENAME MAME32NAME ".ini"

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

static settings_type settings;

static options_type global; // Global 'default' options
static options_type *game_options;  // Array of Game specific options
static options_type *folder_options;  // Array of Folder specific options
static int *folder_options_redirect;  // Array of Folder Ids for redirecting Folder specific options
static game_variables_type *game_variables;  // Array of game specific extra data

static BOOL OnlyOnGame(int driver_index)
{
	return driver_index >= 0;
}

// UI options in mame32ui.ini
static const REG_OPTION regSettings[] =
{
#ifdef MESS
	{ "default_system",             RO_STRING,  offsetof(settings_type, default_game),    "nes" },
#else
	{ "default_game",               RO_STRING,  offsetof(settings_type, default_game),    "puckman" },
#endif
	{ "default_folder_id",          RO_INT,     offsetof(settings_type, folder_id),		         "0" },
	{ "show_image_section",         RO_BOOL,    offsetof(settings_type, show_screenshot),           "1" },
	{ "current_tab",                RO_STRING,  offsetof(settings_type, current_tab),               "0" },
	{ "show_tool_bar",              RO_BOOL,    offsetof(settings_type, show_toolbar),              "1" },
	{ "show_status_bar",            RO_BOOL,    offsetof(settings_type, show_statusbar),            "1" },
#ifdef MESS
	{ "show_folder_section",        RO_BOOL,    offsetof(settings_type, show_folderlist),           "0" },
#else
	{ "show_folder_section",        RO_BOOL,    offsetof(settings_type, show_folderlist),           "1" },
#endif
	{ "hide_folders",               RO_ENCODE,  offsetof(settings_type, show_folder_flags),         NULL, NULL, FolderFlagsEncodeString, FolderFlagsDecodeString },

#ifdef MESS
	{ "show_tabs",                  RO_BOOL,    offsetof(settings_type, show_tabctrl),              "0" },
	{ "hide_tabs",                  RO_ENCODE,  offsetof(settings_type, show_tab_flags),            "flyer, cabinet, marquee, title, cpanel", NULL, TabFlagsEncodeString, TabFlagsDecodeString },
	{ "history_tab",				RO_INT,		offsetof(settings_type, history_tab),				 "1" },
#else
	{ "show_tabs",                  RO_BOOL,    offsetof(settings_type, show_tabctrl),              "1" },
	{ "hide_tabs",                  RO_ENCODE,  offsetof(settings_type, show_tab_flags),            "marquee, title, cpanel, history", NULL, TabFlagsEncodeString, TabFlagsDecodeString },
	{ "history_tab",				RO_INT,		offsetof(settings_type, history_tab),				 0, 0},
#endif

	{ "check_game",                 RO_BOOL,    offsetof(settings_type, game_check),                "1" },
	{ "joystick_in_interface",      RO_BOOL,    offsetof(settings_type, use_joygui),                "0" },
	{ "keyboard_in_interface",      RO_BOOL,    offsetof(settings_type, use_keygui),                "0" },
	{ "broadcast_game_name",        RO_BOOL,    offsetof(settings_type, broadcast),                 "0" },
	{ "random_background",          RO_BOOL,    offsetof(settings_type, random_bg),                 "0" },

	{ "sort_column",                RO_INT,     offsetof(settings_type, sort_column),               "0" },
	{ "sort_reversed",              RO_BOOL,    offsetof(settings_type, sort_reverse),              "0" },
	{ "window_x",                   RO_INT,     offsetof(settings_type, area.x),                    "0" },
	{ "window_y",                   RO_INT,     offsetof(settings_type, area.y),                    "0" },
	{ "window_width",               RO_INT,     offsetof(settings_type, area.width),                "640" },
	{ "window_height",              RO_INT,     offsetof(settings_type, area.height),		         "400" },
	{ "window_state",               RO_INT,     offsetof(settings_type, windowstate),               "1" },

	{ "text_color",                 RO_COLOR,   offsetof(settings_type, list_font_color),           "-1",                          NULL },
	{ "clone_color",                RO_COLOR,   offsetof(settings_type, list_clone_color),          "-1",                          NULL },
	{ "custom_color",               RO_ENCODE,  offsetof(settings_type, custom_color),               "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", NULL, CusColorEncodeString, CusColorDecodeString},
	/* ListMode needs to be before ColumnWidths settings */
	{ "list_mode",                  RO_ENCODE,  offsetof(settings_type, view),                      "5",                           NULL, ListEncodeString,     ListDecodeString},
#ifdef MESS
	{ "splitters",                  RO_ENCODE,  offsetof(settings_type, splitter),                   "152,310,468",                 NULL, SplitterEncodeString, SplitterDecodeString},
#else
	{ "splitters",                  RO_ENCODE,  offsetof(settings_type, splitter),                   "152,362",                     NULL, SplitterEncodeString, SplitterDecodeString},
#endif
	{ "list_font",                  RO_ENCODE,  offsetof(settings_type, list_font),                 "-8,0,0,0,400,0,0,0,0,0,0,0,0,MS Sans Serif", NULL, FontEncodeString,     FontDecodeString},
	{ "column_widths",              RO_ENCODE,  offsetof(settings_type, column_width),              "185,68,84,84,64,88,74,108,60,144,84,60", NULL, ColumnEncodeString,   ColumnDecodeWidths},
	{ "column_order",               RO_ENCODE,  offsetof(settings_type, column_order),              "0,2,3,4,5,6,7,8,9,1,10,11",   NULL, ColumnEncodeString,   ColumnDecodeString},
	{ "column_shown",               RO_ENCODE,  offsetof(settings_type, column_shown),              "1,0,1,1,1,1,1,1,1,1,0,0",     NULL, ColumnEncodeString,   ColumnDecodeString},

	{ "ui_key_up",                  RO_ENCODE,  offsetof(settings_type, ui_key_up),                 "KEYCODE_UP",                  NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_down",                RO_ENCODE,  offsetof(settings_type, ui_key_down),               "KEYCODE_DOWN",                NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_left",                RO_ENCODE,  offsetof(settings_type, ui_key_left),               "KEYCODE_LEFT",                NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_right",               RO_ENCODE,  offsetof(settings_type, ui_key_right),              "KEYCODE_RIGHT",               NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_start",               RO_ENCODE,  offsetof(settings_type, ui_key_start),              "KEYCODE_ENTER NOT KEYCODE_LALT",NULL, KeySeqEncodeString, KeySeqDecodeString},
	{ "ui_key_pgup",                RO_ENCODE,  offsetof(settings_type, ui_key_pgup),               "KEYCODE_PGUP",                NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_pgdwn",               RO_ENCODE,  offsetof(settings_type, ui_key_pgdwn),              "KEYCODE_PGDN",                NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_home",                RO_ENCODE,  offsetof(settings_type, ui_key_home),               "KEYCODE_HOME",                NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_end",                 RO_ENCODE,  offsetof(settings_type, ui_key_end),                "KEYCODE_END",                 NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_ss_change",           RO_ENCODE,  offsetof(settings_type, ui_key_ss_change),          "KEYCODE_INSERT",              NULL, KeySeqEncodeString,   KeySeqDecodeString},
	{ "ui_key_history_up",          RO_ENCODE,  offsetof(settings_type, ui_key_history_up),         "KEYCODE_DEL",                 NULL, KeySeqEncodeString,  KeySeqDecodeString},
	{ "ui_key_history_down",        RO_ENCODE,  offsetof(settings_type, ui_key_history_down),       "KEYCODE_LALT KEYCODE_0",      NULL, KeySeqEncodeString,  KeySeqDecodeString},

    { "ui_key_context_filters",     RO_ENCODE,  offsetof(settings_type, ui_key_context_filters),     "KEYCODE_LCONTROL KEYCODE_F", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_select_random",       RO_ENCODE,  offsetof(settings_type, ui_key_select_random),       "KEYCODE_LCONTROL KEYCODE_R", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_game_audit",          RO_ENCODE,  offsetof(settings_type, ui_key_game_audit),          "KEYCODE_LALT KEYCODE_A",     NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_game_properties",     RO_ENCODE,  offsetof(settings_type, ui_key_game_properties),     "KEYCODE_LALT KEYCODE_ENTER", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_help_contents",       RO_ENCODE,  offsetof(settings_type, ui_key_help_contents),       "KEYCODE_F1",                 NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_update_gamelist",     RO_ENCODE,  offsetof(settings_type, ui_key_update_gamelist),     "KEYCODE_F5",                 NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_folders",        RO_ENCODE,  offsetof(settings_type, ui_key_view_folders),        "KEYCODE_LALT KEYCODE_D",     NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_fullscreen",     RO_ENCODE,  offsetof(settings_type, ui_key_view_fullscreen),     "KEYCODE_F11",                NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_pagetab",        RO_ENCODE,  offsetof(settings_type, ui_key_view_pagetab),        "KEYCODE_LALT KEYCODE_B",     NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_picture_area",   RO_ENCODE,  offsetof(settings_type, ui_key_view_picture_area),   "KEYCODE_LALT KEYCODE_P",     NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_status",         RO_ENCODE,  offsetof(settings_type, ui_key_view_status),         "KEYCODE_LALT KEYCODE_S",     NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_toolbars",       RO_ENCODE,  offsetof(settings_type, ui_key_view_toolbars),       "KEYCODE_LALT KEYCODE_T",     NULL, KeySeqEncodeString,  KeySeqDecodeString},

    { "ui_key_view_tab_cabinet",    RO_ENCODE,  offsetof(settings_type, ui_key_view_tab_cabinet),    "KEYCODE_LALT KEYCODE_3", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_tab_cpanel",     RO_ENCODE,  offsetof(settings_type, ui_key_view_tab_cpanel),     "KEYCODE_LALT KEYCODE_6", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_tab_flyer",      RO_ENCODE,  offsetof(settings_type, ui_key_view_tab_flyer),      "KEYCODE_LALT KEYCODE_2", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_tab_history",    RO_ENCODE,  offsetof(settings_type, ui_key_view_tab_history),    "KEYCODE_LALT KEYCODE_7", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_tab_marquee",    RO_ENCODE,  offsetof(settings_type, ui_key_view_tab_marquee),    "KEYCODE_LALT KEYCODE_4", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_tab_screenshot", RO_ENCODE,  offsetof(settings_type, ui_key_view_tab_screenshot), "KEYCODE_LALT KEYCODE_1", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_view_tab_title",      RO_ENCODE,  offsetof(settings_type, ui_key_view_tab_title),      "KEYCODE_LALT KEYCODE_5", NULL, KeySeqEncodeString,  KeySeqDecodeString},
    { "ui_key_quit",			    RO_ENCODE,  offsetof(settings_type, ui_key_quit),				  "KEYCODE_LALT KEYCODE_Q", NULL, KeySeqEncodeString,  KeySeqDecodeString},

	{ "ui_joy_up",                  RO_ENCODE,  offsetof(settings_type, ui_joy_up),                  NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_down",                RO_ENCODE,  offsetof(settings_type, ui_joy_down),                NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_left",                RO_ENCODE,  offsetof(settings_type, ui_joy_left),                NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_right",               RO_ENCODE,  offsetof(settings_type, ui_joy_right),               NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_start",               RO_ENCODE,  offsetof(settings_type, ui_joy_start),               NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_pgup",                RO_ENCODE,  offsetof(settings_type, ui_joy_pgup),                NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_pgdwn",               RO_ENCODE,  offsetof(settings_type, ui_joy_pgdwn),               NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_home",                RO_ENCODE,  offsetof(settings_type, ui_joy_home),                "0,0,0,0", NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_end",                 RO_ENCODE,  offsetof(settings_type, ui_joy_end),                 "0,0,0,0", NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_ss_change",           RO_ENCODE,  offsetof(settings_type, ui_joy_ss_change),           NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_history_up",          RO_ENCODE,  offsetof(settings_type, ui_joy_history_up),          NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_history_down",        RO_ENCODE,  offsetof(settings_type, ui_joy_history_down),        NULL, NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "ui_joy_exec",                RO_ENCODE,  offsetof(settings_type, ui_joy_exec),                "0,0,0,0", NULL, JoyInfoEncodeString,  JoyInfoDecodeString},
	{ "exec_command",               RO_STRING,  offsetof(settings_type, exec_command),               "" },
	{ "exec_wait",                  RO_INT,     offsetof(settings_type, exec_wait),                  "0" },
	{ "hide_mouse",                 RO_BOOL,    offsetof(settings_type, hide_mouse),                 "0" },
	{ "full_screen",                RO_BOOL,    offsetof(settings_type, full_screen),                "0" },
	{ "cycle_screenshot",           RO_INT,     offsetof(settings_type, cycle_screenshot),           "0" },
	{ "stretch_screenshot_larger",  RO_BOOL,    offsetof(settings_type, stretch_screenshot_larger),  "0" },
 	{ "screenshot_bordersize",      RO_INT,     offsetof(settings_type, screenshot_bordersize),      "11" },
 	{ "screenshot_bordercolor",     RO_COLOR,   offsetof(settings_type, screenshot_bordercolor),     "-1" },
	{ "inherit_filter",             RO_BOOL,    offsetof(settings_type, inherit_filter),             "0" },
	{ "offset_clones",              RO_BOOL,    offsetof(settings_type, offset_clones),              "0" },
	{ "game_caption",               RO_BOOL,    offsetof(settings_type, game_caption),               "1" },

	{ "language",                   RO_STRING,  offsetof(settings_type, language),         "english" },
	{ "flyer_directory",            RO_STRING,  offsetof(settings_type, flyerdir),         "flyers" },
	{ "cabinet_directory",          RO_STRING,  offsetof(settings_type, cabinetdir),       "cabinets" },
	{ "marquee_directory",          RO_STRING,  offsetof(settings_type, marqueedir),       "marquees" },
	{ "title_directory",            RO_STRING,  offsetof(settings_type, titlesdir),        "titles" },
	{ "cpanel_directory",           RO_STRING,  offsetof(settings_type, cpaneldir),        "cpanel" },
	{ "background_directory",       RO_STRING,  offsetof(settings_type, bgdir),            "bkground" },
	{ "folder_directory",           RO_STRING,  offsetof(settings_type, folderdir),        "folders" },
	{ "icons_directory",            RO_STRING,  offsetof(settings_type, iconsdir),         "icons" },
#ifdef MESS
	{ "sysinfo_file",				RO_STRING,  offsetof(settings_type, history_filename), "sysinfo.dat" },
	{ "messinfo_file",				RO_STRING,  offsetof(settings_type, mameinfo_filename),"messinfo.dat" },
#else
	{ "history_file",				RO_STRING,  offsetof(settings_type, history_filename), "history.dat" },
	{ "mameinfo_file",				RO_STRING,  offsetof(settings_type, mameinfo_filename),"mameinfo.dat" },
#endif

#ifdef MESS
	{ "mess_column_widths",         RO_ENCODE,  offsetof(settings_type, mess.mess_column_width), "186, 230, 88, 84, 84, 68, 248, 248",	NULL, MessColumnEncodeString, MessColumnDecodeWidths},
	{ "mess_column_order",          RO_ENCODE,  offsetof(settings_type, mess.mess_column_order),   "0,   1,  2,  3,  4,  5,   6,   7",	NULL, MessColumnEncodeString, MessColumnDecodeString},
	{ "mess_column_shown",          RO_ENCODE,  offsetof(settings_type, mess.mess_column_shown),   "1,   1,  1,  1,  1,  0,   0,   0",	NULL, MessColumnEncodeString, MessColumnDecodeString},

	{ "mess_sort_column",           RO_INT,     offsetof(settings_type, mess.mess_sort_column),    "0" },
	{ "mess_sort_reversed",         RO_BOOL,    offsetof(settings_type, mess.mess_sort_reverse),   "0" },

	{ "current_software_tab",       RO_STRING,  offsetof(settings_type, mess.software_tab),        "0" },
#endif
	{ "" }
};

// options in mame32.ini or (gamename).ini
static const REG_OPTION regGameOpts[] =
{
	// video
	{ "video",                  RO_STRING,  offsetof(options_type, videomode),                       "d3d" },
	{ "prescale",               RO_INT,    offsetof(options_type, prescale),                         "1" },
	{ "numscreens",             RO_INT,     offsetof(options_type, numscreens),                      "1" },
	{ "autoframeskip",          RO_BOOL,    offsetof(options_type, autoframeskip),                   "0" },
	{ "frameskip",              RO_INT,     offsetof(options_type, frameskip),                       "0" },
	{ "waitvsync",              RO_BOOL,    offsetof(options_type, wait_vsync),                      "0" },
	{ "effect",                 RO_STRING,  offsetof(options_type, effect),                          "none" },
#ifdef MESS
	{ "triplebuffer",           RO_BOOL,    offsetof(options_type, use_triplebuf),                   "0" },
#else
	{ "triplebuffer",           RO_BOOL,    offsetof(options_type, use_triplebuf),                   "1" },
#endif
	{ "window",                 RO_BOOL,    offsetof(options_type, window_mode),                     "0" },
	{ "hwstretch",              RO_BOOL,    offsetof(options_type, ddraw_stretch),                   "1" },
	{ "switchres",              RO_BOOL,    offsetof(options_type, switchres),                       "0" },
	{ "maximize",               RO_BOOL,    offsetof(options_type, maximize),                        "1" },
	{ "keepaspect",             RO_BOOL,    offsetof(options_type, keepaspect),                      "1" },
	{ "syncrefresh",            RO_BOOL,    offsetof(options_type, syncrefresh),                     "0" },
	{ "throttle",               RO_BOOL,    offsetof(options_type, throttle),                        "1" },
	{ "full_screen_gamma",		RO_DOUBLE,  offsetof(options_type, gfx_gamma),		                 "1.0" },
	{ "full_screen_brightness", RO_DOUBLE,  offsetof(options_type, gfx_brightness),                  "1.0" },
	{ "full_screen_contrast",   RO_DOUBLE,  offsetof(options_type, gfx_contrast),	                 "1.0" },
	{ "frames_to_run",          RO_INT,     offsetof(options_type, frames_to_display),               "0" },
	// per screen parameters
	{ "screen0",				RO_STRING,  offsetof(options_type, screen_params[0].screen),         "auto" },
	{ "aspect0",                RO_STRING,  offsetof(options_type, screen_params[0].aspect),         "auto" },
	{ "resolution0",            RO_STRING,  offsetof(options_type, screen_params[0].resolution),     "auto" },
	{ "view0",                  RO_STRING,  offsetof(options_type, screen_params[0].view),           "auto" },
	{ "screen1",				RO_STRING,  offsetof(options_type, screen_params[1].screen),         "auto" },
	{ "aspect1",                RO_STRING,  offsetof(options_type, screen_params[1].aspect),         "auto" },
	{ "resolution1",            RO_STRING,  offsetof(options_type, screen_params[1].resolution),     "auto" },
	{ "view1",                  RO_STRING,  offsetof(options_type, screen_params[1].view),           "auto" },
	{ "screen2",				RO_STRING,  offsetof(options_type, screen_params[2].screen),         "auto" },
	{ "aspect2",                RO_STRING,  offsetof(options_type, screen_params[2].aspect),         "auto" },
	{ "resolution2",            RO_STRING,  offsetof(options_type, screen_params[2].resolution),     "auto" },
	{ "view2",                  RO_STRING,  offsetof(options_type, screen_params[2].view),           "auto" },
	{ "screen3",				RO_STRING,  offsetof(options_type, screen_params[3].screen),         "auto" },
	{ "aspect3",                RO_STRING,  offsetof(options_type, screen_params[3].aspect),         "auto" },
	{ "resolution3",            RO_STRING,  offsetof(options_type, screen_params[3].resolution),     "auto" },
	{ "view3",                  RO_STRING,  offsetof(options_type, screen_params[3].view),           "auto" },

	// d3d
	{ "d3dfilter",              RO_BOOL,    offsetof(options_type, d3d_filter),                      "1" },
	{ "d3dversion",             RO_INT,     offsetof(options_type, d3d_version),                     "9" },
	
	//input
	{ "mouse",                  RO_BOOL,    offsetof(options_type, use_mouse),                       "0" },
	{ "joystick",               RO_BOOL,    offsetof(options_type, use_joystick),                    "0" },
	{ "a2d",                    RO_DOUBLE,  offsetof(options_type, f_a2d),                           "0.3" },
	{ "steadykey",              RO_BOOL,    offsetof(options_type, steadykey),                       "0" },
	{ "lightgun",               RO_BOOL,    offsetof(options_type, lightgun),                        "0" },
	{ "dual_lightgun",          RO_BOOL,    offsetof(options_type, dual_lightgun),                   "0" },
	{ "offscreen_reload",       RO_BOOL,    offsetof(options_type, offscreen_reload),                "0" },
	{ "ctrlr",                  RO_STRING,  offsetof(options_type, ctrlr),                           "" },
	{ "digital",                RO_STRING,  offsetof(options_type, digital),                         "" },
	{ "paddle",	                RO_STRING,  offsetof(options_type, paddle),					        "" },
	{ "adstick",	            RO_STRING,  offsetof(options_type, adstick),			                "" },
	{ "pedal",			        RO_STRING,  offsetof(options_type, pedal),				            "" },
	{ "dial",				    RO_STRING,  offsetof(options_type, dial),		                    "" },
	{ "trackball",				RO_STRING,  offsetof(options_type, trackball),                       "" },
	{ "lightgun_device",        RO_STRING,  offsetof(options_type, lightgun_device),                 "" },

	// core video
	{ "contrast",               RO_DOUBLE,  offsetof(options_type, f_contrast_correct),	             "1.0" }, 
	{ "brightness",             RO_DOUBLE,  offsetof(options_type, f_bright_correct),                "1.0" }, 
	{ "pause_brightness",       RO_DOUBLE,  offsetof(options_type, f_pause_bright),                  "0.65" }, 
	{ "rotate",                 RO_BOOL,    offsetof(options_type, rotate),                          "1" },
	{ "ror",                    RO_BOOL,    offsetof(options_type, ror),                             "0" },
	{ "rol",                    RO_BOOL,    offsetof(options_type, rol),                             "0" },
	{ "autoror",                RO_BOOL,    offsetof(options_type, auto_ror),                        "0" },
	{ "autorol",                RO_BOOL,    offsetof(options_type, auto_rol),                        "0" },
	{ "flipx",                  RO_BOOL,    offsetof(options_type, flipx),                           "0" },
	{ "flipy",                  RO_BOOL,    offsetof(options_type, flipy),                           "0" },
	{ "gamma",                  RO_DOUBLE,  offsetof(options_type, f_gamma_correct),                 "1.0" },

	// vector
	{ "antialias",              RO_BOOL,    offsetof(options_type, antialias),                       "1" },
	{ "beam",                   RO_DOUBLE,  offsetof(options_type, f_beam),                          "1.0" },
	{ "flicker",                RO_DOUBLE,  offsetof(options_type, f_flicker),                       "0.0" },

	// sound
	{ "samplerate",             RO_INT,     offsetof(options_type, samplerate),                      "48000" },
	{ "samples",                RO_BOOL,    offsetof(options_type, use_samples),                     "1" },
	{ "sound",                  RO_BOOL,    offsetof(options_type, enable_sound),                    "1" },
	{ "volume",                 RO_INT,     offsetof(options_type, attenuation),                     "0" },
	{ "audio_latency",          RO_INT,     offsetof(options_type, audio_latency),                   "1" },

	// misc artwork options
	{ "backdrop",               RO_BOOL,    offsetof(options_type, backdrops),                       "1" },
	{ "overlay",                RO_BOOL,    offsetof(options_type, overlays),                        "1" },
	{ "bezel",                  RO_BOOL,    offsetof(options_type, bezels),                          "1" },
	{ "artwork_crop",           RO_BOOL,    offsetof(options_type, artwork_crop),                    "0" },

	// misc
	{ "cheat",                  RO_BOOL,    offsetof(options_type, cheat),                           "0" },
	{ "debug",                  RO_BOOL,    offsetof(options_type, mame_debug),                      "0" },
	{ "log",                    RO_BOOL,    offsetof(options_type, errorlog),                        "0" },
	{ "sleep",                  RO_BOOL,    offsetof(options_type, sleep),                           "1" },
	{ "rdtsc",                  RO_BOOL,    offsetof(options_type, old_timing),                      "0" },
	{ "priority",               RO_INT,     offsetof(options_type, priority),                        "0" },
	{ "skip_gameinfo",          RO_BOOL,    offsetof(options_type, skip_gameinfo),                   "0" },
#ifdef MESS
	{ "skip_warnings",          RO_BOOL,    offsetof(options_type, skip_warnings),     "0" },
#endif
	{ "bios",                   RO_STRING,  offsetof(options_type, bios),                            "default" },
#ifdef MESS
	{ "autosave",               RO_BOOL,    offsetof(options_type, autosave),                        "0" },
#else
	{ "autosave",               RO_BOOL,    offsetof(options_type, autosave),                        "1" },
#endif
	{ "mt",                     RO_BOOL,    offsetof(options_type, mt_render),                       "0" },

#ifdef MESS
	/* mess options */
	{ "newui",                  RO_BOOL,    offsetof(options_type, mess.use_new_ui),                 "1" },
	{ "ramsize",                RO_INT,     offsetof(options_type, mess.ram_size),                   NULL, OnlyOnGame },
#endif /* MESS */

	{ "" }
};

// options in mame32.ini that we'll never override with with game-specific options
static const REG_OPTION global_game_options[] =
{
#ifdef MESS
	{"skip_warnings",           RO_BOOL,    offsetof(settings_type, skip_warnings),     "0" },
#endif

	{ "rompath",                RO_STRING,  offsetof(settings_type, romdirs),          "roms" },
#ifdef MESS
	{ "softwarepath",           RO_STRING,  offsetof(settings_type, mess.softwaredirs),"software" },
	{ "hashpath",               RO_STRING,  offsetof(settings_type, mess.hashdir),     "hash" },
#endif
	{ "samplepath",             RO_STRING,  offsetof(settings_type, sampledirs),       "samples", },
	{ "inipath",                RO_STRING,  offsetof(settings_type, inidir),           "ini" },
	{ "cfg_directory",          RO_STRING,  offsetof(settings_type, cfgdir),           "cfg" },
	{ "nvram_directory",        RO_STRING,  offsetof(settings_type, nvramdir),         "nvram" },
	{ "memcard_directory",      RO_STRING,  offsetof(settings_type, memcarddir),       "memcard" },
	{ "input_directory",        RO_STRING,  offsetof(settings_type, inpdir),           "inp" },
	{ "state_directory",        RO_STRING,  offsetof(settings_type, statedir),         "sta" },
	{ "artwork_directory",      RO_STRING,  offsetof(settings_type, artdir),           "artwork" },
	{ "snapshot_directory",     RO_STRING,  offsetof(settings_type, imgdir),           "snap" },
	{ "diff_directory",         RO_STRING,  offsetof(settings_type, diffdir),          "diff" },
	{ "cheat_file",             RO_STRING,  offsetof(settings_type, cheat_filename),   "cheat.dat" },
	{ "ctrlr_directory",        RO_STRING,  offsetof(settings_type, ctrlrdir),         "ctrlr" },
	{ "comment_directory",      RO_STRING,  offsetof(settings_type, commentdir),       "comment" },
	{ "" }

};

static const REG_OPTION gamevariable_options[] =
{
	{ "play_count",		RO_INT,		offsetof(game_variables_type, play_count),                "0" },
	{ "play_time",		RO_INT, 	offsetof(game_variables_type, play_time),                 "0" },
	{ "rom_audit",		RO_INT,		offsetof(game_variables_type, rom_audit_results),         "-1" },
	{ "samples_audit",	RO_INT,		offsetof(game_variables_type, samples_audit_results),     "-1", DriverUsesSamples },
#ifdef MESS
	{ "extra_software",	RO_STRING,	offsetof(game_variables_type, mess.extra_software_paths), "" }
#endif
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
	"history",
};


static int  num_games = 0;
static BOOL save_gui_settings = TRUE;
static BOOL save_default_options = TRUE;

static const char *view_modes[VIEW_MAX] = { "Large Icons", "Small Icons", "List", "Details", "Grouped" };

folder_filter_type *folder_filters;
int size_folder_filters;
int num_folder_filters;



static const options_entry windows_opts[] =
{
	// file and directory options
	{ NULL,                       NULL,       OPTION_HEADER,     "PATH AND DIRECTORY OPTIONS" },
#ifndef MESS
	{ "rompath;rp",               "roms",     0,                 "path to ROMsets and hard disk images" },
#else
	{ "rompath;rp",               "roms",     0,                 "path to ROMsets" },
	{ "softwarepath;swp",         "software", 0,                 "path to software" },
	{ "hash_directory;hash",      "hash",     0,                 "path to hash files" },
#endif
	{ "samplepath;sp",            "samples",  0,                 "path to samplesets" },
#ifdef __WIN32__
	{ "inipath",                  ".;ini",    0,                 "path to ini files" },
#else
	{ "inipath",                  "$HOME/.mame;.;ini", 0,        "path to ini files" },
#endif
	{ "cfg_directory",            "cfg",      0,                 "directory to save configurations" },
	{ "nvram_directory",          "nvram",    0,                 "directory to save nvram contents" },
	{ "memcard_directory",        "memcard",  0,                 "directory to save memory card contents" },
	{ "input_directory",          "inp",      0,                 "directory to save input device logs" },
	{ "state_directory",          "sta",      0,                 "directory to save states" },
	{ "artpath;artwork_directory","artwork",  0,                 "path to artwork files" },
	{ "snapshot_directory",       "snap",     0,                 "directory to save screenshots" },
	{ "diff_directory",           "diff",     0,                 "directory to save hard drive image difference files" },
	{ "ctrlrpath;ctrlr_directory","ctrlr",    0,                 "path to controller definitions" },
	{ "comment_directory",        "comments", 0,                 "directory to save debugger comments" },
	{ "cheat_file",               "cheat.dat",0,                 "cheat filename" },
	{ NULL }
};


	
/***************************************************************************
    External functions  
 ***************************************************************************/

BOOL OptionsInit()
{
	int i;

#ifdef MAME_DEBUG
	if (!CheckOptions(regSettings, FALSE))
		return FALSE;
	if (!CheckOptions(regGameOpts, TRUE))
		return FALSE;
	if (!CheckOptions(global_game_options, TRUE))
		return FALSE;
#endif /* MAME_DEBUG */

	num_games = GetNumGames();
	code_init(NULL);
	// Load all default settings
	LoadDefaultOptions(&settings, regSettings);
	LoadDefaultOptions(&settings, global_game_options);
	LoadDefaultOptions(&global, regGameOpts);

	settings.view            = VIEW_GROUPED;
	settings.show_folder_flags = NewBits(MAX_FOLDERS);
	SetAllBits(settings.show_folder_flags,TRUE);

	settings.history_tab = TAB_HISTORY;

	settings.ui_joy_up[0] = 1;
	settings.ui_joy_up[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_up[2] = 2;
	settings.ui_joy_up[3] = JOYCODE_DIR_NEG;

	settings.ui_joy_down[0] = 1;
	settings.ui_joy_down[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_down[2] = 2;
	settings.ui_joy_down[3] = JOYCODE_DIR_POS;

	settings.ui_joy_left[0] = 1;
	settings.ui_joy_left[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_left[2] = 1;
	settings.ui_joy_left[3] = JOYCODE_DIR_NEG;

	settings.ui_joy_right[0] = 1;
	settings.ui_joy_right[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_right[2] = 1;
	settings.ui_joy_right[3] = JOYCODE_DIR_POS;

	settings.ui_joy_start[0] = 1;
	settings.ui_joy_start[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_start[2] = 1;
	settings.ui_joy_start[3] = JOYCODE_DIR_BTN;

	settings.ui_joy_pgup[0] = 2;
	settings.ui_joy_pgup[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_pgup[2] = 2;
	settings.ui_joy_pgup[3] = JOYCODE_DIR_NEG;

	settings.ui_joy_pgdwn[0] = 2;
	settings.ui_joy_pgdwn[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_pgdwn[2] = 2;
	settings.ui_joy_pgdwn[3] = JOYCODE_DIR_POS;

	settings.ui_joy_history_up[0] = 2;
	settings.ui_joy_history_up[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_history_up[2] = 4;
	settings.ui_joy_history_up[3] = JOYCODE_DIR_BTN;

	settings.ui_joy_history_down[0] = 2;
	settings.ui_joy_history_down[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_history_down[2] = 1;
	settings.ui_joy_history_down[3] = JOYCODE_DIR_BTN;

	settings.ui_joy_ss_change[0] = 2;
	settings.ui_joy_ss_change[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_ss_change[2] = 3;
	settings.ui_joy_ss_change[3] = JOYCODE_DIR_BTN;


	// game_options[x] is valid iff game_variables[i].options_loaded == true
	game_options = (options_type *)malloc(num_games * sizeof(options_type));
	game_variables = (game_variables_type *)malloc(num_games * sizeof(game_variables_type));

	if (!game_options || !game_variables)
		return FALSE;

	memset(game_options, 0, num_games * sizeof(options_type));
	memset(game_variables, 0, num_games * sizeof(game_variables_type));
	for (i = 0; i < num_games; i++)
	{
		game_variables[i].play_count = 0;
		game_variables[i].play_time = 0;
		game_variables[i].rom_audit_results = UNKNOWN;
		game_variables[i].samples_audit_results = UNKNOWN;
		
		game_variables[i].options_loaded = FALSE;
		game_variables[i].use_default = TRUE;
	}

	size_folder_filters = 1;
	num_folder_filters = 0;
	folder_filters = (folder_filter_type *)malloc(size_folder_filters*sizeof(folder_filter_type));
	if (!folder_filters)
		return FALSE;

	LoadOptionsAndSettings();

	// have our mame core (file code) know about our rom path
	options_add_entries(windows_opts);
	options_set_string(SEARCHPATH_ROM, settings.romdirs);
	options_set_string(SEARCHPATH_SAMPLE, settings.sampledirs);
#ifdef MESS
	options_set_string(SEARCHPATH_HASH, settings.mess.hashdir);
#endif // MESS
	return TRUE;

}

void FolderOptionsInit()
{
	//Get Number of Folder Entries with options from folder_options_redirect
	int i=GetNumOptionFolders();

	folder_options = (options_type *)malloc( i * sizeof(options_type));
	memset(folder_options, 0, (i * sizeof(options_type)) );
	return;
}

void OptionsExit(void)
{
	int i;

	for (i=0;i<num_games;i++)
	{
		FreeGameOptions(&game_options[i]);
	}

    free(game_options);

	for (i=0;i<GetNumOptionFolders();i++)
	{
		FreeGameOptions(&folder_options[i]);
	}
	free(folder_options);

	FreeGameOptions(&global);
	FreeOptionStruct(&settings, regSettings);

	free(folder_options_redirect);
	DeleteBits(settings.show_folder_flags);
	settings.show_folder_flags = NULL;
}

// frees the sub-data (strings)
void FreeGameOptions(options_type *o)
{
	FreeOptionStruct(o, regGameOpts);
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

// performs a "deep" copy--strings in source are allocated and copied in dest
void CopyGameOptions(const options_type *source,options_type *dest)
{
	CopyOptionStruct(dest, source, regGameOpts, sizeof(options_type));
}

static DWORD GetFolderSettingsFileID(int folder_index)
{
	extern FOLDERDATA g_folderData[];
	extern LPEXFOLDERDATA ExtraFolderData[];
	extern int numExtraFolders;
	DWORD nSettingsFile = 0;
	int i, j;
	const char *pParent;

	if (folder_index < MAX_FOLDERS)
	{
		for (i = 0; g_folderData[i].m_lpTitle; i++)
		{
			if (g_folderData[i].m_nFolderId == folder_index)
			{
				nSettingsFile = i | SETTINGS_FILE_FOLDER;				
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
				nSettingsFile = i | SETTINGS_FILE_EXFOLDER;				
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
				nSettingsFile = folder_index | SETTINGS_FILE_EXFOLDERPARENT;

				if( ExtraFolderData[folder_index]->m_nParent == FOLDER_SOURCE )
				{
					for (j = 0; drivers[j]; j++)
					{
						if (!strcmp(GetDriverFilename(j), ExtraFolderData[folder_index]->m_szTitle))
						{
							nSettingsFile = j | SETTINGS_FILE_SOURCEFILE;
							break;
						}
					}
				}
			}
		}
	}
	return nSettingsFile;
}

// sync in the options specified in filename
void SyncInFolderOptions(options_type *opts, int folder_index)
{
	DWORD nSettingsFile;

	nSettingsFile = GetFolderSettingsFileID(folder_index);
	if (nSettingsFile)
		LoadSettingsFile(nSettingsFile, opts, regGameOpts);
}


options_type * GetDefaultOptions(int iProperty, BOOL bVectorFolder )
{
	if( iProperty == GLOBAL_OPTIONS)
		return &global;
	else if( iProperty == FOLDER_OPTIONS)
	{
		if (bVectorFolder)
			return &global;
		else
			return GetVectorOptions();
	}
	else
	{
		assert(0 <= iProperty && iProperty < num_games);
		return GetSourceOptions( iProperty );
	}
}

options_type * GetFolderOptions(int folder_index, BOOL bIsVector)
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
	CopyGameOptions(&global,&folder_options[redirect_index]);
	LoadFolderOptions(folder_index);
	if( bIsVector)
	{
		SyncInFolderOptions(&folder_options[redirect_index], FOLDER_VECTOR);
	}
	return &folder_options[redirect_index];
}

int * GetFolderOptionsRedirectArr(void)
{
	return &folder_options_redirect[0];
}

void SetFolderOptionsRedirectArr(int *redirect_arr)
{
	folder_options_redirect = redirect_arr;
}


options_type * GetVectorOptions(void)
{
	static options_type vector_ops;

	//initialize vector_ops with global settings, we accumulate all settings there and copy them 
	//to game_options[driver_index] in the end
	CopyGameOptions(&global, &vector_ops);
	
	//If it is a Vector game sync in the Vector.ini settings
	SyncInFolderOptions(&vector_ops, FOLDER_VECTOR);

	return &vector_ops;
}

options_type * GetSourceOptions(int driver_index )
{
	static options_type source_opts;

	assert(0 <= driver_index && driver_index < num_games);
	//initialize source_opts with global settings, we accumulate all settings there and copy them 
	//to game_options[driver_index] in the end
	CopyGameOptions(&global, &source_opts);
	
	if( DriverIsVector(driver_index) )
	{
		//If it is a Vector game sync in the Vector.ini settings
		SyncInFolderOptions(&source_opts, FOLDER_VECTOR);
	}

	//If it has source folder settings sync in the source\sourcefile.ini settings
	LoadSettingsFile(driver_index | SETTINGS_FILE_SOURCEFILE, &source_opts, regGameOpts);

	return &source_opts;
}

options_type * GetGameOptions(int driver_index, int folder_index )
{
	int parent_index, setting;
	struct SettingsHandler handlers[3];

	assert(0 <= driver_index && driver_index < num_games);

	CopyGameOptions(&global, &game_options[driver_index]);
	
	if( DriverIsVector(driver_index) )
	{
		//If it is a Vector game sync in the Vector.ini settings
		SyncInFolderOptions(&game_options[driver_index], FOLDER_VECTOR);
	}
	
	//If it has source folder settings sync in the source\sourcefile.ini settings
	LoadSettingsFile(driver_index | SETTINGS_FILE_SOURCEFILE, &game_options[driver_index], regGameOpts);
	
	//Sync in parent settings if it has one
	if( DriverIsClone(driver_index))
	{
		parent_index = GetParentIndex(drivers[driver_index]);
		LoadSettingsFile(parent_index | SETTINGS_FILE_GAME, &game_options[driver_index], regGameOpts);
	}

	//last but not least, sync in game specific settings
	memset(handlers, 0, sizeof(handlers));
	setting = 0;
	handlers[setting].type = SH_OPTIONSTRUCT;
	handlers[setting].u.option_struct.option_struct = (void *) &game_options[driver_index];
	handlers[setting].u.option_struct.option_array = regGameOpts;
	setting++;
#ifdef MESS
	handlers[setting].type = SH_MANUAL;
	handlers[setting].u.manual.parse = LoadDeviceOption;
	setting++;
#endif // MESS
	handlers[setting].type = SH_END;
	
	LoadSettingsFileEx(driver_index | SETTINGS_FILE_GAME, handlers);
	return &game_options[driver_index];
}

void SetGameUsesDefaults(int driver_index,BOOL use_defaults)
{
	if (driver_index < 0)
	{
		dprintf("got setgameusesdefaults with driver index %i",driver_index);
		return;
	}
	game_variables[driver_index].use_default = use_defaults;
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
	return image_tabs_long_name[tab_index];
}

const char * GetImageTabShortName(int tab_index)
{
	return image_tabs_short_name[tab_index];
}

void SetViewMode(int val)
{
	settings.view = val;
}

int GetViewMode(void)
{
	return settings.view;
}

void SetGameCheck(BOOL game_check)
{
	settings.game_check = game_check;
}

BOOL GetGameCheck(void)
{
	return settings.game_check;
}

void SetJoyGUI(BOOL use_joygui)
{
	settings.use_joygui = use_joygui;
}

BOOL GetJoyGUI(void)
{
	return settings.use_joygui;
}

void SetKeyGUI(BOOL use_keygui)
{
	settings.use_keygui = use_keygui;
}

BOOL GetKeyGUI(void)
{
	return settings.use_keygui;
}

void SetCycleScreenshot(int cycle_screenshot)
{
	settings.cycle_screenshot = cycle_screenshot;
}

int GetCycleScreenshot(void)
{
	return settings.cycle_screenshot;
}

void SetStretchScreenShotLarger(BOOL stretch)
{
	settings.stretch_screenshot_larger = stretch;
}

BOOL GetStretchScreenShotLarger(void)
{
	return settings.stretch_screenshot_larger;
}

void SetScreenshotBorderSize(int size)
{
	settings.screenshot_bordersize = size;
}

int GetScreenshotBorderSize(void)
{
	return settings.screenshot_bordersize;
}

void SetScreenshotBorderColor(COLORREF uColor)
{
	if (settings.screenshot_bordercolor == GetSysColor(COLOR_3DFACE))
		settings.screenshot_bordercolor = (COLORREF)-1;
	else
		settings.screenshot_bordercolor = uColor;
}

COLORREF GetScreenshotBorderColor(void)
{
	if (settings.screenshot_bordercolor == (COLORREF)-1)
		return (GetSysColor(COLOR_3DFACE));

	return settings.screenshot_bordercolor;
}

void SetFilterInherit(BOOL inherit)
{
	settings.inherit_filter = inherit;
}

BOOL GetFilterInherit(void)
{
	return settings.inherit_filter;
}

void SetOffsetClones(BOOL offset)
{
	settings.offset_clones = offset;
}

BOOL GetOffsetClones(void)
{
	return settings.offset_clones;
}

void SetGameCaption(BOOL caption)
{
	settings.game_caption = caption;
}

BOOL GetGameCaption(void)
{
	return settings.game_caption;
}

void SetBroadcast(BOOL broadcast)
{
	settings.broadcast = broadcast;
}

BOOL GetBroadcast(void)
{
	return settings.broadcast;
}

void SetRandomBackground(BOOL random_bg)
{

	settings.random_bg = random_bg;
}

BOOL GetRandomBackground(void)
{
	return settings.random_bg;
}

void SetSavedFolderID(UINT val)
{
	settings.folder_id = val;
}

UINT GetSavedFolderID(void)
{
	return settings.folder_id;
}

void SetShowScreenShot(BOOL val)
{
	settings.show_screenshot = val;
}

BOOL GetShowScreenShot(void)
{
	return settings.show_screenshot;
}

void SetShowFolderList(BOOL val)
{
	settings.show_folderlist = val;
}

BOOL GetShowFolderList(void)
{
	return settings.show_folderlist;
}

BOOL GetShowFolder(int folder)
{
	return TestBit(settings.show_folder_flags,folder);
}

void SetShowFolder(int folder,BOOL show)
{
	if (show)
		SetBit(settings.show_folder_flags,folder);
	else
		ClearBit(settings.show_folder_flags,folder);
}

void SetShowStatusBar(BOOL val)
{
	settings.show_statusbar = val;
}

BOOL GetShowStatusBar(void)
{
	return settings.show_statusbar;
}

void SetShowTabCtrl (BOOL val)
{
	settings.show_tabctrl = val;
}

BOOL GetShowTabCtrl (void)
{
	return settings.show_tabctrl;
}

void SetShowToolBar(BOOL val)
{
	settings.show_toolbar = val;
}

BOOL GetShowToolBar(void)
{
	return settings.show_toolbar;
}

void SetCurrentTab(const char *shortname)
{
	FreeIfAllocated(&settings.current_tab);
	if (shortname != NULL)
		settings.current_tab = mame_strdup(shortname);
}

const char *GetCurrentTab(void)
{
	return settings.current_tab;
}

void SetDefaultGame(const char *name)
{
	FreeIfAllocated(&settings.default_game);

	if (name != NULL)
		settings.default_game = mame_strdup(name);
}

const char *GetDefaultGame(void)
{
	return settings.default_game;
}

void SetWindowArea(AREA *area)
{
	memcpy(&settings.area, area, sizeof(AREA));
}

void GetWindowArea(AREA *area)
{
	memcpy(area, &settings.area, sizeof(AREA));
}

void SetWindowState(UINT state)
{
	settings.windowstate = state;
}

UINT GetWindowState(void)
{
	return settings.windowstate;
}

void SetCustomColor(int iIndex, COLORREF uColor)
{
	settings.custom_color[iIndex] = uColor;
}

COLORREF GetCustomColor(int iIndex)
{
	if (settings.custom_color[iIndex] == (COLORREF)-1)
		return (COLORREF)RGB(0,0,0);

	return settings.custom_color[iIndex];
}

void SetListFont(LOGFONT *font)
{
	memcpy(&settings.list_font, font, sizeof(LOGFONT));
}

void GetListFont(LOGFONT *font)
{
	memcpy(font, &settings.list_font, sizeof(LOGFONT));
}

void SetListFontColor(COLORREF uColor)
{
	if (settings.list_font_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_font_color = (COLORREF)-1;
	else
		settings.list_font_color = uColor;
}

COLORREF GetListFontColor(void)
{
	if (settings.list_font_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_font_color;
}

void SetListCloneColor(COLORREF uColor)
{
	if (settings.list_clone_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_clone_color = (COLORREF)-1;
	else
		settings.list_clone_color = uColor;
}

COLORREF GetListCloneColor(void)
{
	if (settings.list_clone_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_clone_color;

}

int GetShowTab(int tab)
{
	return (settings.show_tab_flags & (1 << tab)) != 0;
}

void SetShowTab(int tab,BOOL show)
{
	if (show)
		settings.show_tab_flags |= 1 << tab;
	else
		settings.show_tab_flags &= ~(1 << tab);
}

// don't delete the last one
BOOL AllowedToSetShowTab(int tab,BOOL show)
{
	int show_tab_flags = settings.show_tab_flags;

	if (show == TRUE)
		return TRUE;

	show_tab_flags &= ~(1 << tab);
	return show_tab_flags != 0;
}

int GetHistoryTab(void)
{
	return settings.history_tab;
}

void SetHistoryTab(int tab, BOOL show)
{
	if (show)
		settings.history_tab = tab;
	else
		settings.history_tab = TAB_NONE;
}

void SetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_width[i] = width[i];
}

void GetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		width[i] = settings.column_width[i];
}

void SetSplitterPos(int splitterId, int pos)
{
	if (splitterId < GetSplitterCount())
		settings.splitter[splitterId] = pos;
}

int  GetSplitterPos(int splitterId)
{
	if (splitterId < GetSplitterCount())
		return settings.splitter[splitterId];

	return -1; /* Error */
}

void SetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_order[i] = order[i];
}

void GetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		order[i] = settings.column_order[i];
}

void SetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_shown[i] = shown[i];
}

void GetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		shown[i] = settings.column_shown[i];
}

void SetSortColumn(int column)
{
	settings.sort_column = column;
}

int GetSortColumn(void)
{
	return settings.sort_column;
}

void SetSortReverse(BOOL reverse)
{
	settings.sort_reverse = reverse;
}

BOOL GetSortReverse(void)
{
	return settings.sort_reverse;
}

const char* GetLanguage(void)
{
	return settings.language;
}

void SetLanguage(const char* lang)
{
	FreeIfAllocated(&settings.language);

	if (lang != NULL)
		settings.language = mame_strdup(lang);
}

const char* GetRomDirs(void)
{
	return settings.romdirs;
}

void SetRomDirs(const char* paths)
{
	FreeIfAllocated(&settings.romdirs);

	if (paths != NULL)
	{
		settings.romdirs = mame_strdup(paths);

		// have our mame core (file code) know about it
		options_set_string(SEARCHPATH_ROM, settings.romdirs);
	}
}

const char* GetSampleDirs(void)
{
	return settings.sampledirs;
}

void SetSampleDirs(const char* paths)
{
	FreeIfAllocated(&settings.sampledirs);

	if (paths != NULL)
	{
		settings.sampledirs = mame_strdup(paths);
		
		// have our mame core (file code) know about it
		options_set_string(SEARCHPATH_SAMPLE, settings.sampledirs);
	}

}

const char * GetIniDir(void)
{
	return settings.inidir;
}

void SetIniDir(const char *path)
{
	FreeIfAllocated(&settings.inidir);

	if (path != NULL)
		settings.inidir = mame_strdup(path);
}

const char* GetCtrlrDir(void)
{
	return settings.ctrlrdir;
}

void SetCtrlrDir(const char* path)
{
	FreeIfAllocated(&settings.ctrlrdir);

	if (path != NULL)
		settings.ctrlrdir = mame_strdup(path);
}

const char* GetCommentDir(void)
{
	return settings.commentdir;
}

void SetCommentDir(const char* path)
{
	FreeIfAllocated(&settings.commentdir);

	if (path != NULL)
		settings.commentdir = mame_strdup(path);
}

const char* GetCfgDir(void)
{
	return settings.cfgdir;
}

void SetCfgDir(const char* path)
{
	FreeIfAllocated(&settings.cfgdir);

	if (path != NULL)
		settings.cfgdir = mame_strdup(path);
}

const char* GetNvramDir(void)
{
	return settings.nvramdir;
}

void SetNvramDir(const char* path)
{
	FreeIfAllocated(&settings.nvramdir);

	if (path != NULL)
		settings.nvramdir = mame_strdup(path);
}

const char* GetInpDir(void)
{
	return settings.inpdir;
}

void SetInpDir(const char* path)
{
	FreeIfAllocated(&settings.inpdir);

	if (path != NULL)
		settings.inpdir = mame_strdup(path);
}

const char* GetImgDir(void)
{
	return settings.imgdir;
}

void SetImgDir(const char* path)
{
	FreeIfAllocated(&settings.imgdir);

	if (path != NULL)
		settings.imgdir = mame_strdup(path);
}

const char* GetStateDir(void)
{
	return settings.statedir;
}

void SetStateDir(const char* path)
{
	FreeIfAllocated(&settings.statedir);

	if (path != NULL)
		settings.statedir = mame_strdup(path);
}

const char* GetArtDir(void)
{
	return settings.artdir;
}

void SetArtDir(const char* path)
{
	FreeIfAllocated(&settings.artdir);

	if (path != NULL)
		settings.artdir = mame_strdup(path);
}

const char* GetMemcardDir(void)
{
	return settings.memcarddir;
}

void SetMemcardDir(const char* path)
{
	FreeIfAllocated(&settings.memcarddir);

	if (path != NULL)
		settings.memcarddir = mame_strdup(path);
}

const char* GetFlyerDir(void)
{
	return settings.flyerdir;
}

void SetFlyerDir(const char* path)
{
	FreeIfAllocated(&settings.flyerdir);

	if (path != NULL)
		settings.flyerdir = mame_strdup(path);
}

const char* GetCabinetDir(void)
{
	return settings.cabinetdir;
}

void SetCabinetDir(const char* path)
{
	FreeIfAllocated(&settings.cabinetdir);

	if (path != NULL)
		settings.cabinetdir = mame_strdup(path);
}

const char* GetMarqueeDir(void)
{
	return settings.marqueedir;
}

void SetMarqueeDir(const char* path)
{
	FreeIfAllocated(&settings.marqueedir);

	if (path != NULL)
		settings.marqueedir = mame_strdup(path);
}

const char* GetTitlesDir(void)
{
	return settings.titlesdir;
}

void SetTitlesDir(const char* path)
{
	FreeIfAllocated(&settings.titlesdir);

	if (path != NULL)
		settings.titlesdir = mame_strdup(path);
}

const char * GetControlPanelDir(void)
{
	return settings.cpaneldir;
}

void SetControlPanelDir(const char *path)
{
	FreeIfAllocated(&settings.cpaneldir);
	if (path != NULL)
		settings.cpaneldir = mame_strdup(path);
}

const char * GetDiffDir(void)
{
	return settings.diffdir;
}

void SetDiffDir(const char* path)
{
	FreeIfAllocated(&settings.diffdir);

	if (path != NULL)
		settings.diffdir = mame_strdup(path);
}

const char* GetIconsDir(void)
{
	return settings.iconsdir;
}

void SetIconsDir(const char* path)
{
	FreeIfAllocated(&settings.iconsdir);

	if (path != NULL)
		settings.iconsdir = mame_strdup(path);
}

const char* GetBgDir (void)
{
	return settings.bgdir;
}

void SetBgDir (const char* path)
{
	FreeIfAllocated(&settings.bgdir);

	if (path != NULL)
		settings.bgdir = mame_strdup(path);
}

const char* GetFolderDir(void)
{
	return settings.folderdir;
}

void SetFolderDir(const char* path)
{
	FreeIfAllocated(&settings.folderdir);

	if (path != NULL)
		settings.folderdir = mame_strdup(path);
}

const char* GetCheatFileName(void)
{
	return settings.cheat_filename;
}

void SetCheatFileName(const char* path)
{
	FreeIfAllocated(&settings.cheat_filename);

	if (path != NULL)
		settings.cheat_filename = mame_strdup(path);
}

const char* GetHistoryFileName(void)
{
	return settings.history_filename;
}

void SetHistoryFileName(const char* path)
{
	FreeIfAllocated(&settings.history_filename);

	if (path != NULL)
		settings.history_filename = mame_strdup(path);
}


const char* GetMAMEInfoFileName(void)
{
	return settings.mameinfo_filename;
}

void SetMAMEInfoFileName(const char* path)
{
	FreeIfAllocated(&settings.mameinfo_filename);

	if (path != NULL)
		settings.mameinfo_filename = mame_strdup(path);
}

void ResetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	// make sure it's all loaded up.
	GetGameOptions(driver_index, GLOBAL_OPTIONS);

	if (game_variables[driver_index].use_default == FALSE)
	{
		FreeGameOptions(&game_options[driver_index]);
		game_variables[driver_index].use_default = TRUE;
		
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

	for (i = 0; i < num_games; i++)
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
 		CopyGameOptions(GetDefaultOptions(GLOBAL_OPTIONS, FALSE),&folder_options[i]);
 		SaveFolderOptions(redirect_value, 0);
	}
}

int GetRomAuditResults(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].rom_audit_results;
}

void SetRomAuditResults(int driver_index, int audit_results)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].rom_audit_results = audit_results;
}

int  GetSampleAuditResults(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].samples_audit_results;
}

void SetSampleAuditResults(int driver_index, int audit_results)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].samples_audit_results = audit_results;
}

void IncrementPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].play_count++;

	// maybe should do this
	//SavePlayCount(driver_index);
}

int GetPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].play_count;
}

void ResetPlayCount(int driver_index)
{
	int i = 0;
	assert(driver_index < num_games);
	if( driver_index < 0 )
	{
		//All games
		for( i= 0; i< num_games; i++ )
		{
			game_variables[i].play_count = 0;
		}
	}
	else
	{
		game_variables[driver_index].play_count = 0;
	}
}

void ResetPlayTime(int driver_index)
{
	int i = 0;
	assert(driver_index < num_games);
	if( driver_index < 0 )
	{
		//All games
		for( i= 0; i< num_games; i++ )
		{
			game_variables[i].play_time = 0;
		}
	}
	else
	{
		game_variables[driver_index].play_time = 0;
	}
}

int GetPlayTime(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].play_time;
}

void IncrementPlayTime(int driver_index,int playtime)
{
	assert(0 <= driver_index && driver_index < num_games);
	game_variables[driver_index].play_time += playtime;
}

void GetTextPlayTime(int driver_index,char *buf)
{
	int hour, minute, second;
	int temp = game_variables[driver_index].play_time;

	assert(0 <= driver_index && driver_index < num_games);

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
	return &settings.ui_key_up.is;
}
input_seq* Get_ui_key_down(void)
{
	return &settings.ui_key_down.is;
}
input_seq* Get_ui_key_left(void)
{
	return &settings.ui_key_left.is;
}
input_seq* Get_ui_key_right(void)
{
	return &settings.ui_key_right.is;
}
input_seq* Get_ui_key_start(void)
{
	return &settings.ui_key_start.is;
}
input_seq* Get_ui_key_pgup(void)
{
	return &settings.ui_key_pgup.is;
}
input_seq* Get_ui_key_pgdwn(void)
{
	return &settings.ui_key_pgdwn.is;
}
input_seq* Get_ui_key_home(void)
{
	return &settings.ui_key_home.is;
}
input_seq* Get_ui_key_end(void)
{
	return &settings.ui_key_end.is;
}
input_seq* Get_ui_key_ss_change(void)
{
	return &settings.ui_key_ss_change.is;
}
input_seq* Get_ui_key_history_up(void)
{
	return &settings.ui_key_history_up.is;
}
input_seq* Get_ui_key_history_down(void)
{
	return &settings.ui_key_history_down.is;
}


input_seq* Get_ui_key_context_filters(void)
{
	return &settings.ui_key_context_filters.is;
}
input_seq* Get_ui_key_select_random(void)
{
	return &settings.ui_key_select_random.is;
}
input_seq* Get_ui_key_game_audit(void)
{
	return &settings.ui_key_game_audit.is;
}
input_seq* Get_ui_key_game_properties(void)
{
	return &settings.ui_key_game_properties.is;
}
input_seq* Get_ui_key_help_contents(void)
{
	return &settings.ui_key_help_contents.is;
}
input_seq* Get_ui_key_update_gamelist(void)
{
	return &settings.ui_key_update_gamelist.is;
}
input_seq* Get_ui_key_view_folders(void)
{
	return &settings.ui_key_view_folders.is;
}
input_seq* Get_ui_key_view_fullscreen(void)
{
	return &settings.ui_key_view_fullscreen.is;
}
input_seq* Get_ui_key_view_pagetab(void)
{
	return &settings.ui_key_view_pagetab.is;
}
input_seq* Get_ui_key_view_picture_area(void)
{
	return &settings.ui_key_view_picture_area.is;
}
input_seq* Get_ui_key_view_status(void)
{
	return &settings.ui_key_view_status.is;
}
input_seq* Get_ui_key_view_toolbars(void)
{
	return &settings.ui_key_view_toolbars.is;
}

input_seq* Get_ui_key_view_tab_cabinet(void)
{
	return &settings.ui_key_view_tab_cabinet.is;
}
input_seq* Get_ui_key_view_tab_cpanel(void)
{
	return &settings.ui_key_view_tab_cpanel.is;
}
input_seq* Get_ui_key_view_tab_flyer(void)
{
	return &settings.ui_key_view_tab_flyer.is;
}
input_seq* Get_ui_key_view_tab_history(void)
{
	return &settings.ui_key_view_tab_history.is;
}
input_seq* Get_ui_key_view_tab_marquee(void)
{
	return &settings.ui_key_view_tab_marquee.is;
}
input_seq* Get_ui_key_view_tab_screenshot(void)
{
	return &settings.ui_key_view_tab_screenshot.is;
}
input_seq* Get_ui_key_view_tab_title(void)
{
	return &settings.ui_key_view_tab_title.is;
}
input_seq* Get_ui_key_quit(void)
{
	return &settings.ui_key_quit.is;
}



int GetUIJoyUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);
	
	return settings.ui_joy_up[joycodeIndex];
}

void SetUIJoyUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_up[joycodeIndex] = val;
}

int GetUIJoyDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_down[joycodeIndex];
}

void SetUIJoyDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_down[joycodeIndex] = val;
}

int GetUIJoyLeft(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_left[joycodeIndex];
}

void SetUIJoyLeft(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_left[joycodeIndex] = val;
}

int GetUIJoyRight(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_right[joycodeIndex];
}

void SetUIJoyRight(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_right[joycodeIndex] = val;
}

int GetUIJoyStart(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_start[joycodeIndex];
}

void SetUIJoyStart(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_start[joycodeIndex] = val;
}

int GetUIJoyPageUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_pgup[joycodeIndex];
}

void SetUIJoyPageUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_pgup[joycodeIndex] = val;
}

int GetUIJoyPageDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_pgdwn[joycodeIndex];
}

void SetUIJoyPageDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_pgdwn[joycodeIndex] = val;
}

int GetUIJoyHome(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_home[joycodeIndex];
}

void SetUIJoyHome(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_home[joycodeIndex] = val;
}

int GetUIJoyEnd(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_end[joycodeIndex];
}

void SetUIJoyEnd(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_end[joycodeIndex] = val;
}

int GetUIJoySSChange(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_ss_change[joycodeIndex];
}

void SetUIJoySSChange(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_ss_change[joycodeIndex] = val;
}

int GetUIJoyHistoryUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_history_up[joycodeIndex];
}

void SetUIJoyHistoryUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);
  
	settings.ui_joy_history_up[joycodeIndex] = val;
}

int GetUIJoyHistoryDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_history_down[joycodeIndex];
}

void SetUIJoyHistoryDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_history_down[joycodeIndex] = val;
}

void SetUIJoyExec(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_exec[joycodeIndex] = val;
}

int GetUIJoyExec(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_exec[joycodeIndex];
}

char * GetExecCommand(void)
{
	return settings.exec_command;
}

void SetExecCommand(char *cmd)
{
	settings.exec_command = cmd;
}

int GetExecWait(void)
{
	return settings.exec_wait;
}

void SetExecWait(int wait)
{
	settings.exec_wait = wait;
}
 
BOOL GetHideMouseOnStartup(void)
{
	return settings.hide_mouse;
}

void SetHideMouseOnStartup(BOOL hide)
{
	settings.hide_mouse = hide;
}

BOOL GetRunFullScreen(void)
{
	return settings.full_screen;
}

void SetRunFullScreen(BOOL fullScreen)
{
	settings.full_screen = fullScreen;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void CusColorEncodeString(void* data, char* str)
{
	int* value = (int*)data;
	int  i;
	char tmpStr[256];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < 16; i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void CusColorDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[256];

	if (str == NULL)
		return;

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


static void ColumnEncodeStringWithCount(void* data, char *str, int count)
{
	int* value = (int*)data;
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

static void ColumnDecodeStringWithCount(const char* str, void* data, int count)
{
	int* value = (int*)data;
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

static void ColumnEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, COLUMN_MAX);
}

static void ColumnDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, COLUMN_MAX);
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

static void JoyInfoEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, 4);
}

static void JoyInfoDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, 4);
}

static void ColumnDecodeWidths(const char* str, void* data)
{
	ColumnDecodeString(str, data);
}

static void SplitterEncodeString(void* data, char* str)
{
	int* value = (int*)data;
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

static void SplitterDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

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
static void FontDecodeString(const char* str, void* data)
{
	LOGFONT* f = (LOGFONT*)data;
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
static void FontEncodeString(void* data, char *str)
{
	LOGFONT* f = (LOGFONT*)data;

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


static void FolderFlagsEncodeString(void *data,char *str)
{
	int i;
	int num_saved = 0;
	extern FOLDERDATA g_folderData[];

	strcpy(str,"");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_FOLDERS;i++)
	{
		if (TestBit(*(LPBITS *)data,i) == FALSE)
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
}

static void FolderFlagsDecodeString(const char *str,void *data)
{
	char s[2000];
	extern FOLDERDATA g_folderData[];
	char *token;

	snprintf(s,sizeof(s),"%s",str);

	SetAllBits(*(LPBITS *)data,TRUE);

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;g_folderData[j].m_lpTitle != NULL;j++)
		{
			if (strcmp(g_folderData[j].short_name,token) == 0)
			{
				//dprintf("found folder to hide %i",g_folderData[j].m_nFolderId);
				ClearBit(*(LPBITS *)data,g_folderData[j].m_nFolderId);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}
}

static void TabFlagsEncodeString(void *data,char *str)
{
	int i;
	int num_saved = 0;

	strcpy(str,"");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if ((*(int *)data & (1 << i)) == 0)
		{
			if (num_saved != 0)
				strcat(str,", ");

			strcat(str,GetImageTabShortName(i));
			num_saved++;
		}
	}
}

static void TabFlagsDecodeString(const char *str,void *data)
{
	char s[2000];
	char *token;

	snprintf(s,sizeof(s),"%s",str);

	// simple way to set all tab bits "on"
	*(int *)data = (1 << MAX_TAB_TYPES) - 1;

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;j<MAX_TAB_TYPES;j++)
		{
			if (strcmp(GetImageTabShortName(j),token) == 0)
			{
				// turn off this bit
				*(int *)data &= ~(1 << j);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}

	if (*(int *)data == 0)
	{
		// not allowed to hide all tabs, because then why even show the area?
		*(int *)data = (1 << TAB_SCREENSHOT);
	}
}

static BOOL LoadGameVariableOrFolderFilter(DWORD nSettingsFile, char *key,const char *value)
{
	int i;
	int driver_index;
	const char *suffix;
	char buf[32];

	for (i = 0; i < sizeof(gamevariable_options) / sizeof(gamevariable_options[0]); i++)
	{
		snprintf(buf, sizeof(buf), "drivername_%s", gamevariable_options[i].ini_name);
		suffix = strchr(buf, '_');

		if (StringIsSuffixedBy(key, suffix))
		{
			key[strlen(key) - strlen(suffix)] = '\0';
			driver_index = GetGameNameIndex(key);
			if (driver_index < 0)
			{
				dprintf("error loading game variable for invalid game %s",key);
				return TRUE;
			}

			LoadOption(&game_variables[driver_index], &gamevariable_options[i], value);
			return TRUE;
		}
	}

	suffix = "_filters";
	if (StringIsSuffixedBy(key, suffix))
	{
		int folder_index;
		int filters;

		key[strlen(key) - strlen(suffix)] = '\0';
		if (sscanf(key,"%i",&folder_index) != 1)
		{
			dprintf("error loading game variable for invalid game %s",key);
			return TRUE;
		}
		if (folder_index < 0)
			return TRUE;

		if (sscanf(value,"%i",&filters) != 1)
			return TRUE;

		LoadFolderFilter(folder_index,filters);
		return TRUE;
	}

	return FALSE;
}

/* Register access functions below */
static void LoadOptionsAndSettings(void)
{
	struct SettingsHandler handlers[3];

	memset(handlers, 0, sizeof(handlers));

	handlers[0].type = SH_OPTIONSTRUCT;
	handlers[0].u.option_struct.option_struct = &settings;
	handlers[0].u.option_struct.option_array = regSettings;
	handlers[1].type = SH_MANUAL;
	handlers[1].u.manual.parse = LoadGameVariableOrFolderFilter;
	handlers[2].type = SH_END;
	LoadSettingsFileEx(SETTINGS_FILE_UI, handlers);

	handlers[0].type = SH_OPTIONSTRUCT;
	handlers[0].u.option_struct.option_struct = &global;
	handlers[0].u.option_struct.option_array = regGameOpts;
	handlers[1].type = SH_OPTIONSTRUCT;
	handlers[1].u.option_struct.option_struct = &settings;
	handlers[1].u.option_struct.option_array = global_game_options;
	handlers[2].type = SH_END;
	LoadSettingsFileEx(SETTINGS_FILE_GLOBAL, handlers);
}

void LoadGameOptions(int driver_index)
{
	CopyGameOptions(&global, &game_options[driver_index]);

	if (LoadSettingsFile(driver_index, &game_options[driver_index], regGameOpts))
	{
		// successfully loaded
		game_variables[driver_index].use_default = FALSE;
	}
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
	DWORD nSettingsFile;

	nSettingsFile = GetFolderSettingsFileID(folder_index);
	
	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(folder_index);
	if( redirect_index < 0)
		return;

	CopyGameOptions(&global, &folder_options[redirect_index]);
	if (!LoadSettingsFile(nSettingsFile, &folder_options[redirect_index], regGameOpts))
	{
		// uses globals
		CopyGameOptions(&global, &folder_options[redirect_index] );
	}
}

DWORD GetFolderFlags(int folder_index)
{
	int i;

	for (i=0;i<num_folder_filters;i++)
		if (folder_filters[i].folder_index == folder_index)
		{
			//dprintf("found folder filter %i %i",folder_index,folder_filters[i].filters);
			return folder_filters[i].filters;
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



static void EmitGameVariables(DWORD nSettingsFile, void (*emit_callback)(void *param_, const char *key, const char *value_str, const char *comment), void *param)
{
	int i, j;
	int driver_index;
	int nValue;
	const char *pValue;
	char key[32];
	char value_str[32];
	void *pv;

	for (i = 0; i < num_games; i++)
	{
		driver_index = GetIndexFromSortedIndex(i); 

		// need to improve this to not save too many
		for (j = 0; j < sizeof(gamevariable_options) / sizeof(gamevariable_options[0]); j++)
		{
			if (!gamevariable_options[j].m_pfnQualifier || gamevariable_options[j].m_pfnQualifier(driver_index))
			{
				snprintf(key, sizeof(key), "%s_%s", drivers[driver_index]->name, gamevariable_options[j].ini_name);
				pv = ((UINT8 *) &game_variables[driver_index]) + gamevariable_options[j].m_iDataOffset;

				switch(gamevariable_options[j].m_iType) {
				case RO_INT:
				case RO_BOOL:
					nValue = *((int *) pv);
					if (nValue != atoi(gamevariable_options[j].m_pDefaultValue))
					{
						snprintf(value_str, sizeof(value_str), "%i", nValue);
						emit_callback(param, key, value_str, NULL);
					}
					break;

				case RO_STRING:
					pValue = *((const char **) pv);
					if (!pValue)
						pValue = "";

					if (strcmp(pValue, gamevariable_options[j].m_pDefaultValue))
					{
						emit_callback(param, key, pValue, NULL);
					}
					break;

				default:
					assert(0);
					break;
				}
			}
		}
	}
}



void SaveOptions(void)
{
	int i;
	struct SettingsHandler handlers[4];

	memset(handlers, 0, sizeof(handlers));
	i = 0;

	if (save_gui_settings)
	{
		handlers[i].type = SH_OPTIONSTRUCT;
		handlers[i].comment = "interface";
		handlers[i].u.option_struct.option_struct = (void *) &settings;
		handlers[i].u.option_struct.option_array = regSettings;
		handlers[i].u.option_struct.qualifier_param = -1;
		i++;
	}

	handlers[i].type = SH_MANUAL;
	handlers[i].comment = "folder filters";
	handlers[i].u.manual.emit = EmitFolderFilters;
	i++;

	handlers[i].type = SH_MANUAL;
	handlers[i].comment = "game variables";
	handlers[i].u.manual.emit = EmitGameVariables;
	i++;

	handlers[i].type = SH_END;

	SaveSettingsFileEx(SETTINGS_FILE_UI, handlers);
}

//returns true if same
BOOL GetVectorUsesDefaults(void)
{
	int redirect_index = 0;

	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(FOLDER_VECTOR);
	if( redirect_index < 0)
		return TRUE;

	return AreOptionsEqual(regGameOpts, &folder_options[redirect_index], &global);
}

//returns true if same
BOOL GetFolderUsesDefaults(int folder_index, int driver_index)
{
	const options_type *opts;
	int redirect_index;

	if( DriverIsVector(driver_index) )
		opts = GetVectorOptions();
	else
		opts = &global;

	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(folder_index);
	if( redirect_index < 0)
		return TRUE;

	return AreOptionsEqual(regGameOpts, &folder_options[redirect_index], opts);
}

//returns true if same
BOOL GetGameUsesDefaults(int driver_index)
{
	const options_type *opts = NULL;
	int nParentIndex= -1;

	if (driver_index < 0)
	{
		dprintf("got getgameusesdefaults with driver index %i",driver_index);
		return TRUE;
	}

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

	return AreOptionsEqual(regGameOpts, &game_options[driver_index], opts);
}

void SaveGameOptions(int driver_index)
{
	options_type Opts;
	int nParentIndex= -1;
	struct SettingsHandler handlers[3];
	int setting;
#ifdef MESS
	BOOL has_software = FALSE;
#endif /* MESS */

	if( driver_index >= 0)
	{
		if( DriverIsClone(driver_index) )
		{
			nParentIndex = GetParentIndex(drivers[driver_index]);
			if( nParentIndex >= 0)
				CopyGameOptions(GetGameOptions(nParentIndex, FALSE), &Opts );
			else
				//No Parent found, use source
				CopyGameOptions(GetSourceOptions(driver_index), &Opts);
		}
		else
			CopyGameOptions( GetSourceOptions(driver_index), &Opts );
	}
	else
		CopyGameOptions( GetSourceOptions(driver_index), &Opts );

#ifdef MESS
	{
		int i;
		const options_type *o;

		o = &game_options[driver_index];

		for (i = 0; i < sizeof(o->mess.software) / sizeof(o->mess.software[0]); i++)
		{
			if (o->mess.software[i] && o->mess.software[i][0])
			{
				has_software = TRUE;
				break;
			}
		}
	}
#endif // MESS

	memset(handlers, 0, sizeof(handlers));
	setting = 0;
	handlers[setting].type = SH_OPTIONSTRUCT;
	handlers[setting].comment = "Options";
	handlers[setting].u.option_struct.option_struct = (void *) &game_options[driver_index];
	handlers[setting].u.option_struct.comparison_struct = &Opts;
	handlers[setting].u.option_struct.option_array = regGameOpts;
	setting++;
#ifdef MESS
	if (has_software)
	{
		handlers[setting].type = SH_MANUAL;
		handlers[setting].comment = "Devices";
		handlers[setting].u.manual.emit = SaveDeviceOption;
		setting++;
	}
#endif // MESS
	handlers[setting].type = SH_END;

	SaveSettingsFileEx(driver_index | SETTINGS_FILE_GAME, handlers);
}

void SaveFolderOptions(int folder_index, int game_index)
{
	DWORD nSettingsFile;
	int redirect_index = 0;
	options_type *pOpts;
	options_type Opts;

	if( DriverIsVector(game_index) && folder_index != FOLDER_VECTOR )
	{
		CopyGameOptions( GetVectorOptions(), &Opts );
		pOpts = &Opts;
	}
	else
		pOpts = &global;
	//Use the redirect array, to get correct folder
	redirect_index = GetRedirectIndex(folder_index);
	if( redirect_index < 0)
		return;

	//Save out the file
	nSettingsFile = GetFolderSettingsFileID(folder_index);
	SaveSettingsFile(nSettingsFile, &folder_options[redirect_index], pOpts, regGameOpts);
}


void SaveDefaultOptions(void)
{
	int i = 0;
	struct SettingsHandler handlers[3];

	memset(handlers, 0, sizeof(handlers));

	if (save_gui_settings)
	{
		handlers[i].type = SH_OPTIONSTRUCT;
		handlers[i].comment = "global-only options";
		handlers[i].u.option_struct.option_struct = &settings;
		handlers[i].u.option_struct.option_array = global_game_options;
		handlers[i].u.option_struct.qualifier_param = -1;
		i++;
	}

	if (save_default_options)
	{
		handlers[i].type = SH_OPTIONSTRUCT;
		handlers[i].comment = "default game options";
		handlers[i].u.option_struct.option_struct = &global;
		handlers[i].u.option_struct.option_array = regGameOpts;
		handlers[i].u.option_struct.qualifier_param = -1;
		i++;
	}

	handlers[i].type = SH_END;

	SaveSettingsFileEx(SETTINGS_FILE_GLOBAL, handlers);
}

char * GetVersionString(void)
{
	return build_version;
}


/***************************************************************************
	Consistency checking functions
 ***************************************************************************/

#ifdef MAME_DEBUG
static BOOL CheckOptions(const REG_OPTION *opts, BOOL bPassedToMAME)
{
	int nBadOptions = 0;
#if 0
	struct rc_struct *rc;
	int i;

	rc = cli_rc_create();

	for (i = 0; opts[i].ini_name[0]; i++)
	{
		// check to see that all non-'#*' options are present in the MAME core
		if ((opts[i].ini_name[0] != '#') || (opts[i].ini_name[1] != '*'))
		{
			if (bPassedToMAME && !rc_get_option(rc, opts[i].ini_name))
			{
				dprintf("CheckOptions(): Option '%s' is not represented in the MAME core\n", opts[i].ini_name);
				nBadOptions++;
			}
		}

		// some consistency checks
		switch(opts[i].m_iType) {
		case RO_STRING:
			if (!opts[i].m_pDefaultValue)
			{
				dprintf("CheckOptions(): Option '%s' needs a default value\n", opts[i].ini_name);
				nBadOptions++;
			}
			break;

		case RO_ENCODE:
			if (!opts[i].encode || !opts[i].decode)
			{
				dprintf("CheckOptions(): Option '%s' needs both an encode and a decode callback\n", opts[i].ini_name);
				nBadOptions++;
			}
			break;

		default:
			break;
		}
	}

	assert(nBadOptions == 0);

	rc_destroy(rc);
#endif
	return nBadOptions == 0;
}
#endif /* MAME_DEBUG */


/* End of options.c */
