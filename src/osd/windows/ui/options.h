/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

#ifdef MESS
#include "ui/optionsms.h"
#endif

#include "osdcomm.h"
#include "osd_cpu.h"
#include "input.h" /* for input_seq definition */
#include <video.h> /* for MAX_SCREENS Definition*/


enum 
{
	COLUMN_GAMES = 0,
	COLUMN_ROMS,
	COLUMN_SAMPLES,
	COLUMN_DIRECTORY,
	COLUMN_TYPE,
	COLUMN_TRACKBALL,
	COLUMN_PLAYED,
	COLUMN_MANUFACTURER,
	COLUMN_YEAR,
	COLUMN_CLONE,
	COLUMN_SRCDRIVERS,
	COLUMN_PLAYTIME,
	COLUMN_MAX
};


// can't be the same as the VerifyRomSet() results, listed in audit.h
enum
{
	UNKNOWN	= -1
};

/* Default input */
enum 
{
	INPUT_LAYOUT_STD,
	INPUT_LAYOUT_HR,
	INPUT_LAYOUT_HRSE
};

// clean stretch types
enum
{
	// these must match array of strings clean_stretch_long_name in options.c
	CLEAN_STRETCH_NONE = 0,
	CLEAN_STRETCH_AUTO = 1,

	MAX_CLEAN_STRETCH = 5,
};

#define FOLDER_OPTIONS	-2
#define GLOBAL_OPTIONS	-1

// d3d effect types
enum
{
	// these must match array of strings d3d_effects_long_name in options.c
	D3D_EFFECT_NONE = 0,
	D3D_EFFECT_AUTO = 1,

	MAX_D3D_EFFECTS = 17,
};

// d3d prescale types
enum
{
	D3D_PRESCALE_NONE = 0,
	D3D_PRESCALE_AUTO = 1,
	MAX_D3D_PRESCALE = 10,
};

typedef struct
{
	int x, y, width, height;
} AREA;

typedef struct
{
	char *seq_string;	/* KEYCODE_LALT KEYCODE_A, etc... */
	input_seq is;		/* sequence definition in MAME's internal keycodes */
} KeySeq;

typedef struct 
{
	char* screen;
	char* aspect;
	char* resolution;
	char* view;
} ScreenParams;


typedef struct
{
	/* video */
	char   *videomode;
	char   *effect;
	int    numscreens;
	int    prescale;
	BOOL   autoframeskip;
	int    frameskip;
	BOOL   wait_vsync;
	BOOL   use_triplebuf;
	BOOL   window_mode;
	BOOL   ddraw_stretch;
	int    gfx_refresh;
	BOOL   switchres;
	BOOL   maximize;
	BOOL   keepaspect;
	BOOL   syncrefresh;
	BOOL   throttle;
	double gfx_gamma;
	double gfx_brightness;
	double gfx_contrast;
	int    frames_to_display;
	ScreenParams screen_params[MAX_SCREENS];

	// d3d
	BOOL d3d_filter;
	int d3d_version;
	/* sound */

	/* input */
	BOOL   use_mouse;
	BOOL   use_joystick;
	double f_jdz;
	double f_jsat;
	BOOL   steadykey;
	BOOL   lightgun;
	BOOL   dual_lightgun;
	BOOL   offscreen_reload;
	char *ctrlr;
	char *digital;
	char *paddle;
	char *adstick;
	char *pedal;
	char *dial;
	char *trackball;
	char *lightgun_device;

	/* Core video */
	double f_bright_correct; /* "1.0", 0.5, 2.0 */
	double f_pause_bright; /* "0.65", 0.5, 2.0 */
	double f_contrast_correct; /* "1.0", 0.5, 2.0 */
	BOOL   rotate;
	BOOL   ror;
	BOOL   rol;
	BOOL   auto_ror;
	BOOL   auto_rol;
	BOOL   flipx;
	BOOL   flipy;
	double f_gamma_correct; /* "1.0", 0.5, 3.0 */

	/* Core vector */
	BOOL   antialias;
	BOOL   translucency;
	double f_beam;
	double f_flicker;

	/* Sound */
	int    samplerate;
	BOOL   use_samples;
	BOOL   enable_sound;
	int    attenuation;
	int    audio_latency;

	/* Misc artwork options */
	BOOL   backdrops;
	BOOL   overlays;
	BOOL   bezels;
	BOOL   artwork_crop;

	/* misc */
	BOOL   cheat;
	BOOL   mame_debug;
	BOOL   errorlog;
	BOOL   sleep;
	BOOL   old_timing;
	BOOL   leds;
	char   *ledmode;
	int    priority;
	BOOL   skip_gameinfo;
#ifdef MESS
	BOOL   skip_warnings;
#endif
	char   *bios;
	BOOL   autosave;
	BOOL   mt_render;

#ifdef MESS
	struct mess_specific_options mess;
#endif
} options_type;

