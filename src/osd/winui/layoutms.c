/***************************************************************************

  layoutms.c

  MESS specific TreeView definitions (and maybe more in the future)

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdio.h>  /* for sprintf */
#include <stdlib.h> /* For malloc and free */
#include <string.h>

#include "emu.h"
#include "bitmask.h"
#include "TreeView.h"
#include "mui_util.h"
#include "resource.h"
#include "directories.h"
#include "mui_opts.h"
#include "splitters.h"
#include "help.h"
#include "mui_audit.h"
#include "screenshot.h"
#include "winui.h"
#include "properties.h"
#include "optionsms.h"

#include "msuiutil.h"
#include "resourcems.h"
#include "propertiesms.h"

static BOOL FilterAvailable(int driver_index);

extern const FOLDERDATA g_folderData[] =
{
	{"All Systems",     "allgames",          FOLDER_ALLGAMES,     IDI_FOLDER,				0,             0,            NULL,                       NULL,              TRUE },
	{"Available",       "available",         FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   0,            NULL,                       FilterAvailable,              TRUE },
#ifdef SHOW_UNAVAILABLE_FOLDER
	{"Unavailable",     "unavailable",       FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,	0,             F_AVAILABLE,  NULL,                       FilterAvailable,              FALSE },
#endif
	{"Console",         "console",           FOLDER_CONSOLE,      IDI_FOLDER,             F_CONSOLE,     F_COMPUTER,   NULL,                       DriverIsConsole,  TRUE },
	{"Computer",        "computer",          FOLDER_COMPUTER,     IDI_FOLDER,             F_COMPUTER,    F_CONSOLE,    NULL,                       DriverIsComputer,  TRUE },
	{"Modified/Hacked", "modified",          FOLDER_MODIFIED,     IDI_FOLDER,				0,             0,            NULL,                       DriverIsModified,  TRUE },
	{"Manufacturer",    "manufacturer",      FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,            CreateManufacturerFolders },
	{"Year",            "year",              FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,            CreateYearFolders },
	{"Source",          "source",            FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,            CreateSourceFolders },
	{"CPU",             "cpu",               FOLDER_CPU,          IDI_FOLDER,               0,             0,            CreateCPUFolders },
	{"Sound",           "sound",             FOLDER_SND,          IDI_FOLDER,               0,             0,            CreateSoundFolders },
	{"Imperfect",       "imperfect",         FOLDER_DEFICIENCY,   IDI_FOLDER,               0,             0,            CreateDeficiencyFolders },
	{"Dumping Status",  "dumping",           FOLDER_DUMPING,      IDI_FOLDER,               0,             0,            CreateDumpingFolders },
	{"Working",         "working",           FOLDER_WORKING,      IDI_WORKING,              F_WORKING,     F_NONWORKING, NULL,                       DriverIsBroken,    FALSE },
	{"Not Working",     "nonworking",        FOLDER_NONWORKING,   IDI_NONWORKING,           F_NONWORKING,  F_WORKING,    NULL,                       DriverIsBroken,    TRUE },
	{"Originals",       "originals",         FOLDER_ORIGINAL,     IDI_FOLDER,               F_ORIGINALS,   F_CLONES,     NULL,                       DriverIsClone,     FALSE },
	{"Clones",          "clones",            FOLDER_CLONES,       IDI_FOLDER,               F_CLONES,      F_ORIGINALS,  NULL,                       DriverIsClone,     TRUE },
	{"Raster",          "raster",            FOLDER_RASTER,       IDI_FOLDER,               F_RASTER,      F_VECTOR,     NULL,                       DriverIsVector,    FALSE },
	{"Vector",          "vector",            FOLDER_VECTOR,       IDI_FOLDER,               F_VECTOR,      F_RASTER,     NULL,                       DriverIsVector,    TRUE },
	{"Mouse",           "mouse",             FOLDER_MOUSE,        IDI_FOLDER,               0,             0,            NULL,                       DriverUsesMouse,	TRUE },
	{"Trackball",       "trackball",         FOLDER_TRACKBALL,    IDI_FOLDER,               0,             0,            NULL,                       DriverUsesTrackball,	TRUE },
	{"Stereo",          "stereo",            FOLDER_STEREO,       IDI_SOUND,                0,             0,            NULL,                       DriverIsStereo,    TRUE },
	{ NULL }
};

/* list of filter/control Id pairs */
extern const FILTER_ITEM g_filterList[] =
{
	{ F_COMPUTER,     IDC_FILTER_COMPUTER,    DriverIsComputer, TRUE },
	{ F_CONSOLE,      IDC_FILTER_CONSOLE,     DriverIsConsole, TRUE },
	{ F_MODIFIED,     IDC_FILTER_MODIFIED,    DriverIsModified, TRUE },
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone, TRUE },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken, TRUE },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, FilterAvailable, FALSE },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector, FALSE },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector, TRUE },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone, FALSE },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken, FALSE },
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   FilterAvailable, TRUE },
	{ 0 }
};