// per-game data we store, not to pass to mame, but for our own use.
typedef struct
{
    int play_count;
	int play_time;
    int rom_audit_results;
    int samples_audit_results;

    BOOL options_loaded; // whether or not we've loaded the game options yet
    BOOL use_default; // whether or not we should just use default options

#ifdef MESS
    struct mess_specific_game_variables mess;
#endif
} game_variables_type;

// List of artwork types to display in the screen shot area
enum
{
	// these must match array of strings image_tabs_long_name in options.c
	// if you add new Tabs, be sure to also add them to the ComboBox init in dialogs.c
	TAB_SCREENSHOT = 0,
	TAB_FLYER,
	TAB_CABINET,
	TAB_MARQUEE,
	TAB_TITLE,
	TAB_CONTROL_PANEL,
	TAB_HISTORY,

	MAX_TAB_TYPES,
	BACKGROUND,
	TAB_ALL,
	TAB_NONE
};
// Because we have added the Options after MAX_TAB_TYPES, we have to subtract 3 here
// (that's how many options we have after MAX_TAB_TYPES)
#define TAB_SUBTRACT 3


typedef struct
{
    INT      folder_id;
    BOOL     view;
    BOOL     show_folderlist;
	LPBITS   show_folder_flags;
    BOOL     show_toolbar;
    BOOL     show_statusbar;
    BOOL     show_screenshot;
    BOOL     show_tabctrl;
	int      show_tab_flags;
	int      history_tab;
    char     *current_tab;
    BOOL     game_check;        /* Startup GameCheck */
    BOOL     use_joygui;
	BOOL     use_keygui;
    BOOL     broadcast;
    BOOL     random_bg;
    int      cycle_screenshot;
	BOOL     stretch_screenshot_larger;
    int      screenshot_bordersize;
    COLORREF screenshot_bordercolor;
	BOOL     inherit_filter;
	BOOL     offset_clones;
	BOOL	 game_caption;

    char     *default_game;
    int      column_width[COLUMN_MAX];
    int      column_order[COLUMN_MAX];
    int      column_shown[COLUMN_MAX];
    int      sort_column;
    BOOL     sort_reverse;
    AREA     area;
    UINT     windowstate;
    int      splitter[4];		/* NPW 5-Feb-2003 - I don't like hard coding this, but I don't have a choice */
    COLORREF custom_color[16]; /* This is how many custom colors can be shown on the standard ColorPicker */
    LOGFONT  list_font;
    COLORREF list_font_color;
    COLORREF list_clone_color;
    BOOL     skip_gameinfo;
#ifdef MESS
	BOOL     skip_warnings;
#endif
    int      priority;
    BOOL     autosave;
    BOOL     mt_render;

	// Keyboard control of ui
    KeySeq   ui_key_up;
    KeySeq   ui_key_down;
    KeySeq   ui_key_left;
    KeySeq   ui_key_right;
    KeySeq   ui_key_start;
    KeySeq   ui_key_pgup;
    KeySeq   ui_key_pgdwn;
    KeySeq   ui_key_home;
    KeySeq   ui_key_end;
    KeySeq   ui_key_ss_change;
    KeySeq   ui_key_history_up;
    KeySeq   ui_key_history_down;

    KeySeq   ui_key_context_filters;	/* CTRL F */
    KeySeq   ui_key_select_random;		/* CTRL R */
    KeySeq   ui_key_game_audit;			/* ALT A */
    KeySeq   ui_key_game_properties;	/* ALT VK_RETURN */
    KeySeq   ui_key_help_contents;		/* VK_F1 */
    KeySeq   ui_key_update_gamelist;	/* VK_F5 */
    KeySeq   ui_key_view_folders;		/* ALT D */
    KeySeq   ui_key_view_fullscreen;	/* VK_F11 */
    KeySeq   ui_key_view_pagetab;		/* ALT B */
    KeySeq   ui_key_view_picture_area;	/* ALT P */
    KeySeq   ui_key_view_status;		/* ALT S */
    KeySeq   ui_key_view_toolbars;		/* ALT T */

    KeySeq   ui_key_view_tab_cabinet;	/* ALT 3 */
    KeySeq   ui_key_view_tab_cpanel;	/* ALT 6 */
    KeySeq   ui_key_view_tab_flyer;		/* ALT 2 */
    KeySeq   ui_key_view_tab_history;	/* ALT 7 */
    KeySeq   ui_key_view_tab_marquee;	/* ALT 4 */
    KeySeq   ui_key_view_tab_screenshot;/* ALT 1 */
    KeySeq   ui_key_view_tab_title;		/* ALT 5 */
    KeySeq   ui_key_quit;				/* ALT Q */

    // Joystick control of ui
	// array of 4 is joystick index, stick or button, etc.
    int      ui_joy_up[4];
    int      ui_joy_down[4];
    int      ui_joy_left[4];
    int      ui_joy_right[4];
    int      ui_joy_start[4];
    int      ui_joy_pgup[4];
    int      ui_joy_pgdwn[4];
    int      ui_joy_home[4];
    int      ui_joy_end[4];
    int      ui_joy_ss_change[4];
    int      ui_joy_history_up[4];
    int      ui_joy_history_down[4];
    int      ui_joy_exec[4];

    char*    exec_command;  // Command line to execute on ui_joy_exec   
    int      exec_wait;     // How long to wait before executing
    BOOL     hide_mouse;    // Should mouse cursor be hidden on startup?
    BOOL     full_screen;   // Should we fake fullscreen?

    char *language;
    char *flyerdir;
    char *cabinetdir;
    char *marqueedir;
    char *titlesdir;
    char *cpaneldir;

    char*    romdirs;
    char*    sampledirs;
    char*    inidir;
    char*    cfgdir;
    char*    nvramdir;
    char*    memcarddir;
    char*    inpdir;
    char*    statedir;
    char*    artdir;
    char*    imgdir;
    char*    diffdir;
    char*	 iconsdir;
    char*    bgdir;
    char*    cheat_filename;
    char*    history_filename;
    char*    mameinfo_filename;
    char*    ctrlrdir;
    char*    folderdir;
    char*    commentdir;

#ifdef MESS
    struct mess_specific_settings mess;
#endif

} settings_type; /* global settings for the UI only */

BOOL OptionsInit(void);
void FolderOptionsInit(void);
void OptionsExit(void);

void FreeGameOptions(options_type *o);
void CopyGameOptions(const options_type *source,options_type *dest);
void SyncInFolderOptions(options_type *opts, int folder_index);
options_type * GetDefaultOptions(int iProperty, BOOL bVectorFolder);
options_type * GetFolderOptions(int folder_index, BOOL bIsVector);

int * GetFolderOptionsRedirectArr(void);
void SetFolderOptionsRedirectArr(int *redirect_arr);
int GetRedirectIndex(int folder_index);
int GetRedirectValue(int index);

options_type * GetVectorOptions(void);
options_type * GetSourceOptions(int driver_index );
options_type * GetGameOptions(int driver_index, int folder_index );
BOOL GetVectorUsesDefaults(void);
BOOL GetFolderUsesDefaults(int folder_index, int driver_index);
BOOL GetGameUsesDefaults(int driver_index);
BOOL GetGameUsesDefaults(int driver_index);
void SetGameUsesDefaults(int driver_index,BOOL use_defaults);
void LoadGameOptions(int driver_index);
void LoadFolderOptions(int folder_index);

const char* GetFolderNameByID(UINT nID);

void SaveOptions(void);
void SaveFolderOptions(int folder_index, int game_index);

void ResetGUI(void);
void ResetGameDefaults(void);
void ResetAllGameOptions(void);

const char * GetImageTabLongName(int tab_index);
const char * GetImageTabShortName(int tab_index);

void SetViewMode(int val);
int  GetViewMode(void);

void SetGameCheck(BOOL game_check);
BOOL GetGameCheck(void);

void SetVersionCheck(BOOL version_check);
BOOL GetVersionCheck(void);

void SetJoyGUI(BOOL use_joygui);
BOOL GetJoyGUI(void);

void SetKeyGUI(BOOL use_keygui);
BOOL GetKeyGUI(void);

void SetCycleScreenshot(int cycle_screenshot);
int GetCycleScreenshot(void);

void SetStretchScreenShotLarger(BOOL stretch);
BOOL GetStretchScreenShotLarger(void);