extern const DIRECTORYINFO g_directoryInfo[] =
{
	{ "ROMs",                  GetRomDirs,         SetRomDirs,         TRUE,  DIRDLG_ROMS },
	{ "Software",              GetSoftwareDirs,    SetSoftwareDirs,    TRUE,  DIRDLG_SOFTWARE },
	{ "Artwork",               GetArtDir,          SetArtDir,          FALSE, 0 },
	{ "Background Images",     GetBgDir,           SetBgDir,           FALSE, 0 },
	{ "Cabinets",              GetCabinetDir,      SetCabinetDir,      FALSE, 0 },
	{ "Cheats",		   GetCheatDir,        SetCheatDir,        FALSE, DIRDLG_CHEAT },
	{ "Comment Files",	   GetCommentDir,      SetCommentDir,      FALSE, DIRDLG_COMMENT },
	{ "Config",                GetCfgDir,          SetCfgDir,          FALSE, DIRDLG_CFG },
	{ "Control Panels",        GetControlPanelDir, SetControlPanelDir, FALSE, 0 },
	{ "Controller Files",      GetCtrlrDir,        SetCtrlrDir,        FALSE, DIRDLG_CTRLR },
	{ "Crosshairs",		   GetCrosshairDir,    SetCrosshairDir,    FALSE, 0 },
	{ "Fonts",                 GetFontDir,         SetFontDir,         FALSE, 0 },
	{ "Flyers",                GetFlyerDir,        SetFlyerDir,        FALSE, 0 },
	{ "Hash",                  GetHashDirs,        SetHashDirs,        FALSE, 0 },
	{ "Hard Drive Difference", GetDiffDir,         SetDiffDir,         FALSE, 0 },
	{ "Icons",                 GetIconsDir,        SetIconsDir,        FALSE, 0 },
	{ "Ini Files",             GetIniDir,          SetIniDir,          FALSE, DIRDLG_INI },
	{ "Input files",           GetInpDir,          SetInpDir,          FALSE, DIRDLG_INP },
	{ "Marquees",              GetMarqueeDir,      SetMarqueeDir,      FALSE, 0 },
	{ "Memory Card",           GetMemcardDir,      SetMemcardDir,      FALSE, 0 },
	{ "NVRAM",                 GetNvramDir,        SetNvramDir,        FALSE, 0 },
	{ "PCBs",                  GetPcbDir,          SetPcbDir,          FALSE, 0 },
	{ "Snapshots",             GetImgDir,          SetImgDir,          FALSE, DIRDLG_IMG },
	{ "State",                 GetStateDir,        SetStateDir,        FALSE, 0 },
	{ "Titles",                GetTitlesDir,       SetTitlesDir,       FALSE, 0 },
	{ NULL }
};

extern const SPLITTERINFO g_splitterInfo[] =
{
	{ 0.2,	IDC_SPLITTER,	IDC_TREE,	IDC_LIST,		AdjustSplitter1Rect },
	{ 0.4,	IDC_SPLITTER2,	IDC_LIST,	IDC_SWLIST,		AdjustSplitter1Rect },
	{ 0.6,	IDC_SPLITTER3,	IDC_SWTAB,	IDC_SSFRAME,	AdjustSplitter2Rect },
	{ -1 }
};

extern const MAMEHELPINFO g_helpInfo[] =
{
	{ ID_HELP_CONTENTS,		TRUE,	"mess.chm::/windows/main.htm" },
	{ ID_HELP_RELEASE,		TRUE,	"mess.chm" },
	{ ID_HELP_WHATS_NEW,	TRUE,	"mess.chm::/messnew.txt" },
	{ -1 }
};

extern const PROPERTYSHEETINFO g_propSheets[] =
{
	{ FALSE,	NULL,					IDD_PROP_GAME,			GamePropertiesDialogProc },
	{ FALSE,	NULL,					IDD_PROP_AUDIT,			GameAuditDialogProc },
	{ TRUE,		NULL,					IDD_PROP_DISPLAY,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_ADVANCED,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_SCREEN,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_SOUND,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_INPUT,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_CONTROLLER,	GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_DEBUG,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_MISC,			GameOptionsProc },
	{ FALSE,	NULL,					IDD_PROP_SOFTWARE,		GameMessOptionsProc },
	{ FALSE,	PropSheetFilter_Config,	IDD_PROP_CONFIGURATION,	GameMessOptionsProc },
	{ TRUE,		PropSheetFilter_Vector,	IDD_PROP_VECTOR,		GameOptionsProc },
	{ FALSE }
};

extern const ICONDATA g_iconData[] =
{
	{ IDI_WIN_NOROMS,			"noroms" },
	{ IDI_WIN_ROMS,				"roms" },
	{ IDI_WIN_UNKNOWN,			"unknown" },
	{ IDI_WIN_CLONE,			"clone" },
	{ IDI_WIN_REDX,				"warning" },
	{ IDI_WIN_NOROMSNEEDED,		"noromsneeded" },
	{ IDI_WIN_MISSINGOPTROM,	"missingoptrom" },
	{ IDI_WIN_FLOP,				"floppy" },
	{ IDI_WIN_CASS,				"cassette" },
	{ IDI_WIN_SERL,				"serial" },
	{ IDI_WIN_SNAP,				"snapshot" },
	{ IDI_WIN_PRIN,				"printer" },
	{ IDI_WIN_HARD,				"hard" },
	{ 0 }
};

extern const TCHAR g_szPlayGameString[] = TEXT("&Run %s");
extern const char g_szGameCountString[] = "%d systems";
static const char g_szHistoryFileName[] = "sysinfo.dat";
static const char g_szMameInfoFileName[] = "messinfo.dat";

static BOOL FilterAvailable(int driver_index)
{
	return !DriverUsesRoms(driver_index) || IsAuditResultYes(GetRomAuditResults(driver_index));
}