void SetScreenshotBorderSize(int size);
int GetScreenshotBorderSize(void);

void SetScreenshotBorderColor(COLORREF uColor);
COLORREF GetScreenshotBorderColor(void);

void SetFilterInherit(BOOL inherit);
BOOL GetFilterInherit(void);

void SetOffsetClones(BOOL offset);
BOOL GetOffsetClones(void);

void SetGameCaption(BOOL caption);
BOOL GetGameCaption(void);

void SetBroadcast(BOOL broadcast);
BOOL GetBroadcast(void);

void SetRandomBackground(BOOL random_bg);
BOOL GetRandomBackground(void);

void SetSavedFolderID(UINT val);
UINT GetSavedFolderID(void);

void SetShowScreenShot(BOOL val);
BOOL GetShowScreenShot(void);

void SetShowFolderList(BOOL val);
BOOL GetShowFolderList(void);

BOOL GetShowFolder(int folder);
void SetShowFolder(int folder,BOOL show);


void SetShowStatusBar(BOOL val);
BOOL GetShowStatusBar(void);

void SetShowToolBar(BOOL val);
BOOL GetShowToolBar(void);

void SetShowTabCtrl(BOOL val);
BOOL GetShowTabCtrl(void);

void SetCurrentTab(const char *shortname);
const char *GetCurrentTab(void);

void SetDefaultGame(const char *name);
const char *GetDefaultGame(void);

void SetWindowArea(AREA *area);
void GetWindowArea(AREA *area);

void SetWindowState(UINT state);
UINT GetWindowState(void);

void SetColumnWidths(int widths[]);
void GetColumnWidths(int widths[]);

void SetColumnOrder(int order[]);
void GetColumnOrder(int order[]);

void SetColumnShown(int shown[]);
void GetColumnShown(int shown[]);

void SetSplitterPos(int splitterId, int pos);
int  GetSplitterPos(int splitterId);

void SetCustomColor(int iIndex, COLORREF uColor);
COLORREF GetCustomColor(int iIndex);

void SetListFont(LOGFONT *font);
void GetListFont(LOGFONT *font);

DWORD GetFolderFlags(int folder_index);

void SetListFontColor(COLORREF uColor);
COLORREF GetListFontColor(void);

void SetListCloneColor(COLORREF uColor);
COLORREF GetListCloneColor(void);

int GetHistoryTab(void);
void SetHistoryTab(int tab,BOOL show);

int GetShowTab(int tab);
void SetShowTab(int tab,BOOL show);
BOOL AllowedToSetShowTab(int tab,BOOL show);

void SetSortColumn(int column);
int  GetSortColumn(void);

void SetSortReverse(BOOL reverse);
BOOL GetSortReverse(void);

const char* GetLanguage(void);
void SetLanguage(const char* lang);

const char* GetRomDirs(void);
void SetRomDirs(const char* paths);

const char* GetSampleDirs(void);
void  SetSampleDirs(const char* paths);

const char * GetIniDir(void);
void SetIniDir(const char *path);

const char* GetCfgDir(void);
void SetCfgDir(const char* path);

const char* GetNvramDir(void);
void SetNvramDir(const char* path);

const char* GetInpDir(void);
void SetInpDir(const char* path);

const char* GetImgDir(void);
void SetImgDir(const char* path);

const char* GetStateDir(void);
void SetStateDir(const char* path);

const char* GetArtDir(void);
void SetArtDir(const char* path);

const char* GetMemcardDir(void);
void SetMemcardDir(const char* path);

const char* GetFlyerDir(void);
void SetFlyerDir(const char* path);

const char* GetCabinetDir(void);
void SetCabinetDir(const char* path);

const char* GetMarqueeDir(void);
void SetMarqueeDir(const char* path);

const char* GetTitlesDir(void);
void SetTitlesDir(const char* path);

const char * GetControlPanelDir(void);
void SetControlPanelDir(const char *path);

const char* GetDiffDir(void);
void SetDiffDir(const char* path);

const char* GetIconsDir(void);
void SetIconsDir(const char* path);

const char *GetBgDir(void);
void SetBgDir(const char *path);

const char* GetCtrlrDir(void);
void SetCtrlrDir(const char* path);

const char* GetCommentDir(void);
void SetCommentDir(const char* path);

const char* GetFolderDir(void);
void SetFolderDir(const char* path);

const char* GetCheatFileName(void);
void SetCheatFileName(const char* path);

const char* GetHistoryFileName(void);
void SetHistoryFileName(const char* path);

const char* GetMAMEInfoFileName(void);
void SetMAMEInfoFileName(const char* path);

void ResetGameOptions(int driver_index);

int GetRomAuditResults(int driver_index);
void SetRomAuditResults(int driver_index, int audit_results);

int GetSampleAuditResults(int driver_index);
void SetSampleAuditResults(int driver_index, int audit_results);

void IncrementPlayCount(int driver_index);
int GetPlayCount(int driver_index);
void ResetPlayCount(int driver_index);

void IncrementPlayTime(int driver_index,int playtime);
int GetPlayTime(int driver_index);
void GetTextPlayTime(int driver_index,char *buf);
void ResetPlayTime(int driver_index);

char * GetVersionString(void);

void SaveGameOptions(int driver_index);
void SaveDefaultOptions(void);


// Keyboard control of ui
input_seq* Get_ui_key_up(void);
input_seq* Get_ui_key_down(void);
input_seq* Get_ui_key_left(void);
input_seq* Get_ui_key_right(void);
input_seq* Get_ui_key_start(void);
input_seq* Get_ui_key_pgup(void);
input_seq* Get_ui_key_pgdwn(void);
input_seq* Get_ui_key_home(void);
input_seq* Get_ui_key_end(void);
input_seq* Get_ui_key_ss_change(void);
input_seq* Get_ui_key_history_up(void);
input_seq* Get_ui_key_history_down(void);

input_seq* Get_ui_key_context_filters(void);
input_seq* Get_ui_key_select_random(void);
input_seq* Get_ui_key_game_audit(void);
input_seq* Get_ui_key_game_properties(void);
input_seq* Get_ui_key_help_contents(void);
input_seq* Get_ui_key_update_gamelist(void);
input_seq* Get_ui_key_view_folders(void);
input_seq* Get_ui_key_view_fullscreen(void);
input_seq* Get_ui_key_view_pagetab(void);
input_seq* Get_ui_key_view_picture_area(void);
input_seq* Get_ui_key_view_status(void);
input_seq* Get_ui_key_view_toolbars(void);

input_seq* Get_ui_key_view_tab_cabinet(void);
input_seq* Get_ui_key_view_tab_cpanel(void);
input_seq* Get_ui_key_view_tab_flyer(void);
input_seq* Get_ui_key_view_tab_history(void);
input_seq* Get_ui_key_view_tab_marquee(void);
input_seq* Get_ui_key_view_tab_screenshot(void);
input_seq* Get_ui_key_view_tab_title(void);
input_seq* Get_ui_key_quit(void);


int GetUIJoyUp(int joycodeIndex);
void SetUIJoyUp(int joycodeIndex, int val);

int GetUIJoyDown(int joycodeIndex);
void SetUIJoyDown(int joycodeIndex, int val);

int GetUIJoyLeft(int joycodeIndex);
void SetUIJoyLeft(int joycodeIndex, int val);

int GetUIJoyRight(int joycodeIndex);
void SetUIJoyRight(int joycodeIndex, int val);

int GetUIJoyStart(int joycodeIndex);
void SetUIJoyStart(int joycodeIndex, int val);

int GetUIJoyPageUp(int joycodeIndex);
void SetUIJoyPageUp(int joycodeIndex, int val);

int GetUIJoyPageDown(int joycodeIndex);
void SetUIJoyPageDown(int joycodeIndex, int val);

int GetUIJoyHome(int joycodeIndex);
void SetUIJoyHome(int joycodeIndex, int val);

int GetUIJoyEnd(int joycodeIndex);
void SetUIJoyEnd(int joycodeIndex, int val);

int GetUIJoySSChange(int joycodeIndex);
void SetUIJoySSChange(int joycodeIndex, int val);

int GetUIJoyHistoryUp(int joycodeIndex);
void SetUIJoyHistoryUp(int joycodeIndex, int val);

int GetUIJoyHistoryDown(int joycodeIndex);
void SetUIJoyHistoryDown(int joycodeIndex, int val);

int GetUIJoyExec(int joycodeIndex);
void SetUIJoyExec(int joycodeIndex, int val);

char* GetExecCommand(void);
void SetExecCommand(char* cmd);

int GetExecWait(void);
void SetExecWait(int wait);

BOOL GetHideMouseOnStartup(void);
void SetHideMouseOnStartup(BOOL hide);

BOOL GetRunFullScreen(void);
void SetRunFullScreen(BOOL fullScreen);

#endif
