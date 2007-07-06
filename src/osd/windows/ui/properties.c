/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/
 
/***************************************************************************

  Properties.c

    Properties Popup and Misc UI support routines.
    
    Created 8/29/98 by Mike Haaland (mhaaland@hypertech.com)

***************************************************************************/
#define WIN32_LEAN_AND_MEAN
#define NONAMELESSUNION 1
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#undef NONAMELESSUNION
#include <ddraw.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <tchar.h>

#include <driver.h>
#include <info.h>
#include "audit.h"
#include "audit32.h"
#include "bitmask.h"
#include "m32opts.h"
#include "file.h"
#include "resource.h"
#include "DIJoystick.h"     /* For DIJoystick avalibility. */
#include "m32util.h"
#include "directdraw.h"
#include "properties.h"
#include "treeview.h"
#include "win32ui.h"
#include "screenshot.h"
#include "Mame32.h"
#include "DataMap.h"
#include "help.h"
#include "resource.hm"
#include "winmain.h"
#include "strconv.h"
#include "winutf8.h"
#include "ui/m32util.h"

typedef HANDLE HTHEME;

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef MESS
// done like this until I figure out a better idea
#include "messopts.h"
#include "ui/resourcems.h"
#include "ui/propertiesms.h"
#endif

// missing win32 api defines
#ifndef TBCD_TICS
#define TBCD_TICS 1
#endif
#ifndef TBCD_THUMB
#define TBCD_THUMB 2
#endif
#ifndef TBCD_CHANNEL
#define TBCD_CHANNEL 3
#endif

#if defined(__GNUC__)
/* fix warning: cast does not match function type */
#undef  PropSheet_GetTabControl
#define PropSheet_GetTabControl(d) (HWND)(LRESULT)(int)SendMessage((d),PSM_GETTABCONTROL,0,0)
#endif /* defined(__GNUC__) */

/***************************************************************
 * Imported function prototypes
 ***************************************************************/

/**************************************************************
 * Local function prototypes
 **************************************************************/

static void SetStereoEnabled(HWND hWnd, int nIndex);
static void SetYM3812Enabled(HWND hWnd, int nIndex);
static void SetSamplesEnabled(HWND hWnd, int nIndex, BOOL bSoundEnabled);
static void InitializeOptions(HWND hDlg);
static void InitializeMisc(HWND hDlg);
static void OptOnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);
static void NumScreensSelectionChange(HWND hwnd);
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl);
static void InitializeSoundUI(HWND hwnd);
static void InitializeSkippingUI(HWND hwnd);
static void InitializeRotateUI(HWND hwnd);
static void UpdateSelectScreenUI(HWND hwnd);
static void InitializeSelectScreenUI(HWND hwnd);
static void InitializeD3DVersionUI(HWND hwnd);
static void InitializeVideoUI(HWND hwnd);
static void InitializeAnalogAxesUI(HWND hWnd);
static void InitializeEffectUI(HWND hWnd);
static void InitializeBIOSUI(HWND hwnd);
static void InitializeControllerMappingUI(HWND hwnd);
static void PropToOptions(HWND hWnd, core_options *o);
static void OptionsToProp(HWND hWnd, core_options *o);
static void SetPropEnabledControls(HWND hWnd);
static BOOL SelectEffect(HWND hWnd);
static BOOL ResetEffect(HWND hWnd);

static void BuildDataMap(void);
static void ResetDataMap(HWND hWnd);

static BOOL IsControlOptionValue(HWND hDlg,HWND hwnd_ctrl, core_options *opts );

static void UpdateBackgroundBrush(HWND hwndTab);
HBRUSH hBkBrush;

/**************************************************************
 * Local private variables
 **************************************************************/

static core_options *origGameOpts;
static BOOL orig_uses_defaults;
static core_options *pGameOpts = NULL;
static datamap *properties_datamap;

static int  g_nGame            = 0;
static int  g_nFolder          = 0;
static int  g_nFolderGame      = 0;
static int  g_nPropertyMode    = 0;
static BOOL g_bUseDefaults     = FALSE;
static BOOL g_bReset           = FALSE;
static BOOL  g_bAutoAspect[MAX_SCREENS] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
static BOOL g_bAnalogCheckState[65]; // 8 Joysticks  * 8 Axes each
static HICON g_hIcon = NULL;

/* Property sheets */

#define HIGHLIGHT_COLOR RGB(0,196,0)
HBRUSH highlight_brush = NULL;
HBRUSH background_brush = NULL;
#define VECTOR_COLOR RGB( 190, 0, 0) //DARK RED
#define FOLDER_COLOR RGB( 0, 128, 0 ) // DARK GREEN
#define PARENT_COLOR RGB( 190, 128, 192 ) // PURPLE
#define GAME_COLOR RGB( 0, 128, 192 ) // DARK BLUE

BOOL PropSheetFilter_Vector(const machine_config *drv, const game_driver *gamedrv)
{
	return (drv->video_attributes & VIDEO_TYPE_VECTOR) != 0;
}

/* Help IDs */
static DWORD dwHelpIDs[] =
{
	
	IDC_JDZ,				HIDC_JDZ,
	IDC_ANTIALIAS,          HIDC_ANTIALIAS,
	IDC_ARTWORK_CROP,		HIDC_ARTWORK_CROP,
	IDC_ASPECTRATIOD,       HIDC_ASPECTRATIOD,
	IDC_ASPECTRATION,       HIDC_ASPECTRATION,
	IDC_AUTOFRAMESKIP,      HIDC_AUTOFRAMESKIP,
	IDC_BACKDROPS,			HIDC_BACKDROPS,
	IDC_BEAM,               HIDC_BEAM,
	IDC_BEZELS,				HIDC_BEZELS,
	IDC_FSGAMMA,		    HIDC_FSGAMMA,
	IDC_BRIGHTCORRECT,      HIDC_BRIGHTCORRECT,
	IDC_BROADCAST,			HIDC_BROADCAST,
	IDC_RANDOM_BG,          HIDC_RANDOM_BG,
	IDC_CHEAT,              HIDC_CHEAT,
	IDC_DEFAULT_INPUT,      HIDC_DEFAULT_INPUT,
	IDC_FILTER_CLONES,      HIDC_FILTER_CLONES,
	IDC_FILTER_EDIT,        HIDC_FILTER_EDIT,
	IDC_FILTER_NONWORKING,  HIDC_FILTER_NONWORKING,
	IDC_FILTER_ORIGINALS,   HIDC_FILTER_ORIGINALS,
	IDC_FILTER_RASTER,      HIDC_FILTER_RASTER,
	IDC_FILTER_UNAVAILABLE, HIDC_FILTER_UNAVAILABLE,
	IDC_FILTER_VECTOR,      HIDC_FILTER_VECTOR,
	IDC_FILTER_WORKING,     HIDC_FILTER_WORKING,
	IDC_FLICKER,            HIDC_FLICKER,
	IDC_FLIPX,              HIDC_FLIPX,
	IDC_FLIPY,              HIDC_FLIPY,
	IDC_FRAMESKIP,          HIDC_FRAMESKIP,
	IDC_GAMMA,              HIDC_GAMMA,
	IDC_HISTORY,            HIDC_HISTORY,
	IDC_HWSTRETCH,          HIDC_HWSTRETCH,
	IDC_JOYSTICK,           HIDC_JOYSTICK,
	IDC_KEEPASPECT,         HIDC_KEEPASPECT,
	IDC_LANGUAGECHECK,      HIDC_LANGUAGECHECK,
	IDC_LANGUAGEEDIT,       HIDC_LANGUAGEEDIT,
	IDC_LOG,                HIDC_LOG,
	IDC_SLEEP,				HIDC_SLEEP,
	IDC_MAXIMIZE,           HIDC_MAXIMIZE,
	IDC_OVERLAYS,			HIDC_OVERLAYS,
	IDC_PROP_RESET,         HIDC_PROP_RESET,
	IDC_REFRESH,            HIDC_REFRESH,
	IDC_RESET_DEFAULT,      HIDC_RESET_DEFAULT,
	IDC_RESET_FILTERS,      HIDC_RESET_FILTERS,
	IDC_RESET_GAMES,        HIDC_RESET_GAMES,
	IDC_RESET_UI,           HIDC_RESET_UI,
	IDC_ROTATE,             HIDC_ROTATE,
	IDC_SAMPLERATE,         HIDC_SAMPLERATE,
	IDC_SAMPLES,            HIDC_SAMPLES,
	IDC_SIZES,              HIDC_SIZES,
	IDC_START_GAME_CHECK,   HIDC_START_GAME_CHECK,
	IDC_SWITCHRES,          HIDC_SWITCHRES,
	IDC_SYNCREFRESH,        HIDC_SYNCREFRESH,
	IDC_THROTTLE,           HIDC_THROTTLE,
	IDC_TRIPLE_BUFFER,      HIDC_TRIPLE_BUFFER,
	IDC_USE_DEFAULT,        HIDC_USE_DEFAULT,
	IDC_USE_MOUSE,          HIDC_USE_MOUSE,
	IDC_USE_SOUND,          HIDC_USE_SOUND,
	IDC_VOLUME,             HIDC_VOLUME,
	IDC_WAITVSYNC,          HIDC_WAITVSYNC,
	IDC_WINDOWED,           HIDC_WINDOWED,
	IDC_PAUSEBRIGHT,        HIDC_PAUSEBRIGHT,
	IDC_LIGHTGUN,           HIDC_LIGHTGUN,
	IDC_STEADYKEY,          HIDC_STEADYKEY,
	IDC_JOY_GUI,            HIDC_JOY_GUI,
	IDC_RANDOM_BG,          HIDC_RANDOM_BG,
	IDC_SKIP_GAME_INFO,     HIDC_SKIP_GAME_INFO,
	IDC_HIGH_PRIORITY,      HIDC_HIGH_PRIORITY,
	IDC_D3D_FILTER,         HIDC_D3D_FILTER,
	IDC_AUDIO_LATENCY,      HIDC_AUDIO_LATENCY,
	IDC_BIOS,               HIDC_BIOS,
	IDC_STRETCH_SCREENSHOT_LARGER, HIDC_STRETCH_SCREENSHOT_LARGER,
	IDC_SCREEN,             HIDC_SCREEN,
	IDC_ANALOG_AXES,		HIDC_ANALOG_AXES,
	IDC_PADDLE,				HIDC_PADDLE,
	IDC_ADSTICK,			HIDC_ADSTICK,
	IDC_PEDAL,				HIDC_PEDAL,
	IDC_DIAL,				HIDC_DIAL,
	IDC_TRACKBALL,			HIDC_TRACKBALL,
	IDC_LIGHTGUNDEVICE,		HIDC_LIGHTGUNDEVICE,
	IDC_ENABLE_AUTOSAVE,    HIDC_ENABLE_AUTOSAVE,
	IDC_MULTITHREAD_RENDERING,    HIDC_MULTITHREAD_RENDERING,
	IDC_JSAT,				HIDC_JSAT,
	0,                      0
};

static struct ComboBoxVideo
{
	const TCHAR*	m_pText;
	const char*		m_pData;
} g_ComboBoxVideo[] = 
{
	{ TEXT("GDI"),                  "gdi"    },
	{ TEXT("DirectDraw"),           "ddraw"   },
	{ TEXT("Direct3D"),             "d3d"     },
};
#define NUMVIDEO (sizeof(g_ComboBoxVideo) / sizeof(g_ComboBoxVideo[0]))

static struct ComboBoxD3DVersion
{
	const TCHAR*	m_pText;
	const int		m_pData;
} g_ComboBoxD3DVersion[] = 
{
	{ TEXT("Version 9"),           9   },
	{ TEXT("Version 8"),           8   },
};

#define NUMD3DVERSIONS (sizeof(g_ComboBoxD3DVersion) / sizeof(g_ComboBoxD3DVersion[0]))

static struct ComboBoxSelectScreen
{
	const TCHAR*	m_pText;
	const int		m_pData;
} g_ComboBoxSelectScreen[] = 
{
	{ TEXT("Screen 0"),             0    },
	{ TEXT("Screen 1"),             1    },
	{ TEXT("Screen 2"),             2    },
	{ TEXT("Screen 3"),             3    },
};
#define NUMSELECTSCREEN (sizeof(g_ComboBoxSelectScreen) / sizeof(g_ComboBoxSelectScreen[0]))

static struct ComboBoxView
{
	const TCHAR*	m_pText;
	const char*		m_pData;
} g_ComboBoxView[] = 
{
	{ TEXT("Auto"),		        "auto"        },
	{ TEXT("Standard"),         "standard"    }, 
	{ TEXT("Pixel Aspect"),     "pixel"       }, 
	{ TEXT("Cocktail"),         "cocktail"    },
};
#define NUMVIEW (sizeof(g_ComboBoxView) / sizeof(g_ComboBoxView[0]))



static struct ComboBoxDevices
{
	const TCHAR*	m_pText;
	const char* 	m_pData;
} g_ComboBoxDevice[] = 
{
	{ TEXT("None"),                  "none"      },
	{ TEXT("Keyboard"),              "keyboard"  },
	{ TEXT("Mouse"),				   "mouse"     },
	{ TEXT("Joystick"),              "joystick"  },
	{ TEXT("Lightgun"),              "lightgun"  },
};

#define NUMDEVICES (sizeof(g_ComboBoxDevice) / sizeof(g_ComboBoxDevice[0]))

/***************************************************************
 * Public functions
 ***************************************************************/

void PropertiesInit(void)
{
}

int PropertiesCurrentGame(HWND hDlg)
{
	return g_nGame;
}

DWORD GetHelpIDs(void)
{
	return (DWORD) (LPSTR) dwHelpIDs;
}

// This function (and the code that use it) is a gross hack - but at least the vile
// and disgusting global variables are gone, making it less gross than what came before
static int GetSelectedScreen(HWND hWnd)
{
	int nSelectedScreen = 0;
	HWND hCtrl = GetDlgItem(hWnd, IDC_SCREENSELECT);
	if (hCtrl)
		nSelectedScreen = ComboBox_GetCurSel(hCtrl);
	if ((nSelectedScreen < 0) || (nSelectedScreen >= MAX_SCREENS))
		nSelectedScreen = 0;
	return nSelectedScreen;

}

static PROPSHEETPAGE *CreatePropSheetPages(HINSTANCE hInst, BOOL bOnlyDefault,
	const game_driver *gamedrv, UINT *pnMaxPropSheets, PROP_SOURCE source )
{
	PROPSHEETPAGE *pspages;
	int maxPropSheets;
	int possiblePropSheets;
	int i;
	machine_config drv;

	if (gamedrv)
	    expand_machine_driver(gamedrv->drv, &drv);

	if( source != SRC_GAME )
	{
		i = 2;
	}
	else
	{
		i = 0;
	}
	for (; g_propSheets[i].pfnDlgProc; i++)
		;

	if( source != SRC_GAME )
	{
		possiblePropSheets = i - 1;
	}
	else
	{
		possiblePropSheets = i + 1;
	}

	pspages = malloc(sizeof(PROPSHEETPAGE) * possiblePropSheets);
	if (!pspages)
		return NULL;
	memset(pspages, 0, sizeof(PROPSHEETPAGE) * possiblePropSheets);

	maxPropSheets = 0;
	if( source != SRC_GAME )
	{
		i = 2;
	}
	else
	{
		i = 0;
	}
	for (; g_propSheets[i].pfnDlgProc; i++)
	{
		if ((gamedrv != NULL) || g_propSheets[i].bOnDefaultPage)
		{
			if (!gamedrv || !g_propSheets[i].pfnFilterProc || g_propSheets[i].pfnFilterProc(&drv, gamedrv))
			{
				pspages[maxPropSheets].dwSize                     = sizeof(PROPSHEETPAGE);
				pspages[maxPropSheets].dwFlags                    = 0;
				pspages[maxPropSheets].hInstance                  = hInst;
				pspages[maxPropSheets].DUMMYUNIONNAME.pszTemplate = MAKEINTRESOURCE(g_propSheets[i].dwDlgID);
				pspages[maxPropSheets].pfnCallback                = NULL;
				pspages[maxPropSheets].lParam                     = 0;
				pspages[maxPropSheets].pfnDlgProc                 = g_propSheets[i].pfnDlgProc;
				maxPropSheets++;
			}
		}
	}
	
	if (pnMaxPropSheets)
		*pnMaxPropSheets = maxPropSheets;

	return pspages;
}

void InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd)
{
	PROPSHEETHEADER pshead;
	PROPSHEETPAGE   *pspage;

	// clear globals
	pGameOpts = NULL;

	g_nGame = GLOBAL_OPTIONS;

	/* Get default options to populate property sheets */
	pGameOpts = GetDefaultOptions(-1, FALSE);
	g_bUseDefaults = FALSE;
	/* Stash the result for comparing later */
	origGameOpts = CreateGameOptions(OPTIONS_TYPE_GLOBAL);
	options_copy(origGameOpts, pGameOpts);
	orig_uses_defaults = FALSE;
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(pshead));

	pspage = CreatePropSheetPages(hInst, TRUE, NULL, &pshead.nPages, SRC_GAME);
	if (!pspage)
		return;

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	pshead.pszCaption                 = TEXT("Default Game");
	pshead.DUMMYUNIONNAME2.nStartPage = 0;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		win_message_box_utf8(0, temp, "Error", IDOK);
	}

	free(pspage);
}

void InitPropertyPage(HINSTANCE hInst, HWND hWnd, int game_num, HICON hIcon, int folder_index, PROP_SOURCE source)
{
	InitPropertyPageToPage(hInst, hWnd, game_num, hIcon, PROPERTIES_PAGE, folder_index, source);
}

void InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, int game_num, HICON hIcon, int start_page, int folder_index, PROP_SOURCE source )
{
	PROPSHEETHEADER pshead;
	PROPSHEETPAGE   *pspage;
	TCHAR*          t_description = 0;
	TCHAR*          t_foldername = 0;

	// clear globals
	pGameOpts = NULL;

	if (highlight_brush == NULL)
		highlight_brush = CreateSolidBrush(HIGHLIGHT_COLOR);

	if (background_brush == NULL)
		background_brush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));

	g_hIcon = CopyIcon(hIcon);
	InitGameAudit(game_num);
	g_nGame = game_num;
	g_nFolder = game_num;
	if( source == SRC_GAME )
	{
		pGameOpts = GetGameOptions(game_num, folder_index);
		g_bUseDefaults = GetGameUsesDefaults(game_num);
		/* Stash the result for comparing later */
		origGameOpts = CreateGameOptions(game_num);
		options_copy(origGameOpts, pGameOpts);
		g_nFolderGame = game_num;
		g_nPropertyMode = SOURCE_GAME;
	}
	else
	{
		g_nFolderGame = folder_index;
		//For our new scheme to work, we need the possibility to determine whether a sourcefolder contains vector games
		//so we take the selected game in the listview of a source file to check if it is a Vector game
		//this implies that vector games are not mixed up in the source with non-vector games
		//which I think is the case
		if (DriverIsVector( folder_index ) && (game_num != FOLDER_VECTOR ) )
		{
			GetFolderOptions(game_num, TRUE);
			pGameOpts = GetSourceOptions( folder_index );
		}
		else
		{
			pGameOpts = GetFolderOptions( game_num, FALSE );
		}
		if( g_nFolder == FOLDER_VECTOR )
		{
			g_nPropertyMode = SOURCE_VECTOR;
			g_bUseDefaults = GetVectorUsesDefaults() ? TRUE : FALSE;
		}
		else
		{
			g_nPropertyMode = SOURCE_FOLDER;
			g_bUseDefaults = GetFolderUsesDefaults(g_nFolder, g_nFolderGame) ? TRUE : FALSE;
		}
		if( DriverIsVector( folder_index ) && (game_num != FOLDER_VECTOR ) )
		{
			pGameOpts =GetFolderOptions(game_num, TRUE);
			//CopyGameOptions(GetSourceOptions( folder_index ), &folder_options[folder_index] );
			//pGameOpts = &folder_options[folder_index];
		}
		else
		{
			pGameOpts = GetFolderOptions( game_num, FALSE );
		}
		/* Stash the result for comparing later */
		g_nGame = FOLDER_OPTIONS;
		origGameOpts = CreateGameOptions(OPTIONS_TYPE_FOLDER);
		options_copy(origGameOpts, pGameOpts);
	}
	orig_uses_defaults = g_bUseDefaults;
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(PROPSHEETHEADER));

	if( source == SRC_GAME )
	{
		pspage = CreatePropSheetPages(hInst, FALSE, drivers[game_num], &pshead.nPages, source);
	}
	else
	{
		pspage = CreatePropSheetPages(hInst, FALSE, NULL, &pshead.nPages, source);
	}
	if (!pspage)
		return;

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	if( source == SRC_GAME )
	{
		t_description = tstring_from_utf8(ModifyThe(drivers[g_nGame]->description));
		if( !t_description )
			return;
		pshead.pszCaption = t_description;
	}
	else
	{
		t_foldername = tstring_from_utf8(GetFolderNameByID(g_nFolder));
		if( !t_foldername )
			return;
		pshead.pszCaption = t_foldername;
	}
	pshead.DUMMYUNIONNAME2.nStartPage = start_page;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		win_message_box_utf8(0, temp, "Error", IDOK);
	}
	
	if( t_foldername )
		free(t_foldername);
	if( t_description )
		free(t_description);
	free(pspage);
}

/*********************************************************************
 * Local Functions
 *********************************************************************/

/* Build CPU info string */
static char *GameInfoCPU(UINT nIndex)
{
	int i;
	static char buf[1024] = "";
	machine_config drv;
	expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));

	cpuintrf_init(NULL);

	i = 0;
	while (i < MAX_CPU && drv.cpu[i].cpu_type)
	{
		if (drv.cpu[i].cpu_clock >= 1000000)
			sprintf(&buf[strlen(buf)], "%s %d.%06d MHz",
					cputype_name(drv.cpu[i].cpu_type),
					drv.cpu[i].cpu_clock / 1000000,
					drv.cpu[i].cpu_clock % 1000000);
		else
			sprintf(&buf[strlen(buf)], "%s %d.%03d kHz",
					cputype_name(drv.cpu[i].cpu_type),
					drv.cpu[i].cpu_clock / 1000,
					drv.cpu[i].cpu_clock % 1000);

		strcat(buf, "\n");

		i++;
    }

	return buf;
}

/* Build Sound system info string */
static char *GameInfoSound(UINT nIndex)
{
	int i;
	static char buf[1024];
	machine_config drv;
	expand_machine_driver(drivers[nIndex]->drv,&drv);

	buf[0] = 0;

	i = 0;
	while (i < MAX_SOUND && drv.sound[i].sound_type)
	{
		int clock,sound_type,count;

		sound_type = drv.sound[i].sound_type;
		clock = drv.sound[i].clock;

		count = 1;
		i++;

		while (i < MAX_SOUND
				&& drv.sound[i].sound_type == sound_type
				&& drv.sound[i].clock == clock)
		{
			count++;
			i++;
		}

		if (count > 1)
			sprintf(&buf[strlen(buf)],"%dx",count);

		sprintf(&buf[strlen(buf)],"%s",sndtype_name(sound_type));

		if (clock)
		{
			if (clock >= 1000000)
				sprintf(&buf[strlen(buf)]," %d.%06d MHz",
						clock / 1000000,
						clock % 1000000);
			else
				sprintf(&buf[strlen(buf)]," %d.%03d kHz",
						clock / 1000,
						clock % 1000);
		}

		strcat(buf,"\n");
	}

	return buf;
}

/* Build Display info string */
static char *GameInfoScreen(UINT nIndex)
{
	static char buf[1024];
	machine_config drv;
	expand_machine_driver(drivers[nIndex]->drv,&drv);

	if (drv.video_attributes & VIDEO_TYPE_VECTOR)
		strcpy(buf, "Vector Game");
	else
	{
		if (drivers[nIndex]->flags & ORIENTATION_SWAP_XY)
			sprintf(buf,"%d x %d (V) %f Hz",
			drv.screen[0].defstate.visarea.max_y - drv.screen[0].defstate.visarea.min_y + 1,
					drv.screen[0].defstate.visarea.max_x - drv.screen[0].defstate.visarea.min_x + 1,
					SUBSECONDS_TO_HZ(drv.screen[0].defstate.refresh));
		else
			sprintf(buf,"%d x %d (H) %f Hz",
					drv.screen[0].defstate.visarea.max_x - drv.screen[0].defstate.visarea.min_x + 1,
					drv.screen[0].defstate.visarea.max_y - drv.screen[0].defstate.visarea.min_y + 1,
					SUBSECONDS_TO_HZ(drv.screen[0].defstate.refresh));
	}
	return buf;
}

/* Build color information string */
static char *GameInfoColors(UINT nIndex)
{
	static char buf[1024];
	machine_config drv;
	expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));
	sprintf(buf, "%d colors ", drv.total_colors);

	return buf;
}

/* Build game status string */
const char *GameInfoStatus(int driver_index, BOOL bRomStatus)
{
	static char buffer[1024];
	int audit_result = GetRomAuditResults(driver_index);
	memset(buffer,0,sizeof(char)*1024);
	if ( bRomStatus )
 	{
		if (IsAuditResultKnown(audit_result) == FALSE)
 		{
			strcpy(buffer, "Unknown");
		}
		else if (IsAuditResultYes(audit_result))
		{
			if (DriverIsBroken(driver_index))
 			{
				strcpy(buffer, "Not working");
				
				if (drivers[driver_index]->flags & GAME_UNEMULATED_PROTECTION)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Game protection isn't fully emulated");
				}
				if (drivers[driver_index]->flags & GAME_WRONG_COLORS)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Colors are completely wrong");
				}
				if (drivers[driver_index]->flags & GAME_IMPERFECT_COLORS)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Colors aren't 100% accurate");
				}
				if (drivers[driver_index]->flags & GAME_IMPERFECT_GRAPHICS)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Video emulation isn't 100% accurate");
				}
				if (drivers[driver_index]->flags & GAME_NO_SOUND)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Game lacks sound");
				}
				if (drivers[driver_index]->flags & GAME_IMPERFECT_SOUND)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Sound emulation isn't 100% accurate");
				}
				if (drivers[driver_index]->flags & GAME_NO_COCKTAIL)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Screen flipping is not supported");
				}
 			}
			else
			{
				strcpy(buffer, "Working");
				
				if (drivers[driver_index]->flags & GAME_UNEMULATED_PROTECTION)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Game protection isn't fully emulated");
				}
				if (drivers[driver_index]->flags & GAME_WRONG_COLORS)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Colors are completely wrong");
				}
				if (drivers[driver_index]->flags & GAME_IMPERFECT_COLORS)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Colors aren't 100% accurate");
				}
				if (drivers[driver_index]->flags & GAME_IMPERFECT_GRAPHICS)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Video emulation isn't 100% accurate");
				}
				if (drivers[driver_index]->flags & GAME_NO_SOUND)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Game lacks sound");
				}
				if (drivers[driver_index]->flags & GAME_IMPERFECT_SOUND)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Sound emulation isn't 100% accurate");
				}
				if (drivers[driver_index]->flags & GAME_NO_COCKTAIL)
				{
					if (*buffer != '\0')
						strcat(buffer, "\r\n");
					strcat(buffer, "Screen flipping is not supported");
				}
			}
		}
		else
		{
			// audit result is no
	#ifdef MESS
			strcpy(buffer, "BIOS missing");
	#else
			strcpy(buffer, "ROMs missing");
	#endif
		}
	}
	else
	{
		//Just show the emulation flags
		if (DriverIsBroken(driver_index))
		{
			strcpy(buffer, "Not working");
		}
		else
		{
			strcpy(buffer, "Working");
		}	
		if (drivers[driver_index]->flags & GAME_UNEMULATED_PROTECTION)
		{
			if (*buffer != '\0')
				strcat(buffer, "\r\n");
			strcat(buffer, "Game protection isn't fully emulated");
		}
		if (drivers[driver_index]->flags & GAME_WRONG_COLORS)
		{
		if (*buffer != '\0')
				strcat(buffer, "\r\n");
			strcat(buffer, "Colors are completely wrong");
		}
		if (drivers[driver_index]->flags & GAME_IMPERFECT_COLORS)
		{
			if (*buffer != '\0')
				strcat(buffer, "\r\n");
			strcat(buffer, "Colors aren't 100% accurate");
		}
		if (drivers[driver_index]->flags & GAME_IMPERFECT_GRAPHICS)
		{
			if (*buffer != '\0')
				strcat(buffer, "\r\n");
			strcat(buffer, "Video emulation isn't 100% accurate");
		}
		if (drivers[driver_index]->flags & GAME_NO_SOUND)
		{
			if (*buffer != '\0')
				strcat(buffer, "\r\n");
			strcat(buffer, "Game lacks sound");
		}
		if (drivers[driver_index]->flags & GAME_IMPERFECT_SOUND)
		{
			if (*buffer != '\0')
				strcat(buffer, "\r\n");
			strcat(buffer, "Sound emulation isn't 100% accurate");
		}
		if (drivers[driver_index]->flags & GAME_NO_COCKTAIL)
		{
			if (*buffer != '\0')
				strcat(buffer, "\r\n");
			strcat(buffer, "Screen flipping is not supported");
		}
	}
	return buffer;
}

/* Build game manufacturer string */
static char *GameInfoManufactured(UINT nIndex)
{
	static char buffer[1024];

	snprintf(buffer,sizeof(buffer),"%s %s",drivers[nIndex]->year,drivers[nIndex]->manufacturer); 
	return buffer;
}

/* Build Game title string */
char *GameInfoTitle(UINT nIndex)
{
	static char buf[1024];

	if (nIndex == GLOBAL_OPTIONS)
		strcpy(buf, "Global game options\nDefault options used by all games");
	else if (nIndex == FOLDER_OPTIONS)
		strcpy(buf, "Global folder options\nDefault options used by all games in the folder");
	else
		sprintf(buf, "%s\n\"%s\"", ModifyThe(drivers[nIndex]->description), drivers[nIndex]->name); 
	return buf;
}

/* Build game clone infromation string */
static char *GameInfoCloneOf(UINT nIndex)
{
	static char buf[1024];
	int nParentIndex= -1;

	buf[0] = '\0';

	if (DriverIsClone(nIndex))
	{
		nParentIndex = GetParentIndex(drivers[nIndex]);
		sprintf(buf, "%s - \"%s\"",
			ConvertAmpersandString(ModifyThe(drivers[nParentIndex]->description)),
			drivers[nParentIndex]->name); 
	}

	return buf;
}

static const char * GameInfoSource(UINT nIndex)
{
	return GetDriverFilename(nIndex);
}

/* Handle the information property page */
INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
HWND hWnd;
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (g_hIcon)
			SendDlgItemMessage(hDlg, IDC_GAME_ICON, STM_SETICON, (WPARAM) g_hIcon, 0);
#if defined(USE_SINGLELINE_TABCONTROL)
		{
			HWND hWnd = PropSheet_GetTabControl(GetParent(hDlg));
			DWORD tabStyle = (GetWindowLong(hWnd,GWL_STYLE) & ~TCS_MULTILINE);
			SetWindowLong(hWnd,GWL_STYLE,tabStyle | TCS_SINGLELINE);
		}
#endif

		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_TITLE),         GameInfoTitle(g_nGame));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_MANUFACTURED),  GameInfoManufactured(g_nGame));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_STATUS),        GameInfoStatus(g_nGame, FALSE));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_CPU),           GameInfoCPU(g_nGame));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_SOUND),         GameInfoSound(g_nGame));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_SCREEN),        GameInfoScreen(g_nGame));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_COLORS),        GameInfoColors(g_nGame));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_CLONEOF),       GameInfoCloneOf(g_nGame));
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_SOURCE),        GameInfoSource(g_nGame));
		
		if (DriverIsClone(g_nGame))
		{
			ShowWindow(GetDlgItem(hDlg, IDC_PROP_CLONEOF_TEXT), SW_SHOW);
		}
		else
		{
			ShowWindow(GetDlgItem(hDlg, IDC_PROP_CLONEOF_TEXT), SW_HIDE);
		}
		hWnd = PropSheet_GetTabControl(GetParent(hDlg));
		UpdateBackgroundBrush(hWnd);
		ShowWindow(hDlg, SW_SHOW);
		return 1;

	}
    return 0;
}

/* Handle all options property pages */
INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	RECT rc;
	int nParentIndex = -1;
	switch (Msg)
	{
	case WM_INITDIALOG:
		/* Fill in the Game info at the top of the sheet */
		win_set_window_text_utf8(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(g_nGame));
		InitializeOptions(hDlg);
		InitializeMisc(hDlg);

		datamap_populate_all_controls(properties_datamap, hDlg, pGameOpts);
		OptionsToProp(hDlg, pGameOpts);
		if( g_nGame >= 0)
		{
			g_bUseDefaults = GetGameUsesDefaults(g_nGame);
		}
		if( g_nGame == FOLDER_OPTIONS)
		{
			if( g_nFolder == FOLDER_VECTOR )
			{
				g_bUseDefaults = GetVectorUsesDefaults() ? TRUE : FALSE;
			}
			else
			{
				g_bUseDefaults = GetFolderUsesDefaults(g_nFolder, g_nFolderGame) ? TRUE : FALSE;
			}
		}
		SetPropEnabledControls(hDlg);
		if (g_nGame == GLOBAL_OPTIONS)
			ShowWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), SW_HIDE);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);

		EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), g_bReset);
		ShowWindow(hDlg, SW_SHOW);
    
		return 1;

	case WM_HSCROLL:
		/* slider changed */
		HANDLE_WM_HSCROLL(hDlg, wParam, lParam, OptOnHScroll);
		g_bUseDefaults = FALSE;
		g_bReset = TRUE;
		EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), TRUE);
		PropSheet_Changed(GetParent(hDlg), hDlg);

		// make sure everything's copied over, to determine what's changed
		PropToOptions(hDlg,pGameOpts);
		datamap_read_all_controls(properties_datamap, hDlg, pGameOpts);
		// redraw it, it might be a new color now
		InvalidateRect((HWND)lParam,NULL,TRUE);

		break;

	case WM_COMMAND:
		{
			/* Below, 'changed' is used to signify the 'Apply'
			 * button should be enabled.
			 */
			WORD wID         = GET_WM_COMMAND_ID(wParam, lParam);
			HWND hWndCtrl    = GET_WM_COMMAND_HWND(wParam, lParam);
			WORD wNotifyCode = GET_WM_COMMAND_CMD(wParam, lParam);
			BOOL changed     = FALSE;
			int nCurSelection = 0;
			TCHAR szClass[256];

			switch (wID)
			{
			case IDC_REFRESH:
				if (wNotifyCode == LBN_SELCHANGE)
				{
					RefreshSelectionChange(hDlg, hWndCtrl);
					changed = TRUE;
				}
				break;

			case IDC_ASPECT:
				nCurSelection = Button_GetCheck( GetDlgItem(hDlg, IDC_ASPECT));
				if( g_bAutoAspect[GetSelectedScreen(hDlg)] != nCurSelection )
				{
					changed = TRUE;
					g_bAutoAspect[GetSelectedScreen(hDlg)] = nCurSelection;
				}
				break;

			case IDC_SELECT_EFFECT:
				changed = SelectEffect(hDlg);
				break;

			case IDC_RESET_EFFECT:
				changed = ResetEffect(hDlg);
				break;

			case IDC_PROP_RESET:
				if (wNotifyCode != BN_CLICKED)
					break;

				options_copy(pGameOpts, origGameOpts);
				if (g_nGame > -1)
					SetGameUsesDefaults(g_nGame,orig_uses_defaults);
				datamap_populate_all_controls(properties_datamap, hDlg, pGameOpts);
				OptionsToProp(hDlg, pGameOpts);
				SetPropEnabledControls(hDlg);
				g_bReset = FALSE;
				PropSheet_UnChanged(GetParent(hDlg), hDlg);
				g_bUseDefaults = orig_uses_defaults;
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				break;

			case IDC_USE_DEFAULT:
				if (g_nGame != GLOBAL_OPTIONS)
				{
					// NPW 20-Apr-2007 - I hate this code.  WTF were people thinking!  This
					// code has more spaghetti than the entire nation of Italy
					if( g_nGame != FOLDER_OPTIONS )
					{
						SetGameUsesDefaults(g_nGame,TRUE);

						SaveGameOptions(g_nGame);
						pGameOpts = GetGameOptions(g_nGame, -1);
					}
					else
					{
						if( g_nFolder == FOLDER_VECTOR)
							options_copy(pGameOpts, GetDefaultOptions(GLOBAL_OPTIONS, TRUE));
						//Not Vector Folder, but Source Folder of Vector Games
						else if ( DriverIsVector(g_nFolderGame) )
							options_copy(pGameOpts, GetVectorOptions());
						// every other folder
						else
							options_copy(pGameOpts, GetDefaultOptions(GLOBAL_OPTIONS, FALSE));

						SaveFolderOptions(g_nFolder, g_nFolderGame);
						pGameOpts = GetFolderOptions(g_nFolder, (g_nFolder == FOLDER_VECTOR));
					}
					datamap_populate_all_controls(properties_datamap, hDlg, pGameOpts);
					OptionsToProp(hDlg, pGameOpts);
					SetPropEnabledControls(hDlg);
					g_bUseDefaults = TRUE;

					if (orig_uses_defaults != g_bUseDefaults)
					{
						PropSheet_Changed(GetParent(hDlg), hDlg);
						g_bReset = TRUE;
					}
					else
					{
						PropSheet_UnChanged(GetParent(hDlg), hDlg);
						g_bReset = FALSE;
					}
					EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				}
				break;

			case IDC_SCREENSELECT:
				if ((wNotifyCode == CBN_SELCHANGE) || (wNotifyCode == CBN_SELENDOK))
				{
					changed = datamap_read_control(properties_datamap, hDlg, pGameOpts, wID);
					datamap_populate_control(properties_datamap, hDlg, pGameOpts, IDC_SCREEN);
				}
				break;

			case IDC_SCREEN:
				// NPW 3-Apr-2007:  Ugh I'm only perpetuating the vile hacks in this code
				if ((wNotifyCode == CBN_SELCHANGE) || (wNotifyCode == CBN_SELENDOK))
				{
					changed = datamap_read_control(properties_datamap, hDlg, pGameOpts, wID);
					datamap_populate_control(properties_datamap, hDlg, pGameOpts, IDC_REFRESH);
					datamap_populate_control(properties_datamap, hDlg, pGameOpts, IDC_SIZES);

					if (strcmp(options_get_string(pGameOpts, "screen0"), options_get_string(origGameOpts, "screen0")) ||
						strcmp(options_get_string(pGameOpts, "screen1"), options_get_string(origGameOpts, "screen1")) ||
						strcmp(options_get_string(pGameOpts, "screen2"), options_get_string(origGameOpts, "screen2")) ||
						strcmp(options_get_string(pGameOpts, "screen3"), options_get_string(origGameOpts, "screen3")))
					{
						changed = TRUE;
					}
				}
				break;

			default:
#ifdef MESS
				if (MessPropertiesCommand(hDlg, wNotifyCode, wID, &changed))
					break;
#endif // MESS

				// use default behavior; try to get the result out of the datamap if
				// appropriate
				GetClassName(hWndCtrl, szClass, ARRAY_LENGTH(szClass));
				if (!_tcscmp(szClass, WC_COMBOBOX))
				{
					// combo box
					if ((wNotifyCode == CBN_SELCHANGE) || (wNotifyCode == CBN_SELENDOK))
					{
						changed = datamap_read_control(properties_datamap, hDlg, pGameOpts, wID);
					}
				}
				else if (!_tcscmp(szClass, WC_BUTTON) && (GetWindowLong(hWndCtrl, GWL_STYLE) & BS_CHECKBOX))
				{
					// check box
					changed = datamap_read_control(properties_datamap, hDlg, pGameOpts, wID);
				}
				break;
			}

			if (changed == TRUE)
			{
				// enable the apply button	
				if (g_nGame > -1)
					SetGameUsesDefaults(g_nGame,FALSE);
				g_bUseDefaults = FALSE;
				PropSheet_Changed(GetParent(hDlg), hDlg);
				g_bReset = TRUE;
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
			}
			SetPropEnabledControls(hDlg);

			// make sure everything's copied over, to determine what's changed
			datamap_read_all_controls(properties_datamap, hDlg, pGameOpts);

			// redraw it, it might be a new color now
			if (GetDlgItem(hDlg,wID))
				InvalidateRect(GetDlgItem(hDlg,wID),NULL,FALSE);

		}
		break;

	case WM_NOTIFY:
		{
			switch (((NMHDR *) lParam)->code)
			{
				//We'll need to use a CheckState Table 
				//Because this one gets called for all kinds of other things too, and not only if a check is set
			case LVN_ITEMCHANGED: 
				{
					if ( ((NMLISTVIEW *) lParam)->hdr.idFrom == IDC_ANALOG_AXES )
					{
						HWND hList = ((NMLISTVIEW *) lParam)->hdr.hwndFrom;
						int iItem = ((NMLISTVIEW *) lParam)->iItem;
						BOOL bCheckState = ListView_GetCheckState(hList, iItem );
						
						if( bCheckState != g_bAnalogCheckState[iItem] )
						{
							// enable the apply button
							if (g_nGame > -1)
								SetGameUsesDefaults(g_nGame,FALSE);
							g_bUseDefaults = FALSE;
							PropSheet_Changed(GetParent(hDlg), hDlg);
							g_bReset = TRUE;
							EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
							g_bAnalogCheckState[iItem] = ! g_bAnalogCheckState[iItem];
						}
					}
				}
				break;
			case PSN_SETACTIVE:
				/* Initialize the controls. */
				datamap_populate_all_controls(properties_datamap, hDlg, pGameOpts);
				OptionsToProp(hDlg, pGameOpts);
				SetPropEnabledControls(hDlg);
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				break;

			case PSN_APPLY:
				/* Save and apply the options here */
				if (g_nGame == GLOBAL_OPTIONS)
				{
					pGameOpts = GetDefaultOptions(g_nGame, FALSE);
					datamap_read_all_controls(properties_datamap, hDlg, pGameOpts);
				}
				else if (g_nGame == FOLDER_OPTIONS)
				{
					pGameOpts = GetFolderOptions(g_nFolder, (g_nFolder == FOLDER_VECTOR));
					datamap_read_all_controls(properties_datamap, hDlg, pGameOpts);
					SaveFolderOptions(g_nFolder, g_nFolderGame);
				}
				else
				{
					//SetGameUsesDefaults(g_nGame,g_bUseDefaults);
					//orig_uses_defaults = g_bUseDefaults;
					pGameOpts = GetGameOptions(g_nGame, -1);
				}

				options_copy(origGameOpts, pGameOpts);

				datamap_populate_all_controls(properties_datamap, hDlg, pGameOpts);
				OptionsToProp(hDlg, pGameOpts);
				SetPropEnabledControls(hDlg);
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				g_bReset = FALSE;
				PropSheet_UnChanged(GetParent(hDlg), hDlg);
				SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
				return PSNRET_NOERROR;

			case PSN_KILLACTIVE:
				/* Save Changes to the options here. */
				datamap_read_all_controls(properties_datamap, hDlg, pGameOpts);
				ResetDataMap(hDlg);
				SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
				return 1;  

			case PSN_RESET:
				// Reset to the original values. Disregard changes
				options_copy(pGameOpts, origGameOpts);
				SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
				break;

			case PSN_HELP:
				// User wants help for this property page
				break;
			}
		}
		break;
	case WM_CTLCOLORSTATIC :
	case WM_CTLCOLOREDIT :
		if( g_nGame >= 0 )
		{
			if( DriverIsClone(g_nGame) )
			{
				nParentIndex = GetParentIndex( drivers[g_nGame] );
			}
		}
		//Set the Coloring of the elements
		if (GetWindowLong((HWND)lParam, GWL_ID) < 0)
			break;
		if (IsControlOptionValue(hDlg,(HWND)lParam, GetDefaultOptions( -1, FALSE) ) )
		{
			//Normal Black case
			SetTextColor((HDC)wParam,COLOR_WINDOWTEXT);
		}
		else if (IsControlOptionValue(hDlg,(HWND)lParam, GetVectorOptions() )  && DriverIsVector(g_nFolderGame) )
		{
			SetTextColor((HDC)wParam,VECTOR_COLOR);
		}
		else if (IsControlOptionValue(hDlg,(HWND)lParam, GetSourceOptions(g_nFolderGame ) ) )
		{
			SetTextColor((HDC)wParam,FOLDER_COLOR);
		}
		else if ((nParentIndex >=0) && (IsControlOptionValue(hDlg,(HWND)lParam, GetGameOptions(nParentIndex, g_nFolder ) ) ) )
		{
			SetTextColor((HDC)wParam,PARENT_COLOR);
		}
//		else if ( (g_nGame >= 0) && (IsControlOptionValue(hDlg,(HWND)lParam, GetGameOptions(g_nGame, g_nFolder) ) ) )
		else if ( (g_nGame >= 0) && (IsControlOptionValue(hDlg,(HWND)lParam, origGameOpts ) ) )
		{
			SetTextColor((HDC)wParam,GAME_COLOR);
		}
		else
		{
			switch ( g_nPropertyMode )
			{
				case SOURCE_GAME:
					SetTextColor((HDC)wParam,GAME_COLOR);
					break;
				case SOURCE_FOLDER:
					SetTextColor((HDC)wParam,FOLDER_COLOR);
					break;
				case SOURCE_VECTOR:
					SetTextColor((HDC)wParam,VECTOR_COLOR);
					break;
				default:
				case SOURCE_GLOBAL:
					SetTextColor((HDC)wParam,COLOR_WINDOWTEXT);
					break;
			}
		}
		if( Msg == WM_CTLCOLORSTATIC )
		{
			if (SafeIsAppThemed())
			{
				HWND hWnd = PropSheet_GetTabControl(GetParent(hDlg));
				// Set the background mode to transparent
				SetBkMode((HDC)wParam, TRANSPARENT);

				// Get the controls window dimensions
				GetWindowRect((HWND)lParam, &rc);

				// Map the coordinates to coordinates with the upper left corner of dialog control as base
				MapWindowPoints(NULL, hWnd, (LPPOINT)(&rc), 2);

				// Adjust the position of the brush for this control (else we see the top left of the brush as background)
				SetBrushOrgEx((HDC)wParam, -rc.left, -rc.top, NULL);

				// Return the brush
				return (INT_PTR)(hBkBrush);
			}
			else
			{
				SetBkColor((HDC) wParam,GetSysColor(COLOR_3DFACE) );
			}
		}
		else
			SetBkColor((HDC)wParam,RGB(255,255,255) );
		UnrealizeObject(background_brush);
		return (DWORD)background_brush;
		break;
	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		HelpFunction(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU: 
		HelpFunction((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break; 

	}
	EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), g_bReset);

	return 0;
}

/* Read controls that are not handled in the DataMap */
static void PropToOptions(HWND hWnd, core_options *o)
{
	HWND hCtrl;
	HWND hCtrl2;
	HWND hCtrl3;

	if (g_nGame > -1)
		SetGameUsesDefaults(g_nGame,g_bUseDefaults);

	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	hCtrl3 = GetDlgItem(hWnd, IDC_ASPECT);
	if (hCtrl && hCtrl2 && hCtrl3)
	{
		char aspect_option[32];
		snprintf(aspect_option, ARRAY_LENGTH(aspect_option), "aspect%d", GetSelectedScreen(hWnd));

		if (Button_GetCheck(hCtrl3))
		{
			options_set_string(o, aspect_option, "auto");
		}
		else
		{
			int n = 0;
			int d = 0;
			TCHAR buffer[200];
			char buffer2[200];

			Edit_GetText(hCtrl,buffer,sizeof(buffer));
			_stscanf(buffer,TEXT("%d"),&n);

			Edit_GetText(hCtrl2,buffer,sizeof(buffer));
			_stscanf(buffer,TEXT("%d"),&d);

			if (n == 0 || d == 0)
			{
				n = 4;
				d = 3;
			}

			snprintf(buffer2,sizeof(buffer2),"%d:%d",n,d);
			options_set_string(o, aspect_option, buffer2);
		}
	}
	/*analog axes*/
	hCtrl = GetDlgItem(hWnd, IDC_ANALOG_AXES);	
	if (hCtrl)
	{
		int nCount;
		TCHAR buffer[200];
		TCHAR digital[200];
		int oldJoyId = -1;
		int joyId = 0;
		int axisId = 0;
		BOOL bFirst = TRUE;
		char* utf8_digital;
		memset(digital,0,ARRAY_LENGTH(digital));
		// Get the number of items in the control
		for(nCount=0;nCount < ListView_GetItemCount(hCtrl);nCount++)
		{
			if( ListView_GetCheckState(hCtrl,nCount) )
			{
				//Get The JoyId
				ListView_GetItemText(hCtrl, nCount,2, buffer, ARRAY_LENGTH(buffer));
				joyId = _ttoi(buffer);
				if( oldJoyId != joyId) 
				{
					oldJoyId = joyId;
					//add new JoyId
					if( bFirst )
					{
						_tcscat(digital, TEXT("j"));
						bFirst = FALSE;
					}
					else
					{
						_tcscat(digital, TEXT(",j"));
					}
					_tcscat(digital, buffer);
				}
				//Get The AxisId
				ListView_GetItemText(hCtrl, nCount,3, buffer, ARRAY_LENGTH(buffer));
				axisId = _ttoi(buffer);
				_tcscat(digital, TEXT("a"));
				_tcscat(digital, buffer);
			}
		}
		utf8_digital = utf8_from_tstring(digital);
		if( !utf8_digital )
			return;
		
		if (mame_stricmp (utf8_digital, options_get_string(o, WINOPTION_DIGITAL)) != 0)
		{
			// save the new setting
			options_set_string(o, WINOPTION_DIGITAL, utf8_digital);
		}
		free(utf8_digital);
	}
}

/* Populate controls that are not handled in the DataMap */
static void OptionsToProp(HWND hWnd, core_options* o)
{
	HWND hCtrl;
	HWND hCtrl2;
	TCHAR buf[100];
	int  n = 0;
	int  d = 0;

	/* video */

	/* Setup refresh list based on depth. */
	datamap_update_control(properties_datamap, hWnd, pGameOpts, IDC_REFRESH);
	/* Setup Select screen*/
	UpdateSelectScreenUI(hWnd );

	hCtrl = GetDlgItem(hWnd, IDC_ASPECT);
	if (hCtrl)
	{
		Button_SetCheck(hCtrl, g_bAutoAspect[GetSelectedScreen(hWnd)] )	;
	}

	/* Bios select list */
	hCtrl = GetDlgItem(hWnd, IDC_BIOS);
	if (hCtrl)
	{
		const char* cBuffer;
		int i = 0;
		int iCount = ComboBox_GetCount( hCtrl );
		for( i=0; i< iCount; i++ )
		{
			cBuffer = (const char*)ComboBox_GetItemData( hCtrl, i );
			if (strcmp(cBuffer, options_get_string(pGameOpts, OPTION_BIOS) ) == 0)
			{
				ComboBox_SetCurSel(hCtrl, i);
				break;
			}

		}
	}


	hCtrl = GetDlgItem(hWnd, IDC_ASPECT);
	if (hCtrl)
	{
		char aspect_option[32];
		snprintf(aspect_option, ARRAY_LENGTH(aspect_option), "aspect%d", GetSelectedScreen(hWnd));
		if( strcmp(options_get_string(o, aspect_option), "auto") == 0)
		{
			Button_SetCheck(hCtrl, TRUE);
			g_bAutoAspect[GetSelectedScreen(hWnd)] = TRUE;
		}
		else
		{
			Button_SetCheck(hCtrl, FALSE);
			g_bAutoAspect[GetSelectedScreen(hWnd)] = FALSE;
		}
	}

	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	if (hCtrl && hCtrl2)
	{
		char aspect_option[32];
		snprintf(aspect_option, ARRAY_LENGTH(aspect_option), "aspect%d", GetSelectedScreen(hWnd));

		n = 0;
		d = 0;
		if (options_get_string(o, aspect_option))
		{
			if (sscanf(options_get_string(o, aspect_option), "%d:%d", &n, &d) == 2 && n != 0 && d != 0)
			{
				_stprintf(buf, TEXT("%d"), n);
				Edit_SetText(hCtrl, buf);
				_stprintf(buf, TEXT("%d"), d);
				Edit_SetText(hCtrl2, buf);
			}
			else
			{
				Edit_SetText(hCtrl,  TEXT("4"));
				Edit_SetText(hCtrl2, TEXT("3"));
			}
		}
		else
		{
			Edit_SetText(hCtrl,  TEXT("4"));
			Edit_SetText(hCtrl2, TEXT("3"));
		}
	}

	/* core video */
	hCtrl = GetDlgItem(hWnd, IDC_ANALOG_AXES);	
	if (hCtrl)
	{
		int nCount;

		/* Get the number of items in the control */
		TCHAR buffer[200];
		TCHAR digital[200];
		TCHAR *pDest = NULL;
		TCHAR *pDest2 = NULL;
		TCHAR *pDest3 = NULL;
		int result = 0;
		int result2 = 0;
		int result3 = 0;
		int joyId = 0;
		int axisId = 0;
		TCHAR* t_digital_option = tstring_from_utf8(options_get_string(o, WINOPTION_DIGITAL));
		if( !t_digital_option )
			return;

		memset(digital,0,sizeof(digital));
		// Get the number of items in the control
		for(nCount=0;nCount < ListView_GetItemCount(hCtrl);nCount++)
		{
			//Get The JoyId
			ListView_GetItemText(hCtrl, nCount,2, buffer, ARRAY_LENGTH(buffer));
			joyId = _ttoi(buffer);
			_stprintf(digital,TEXT("j%s"),buffer);
			//First find the JoyId in the saved String
			pDest = _tcsstr (t_digital_option, digital);
			result = pDest - t_digital_option + 1;
			if ( pDest != NULL)
			{
				//TrimRight pDest to the first Comma, as there starts a new Joystick
				pDest2 = _tcschr(pDest,',');
				if( pDest2 != NULL )
				{
					result2 = pDest2 - pDest + 1;
				}
				//Get The AxisId
				ListView_GetItemText(hCtrl, nCount,3, buffer, ARRAY_LENGTH(buffer));
				axisId = _ttoi(buffer);
				_stprintf(digital,TEXT("a%s"),buffer);
				//Now find the AxisId in the saved String
				pDest3 = _tcsstr (pDest,digital);
				result3 = pDest3 - pDest + 1;
				if ( pDest3 != NULL)
				{
					//if this is after the comma result3 is bigger than result2
					// show the setting in the Control
					if( result2 == 0 )
					{
						//The Table variable needs to be set before we send the message to the Listview,
						//this is true for all below cases, otherwise we get false positives
						g_bAnalogCheckState[nCount] = TRUE;
						ListView_SetCheckState(hCtrl, nCount, TRUE );
					}
					else
					{
						if( result3 < result2)
						{
							g_bAnalogCheckState[nCount] = TRUE;
							ListView_SetCheckState(hCtrl, nCount, TRUE );
						}
						else
						{
							g_bAnalogCheckState[nCount] = FALSE;
							ListView_SetCheckState(hCtrl, nCount, FALSE );
						}
					}
				}
				else
				{
					g_bAnalogCheckState[nCount] = FALSE;
					ListView_SetCheckState(hCtrl, nCount, FALSE );
				}
			}
			else
			{
				g_bAnalogCheckState[nCount] = FALSE;
				ListView_SetCheckState(hCtrl, nCount, FALSE );
			}
		}		
		free(t_digital_option);
	}
}

/* Adjust controls - tune them to the currently selected game */
static void SetPropEnabledControls(HWND hWnd)
{
	HWND hCtrl;
	int  nIndex;
	int  sound;
	BOOL ddraw = FALSE;
	BOOL d3d = FALSE;
	BOOL gdi = FALSE;
	BOOL useart = TRUE;
	BOOL multimon = (DirectDraw_GetNumDisplays() >= 2);
	int joystick_attached = 0;
	int in_window = 0;

	nIndex = g_nGame;
	
	d3d = !mame_stricmp(options_get_string(pGameOpts, WINOPTION_VIDEO), "d3d");
	ddraw = !mame_stricmp(options_get_string(pGameOpts, WINOPTION_VIDEO), "ddraw");
	gdi = !d3d && !ddraw;

	in_window = options_get_bool(pGameOpts, WINOPTION_WINDOW);
	Button_SetCheck(GetDlgItem(hWnd, IDC_ASPECT), g_bAutoAspect[GetSelectedScreen(hWnd)] );

	EnableWindow(GetDlgItem(hWnd, IDC_WAITVSYNC), !gdi);
	EnableWindow(GetDlgItem(hWnd, IDC_TRIPLE_BUFFER), !gdi);
	EnableWindow(GetDlgItem(hWnd, IDC_PRESCALE), !gdi);
	EnableWindow(GetDlgItem(hWnd, IDC_PRESCALEDISP), !gdi);
	EnableWindow(GetDlgItem(hWnd, IDC_HWSTRETCH),       ddraw && DirectDraw_HasHWStretch());
	EnableWindow(GetDlgItem(hWnd, IDC_SWITCHRES), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_SYNCREFRESH),     TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_REFRESH),         !in_window );
	EnableWindow(GetDlgItem(hWnd, IDC_REFRESHTEXT),     !in_window );
	EnableWindow(GetDlgItem(hWnd, IDC_FSGAMMA),      !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSGAMMATEXT),  !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSGAMMADISP),  !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSBRIGHTNESS),      !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSBRIGHTNESSTEXT),  !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSBRIGHTNESSDISP),  !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSCONTRAST),      !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSCONTRASTTEXT),  !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_FSCONTRASTDISP),  !in_window);

	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOTEXT), !g_bAutoAspect[GetSelectedScreen(hWnd)]);
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATION), !g_bAutoAspect[GetSelectedScreen(hWnd)]);
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOD), !g_bAutoAspect[GetSelectedScreen(hWnd)]);

	EnableWindow(GetDlgItem(hWnd,IDC_D3D_FILTER),d3d);
	EnableWindow(GetDlgItem(hWnd,IDC_D3D_VERSION),d3d);
	
	//Switchres and D3D or ddraw enable the per screen parameters

	EnableWindow(GetDlgItem(hWnd, IDC_NUMSCREENS),                 (ddraw || d3d) && multimon);
	EnableWindow(GetDlgItem(hWnd, IDC_NUMSCREENSDISP),             (ddraw || d3d) && multimon);
	EnableWindow(GetDlgItem(hWnd, IDC_SCREENSELECT),               (ddraw || d3d) && multimon);
	EnableWindow(GetDlgItem(hWnd, IDC_SCREENSELECTTEXT),           (ddraw || d3d) && multimon);

	EnableWindow(GetDlgItem(hWnd, IDC_ARTWORK_CROP),	useart);
	EnableWindow(GetDlgItem(hWnd, IDC_BACKDROPS),		useart);
	EnableWindow(GetDlgItem(hWnd, IDC_BEZELS),			useart);
	EnableWindow(GetDlgItem(hWnd, IDC_OVERLAYS),		useart);
	EnableWindow(GetDlgItem(hWnd, IDC_ARTMISCTEXT),		useart);

	/* Joystick options */
	joystick_attached = DIJoystick.Available();

	Button_Enable(GetDlgItem(hWnd,IDC_JOYSTICK),		joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_JDZTEXT),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_JDZDISP),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_JDZ),				joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_JSATTEXT),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_JSATDISP),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_JSAT),				joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_ANALOG_AXES),		joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_ANALOG_AXES_TEXT),joystick_attached);
	/* Trackball / Mouse options */
	if (nIndex <= -1 || DriverUsesTrackball(nIndex) || DriverUsesLightGun(nIndex))
		Button_Enable(GetDlgItem(hWnd,IDC_USE_MOUSE),TRUE);
	else
		Button_Enable(GetDlgItem(hWnd,IDC_USE_MOUSE),FALSE);

	if (!in_window && (nIndex <= -1 || DriverUsesLightGun(nIndex)))
	{
		// on WinXP the Lightgun and Dual Lightgun switches are no longer supported use mouse instead
		OSVERSIONINFOEX osvi;
		BOOL bOsVersionInfoEx;
		// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
		// If that fails, try using the OSVERSIONINFO structure.

		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		{
			osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
			bOsVersionInfoEx = GetVersionEx ( (OSVERSIONINFO *) &osvi);
		}

		if( bOsVersionInfoEx && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 5) )
		{
			BOOL use_lightgun;
			//XP and above...
			Button_Enable(GetDlgItem(hWnd,IDC_LIGHTGUN),FALSE);
			use_lightgun = Button_GetCheck(GetDlgItem(hWnd,IDC_USE_MOUSE));
			Button_Enable(GetDlgItem(hWnd,IDC_DUAL_LIGHTGUN),FALSE);
			Button_Enable(GetDlgItem(hWnd,IDC_RELOAD),use_lightgun);
		}
		else
		{
			BOOL use_lightgun;
			// Older than XP 
			Button_Enable(GetDlgItem(hWnd,IDC_LIGHTGUN),TRUE);
			use_lightgun = Button_GetCheck(GetDlgItem(hWnd,IDC_LIGHTGUN));
			Button_Enable(GetDlgItem(hWnd,IDC_DUAL_LIGHTGUN),use_lightgun);
			Button_Enable(GetDlgItem(hWnd,IDC_RELOAD),use_lightgun);
		}
	}
	else
	{
		Button_Enable(GetDlgItem(hWnd,IDC_LIGHTGUN),FALSE);
		Button_Enable(GetDlgItem(hWnd,IDC_DUAL_LIGHTGUN),FALSE);
		Button_Enable(GetDlgItem(hWnd,IDC_RELOAD),FALSE);
	}


	/* Sound options */
	hCtrl = GetDlgItem(hWnd, IDC_USE_SOUND);
	if (hCtrl)
	{
		sound = Button_GetCheck(hCtrl);
		ComboBox_Enable(GetDlgItem(hWnd, IDC_SAMPLERATE), (sound != 0));

		EnableWindow(GetDlgItem(hWnd,IDC_VOLUME),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_RATETEXT),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_VOLUMEDISP),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_VOLUMETEXT),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_AUDIO_LATENCY),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_AUDIO_LATENCY_DISP),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_AUDIO_LATENCY_TEXT),sound);
		SetSamplesEnabled(hWnd, nIndex, sound);
		SetStereoEnabled(hWnd, nIndex);
		SetYM3812Enabled(hWnd, nIndex);
	}
    
	if (Button_GetCheck(GetDlgItem(hWnd, IDC_AUTOFRAMESKIP)))
		EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), FALSE);
	else
		EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), TRUE);

	if (nIndex <= -1 || DriverHasOptionalBIOS(nIndex))
		EnableWindow(GetDlgItem(hWnd,IDC_BIOS),TRUE);
	else
		EnableWindow(GetDlgItem(hWnd,IDC_BIOS),FALSE);

}

//============================================================
//  CONTROL HELPER FUNCTIONS FOR DATA EXCHANGE
//============================================================

static void RotateReadControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	int selected_index = ComboBox_GetCurSel(control);
	options_set_bool(opts, OPTION_ROR,		selected_index == 1);
	options_set_bool(opts, OPTION_ROL,		selected_index == 2);
	options_set_bool(opts, OPTION_ROTATE,	selected_index != 3);
	options_set_bool(opts, OPTION_AUTOROR,	selected_index == 4);
	options_set_bool(opts, OPTION_AUTOROL,	selected_index == 5);
}



static void RotatePopulateControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	int selected_index = 0;
	if (options_get_bool(opts, OPTION_ROR) && !options_get_bool(opts, OPTION_ROL))
		selected_index = 1;
	else if (!options_get_bool(opts, OPTION_ROR) && options_get_bool(opts, OPTION_ROL))
		selected_index = 2;
	else if (!options_get_bool(opts, OPTION_ROTATE))
		selected_index = 3;
	else if (options_get_bool(opts, OPTION_AUTOROR))
		selected_index = 4;
	else if (options_get_bool(opts, OPTION_AUTOROL))
		selected_index = 5;

	ComboBox_SetCurSel(control, selected_index);
}



static void ScreenReadControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	char screen_option_name[32];
	const char *screen_option_value;
	int selected_screen;
	int screen_option_index;

	selected_screen = GetSelectedScreen(dialog);
	screen_option_index = ComboBox_GetCurSel(control);
	screen_option_value = (const char *) ComboBox_GetItemData(control, screen_option_index);
	snprintf(screen_option_name, ARRAY_LENGTH(screen_option_name), "screen%d", selected_screen);
	options_set_string(opts, screen_option_name, screen_option_value);
}



static void ScreenPopulateControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	int iMonitors;
	DISPLAY_DEVICE dd;
	int i = 0;
	int nSelection = 0;
	TCHAR* t_option = 0;

	/* Remove all items in the list. */
	ComboBox_ResetContent(control);
	ComboBox_InsertString(control, 0, TEXT("Auto"));
	ComboBox_SetItemData(control, 0, (const char*)mame_strdup("auto"));

	//Dynamically populate it, by enumerating the Monitors
	iMonitors = GetSystemMetrics(SM_CMONITORS); // this gets the count of monitors attached
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	for(i=0; EnumDisplayDevices(NULL, i, &dd, 0); i++)
	{
		if( !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) )
		{
			char screen_option[32];

			//we have to add 1 to account for the "auto" entry
			ComboBox_InsertString(control, i+1, win_tstring_strdup(dd.DeviceName));
			ComboBox_SetItemData(control, i+1, (void*)win_tstring_strdup(dd.DeviceName));

			snprintf(screen_option, ARRAY_LENGTH(screen_option), "screen%d", GetSelectedScreen(dialog));
			t_option = tstring_from_utf8(options_get_string(opts, screen_option));
			if( !t_option )
				return;
			if (_tcscmp(t_option, dd.DeviceName) == 0)
				nSelection = i+1;
			free(t_option);
		}
	}
	ComboBox_SetCurSel(control, nSelection);
}



static void ViewSetOptionName(datamap *map, HWND dialog, HWND control, char *buffer, size_t buffer_size)
{
	snprintf(buffer, buffer_size, "view%d", GetSelectedScreen(dialog));
}



static void ViewPopulateControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	int i;
	int selected_index = 0;
	char view_option[32];
	const char *view;

	// determine the view option value
	snprintf(view_option, ARRAY_LENGTH(view_option), "view%d", GetSelectedScreen(dialog));
	view = options_get_string(opts, view_option);

	ComboBox_ResetContent(control);
	for (i = 0; i < NUMVIEW; i++)
	{
		ComboBox_InsertString(control, i, g_ComboBoxView[i].m_pText);
		ComboBox_SetItemData(control, i, g_ComboBoxView[i].m_pData);

		if (!strcmp(view, g_ComboBoxView[i].m_pData))
			selected_index = i;
	}
	ComboBox_SetCurSel(control, selected_index);
}



static void DefaultInputPopulateControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	TCHAR *ext;
	TCHAR root[256];
	TCHAR path[256];
	int selected = 0;
	int index = 0;
	LPCTSTR t_ctrlr_option = 0;
	LPTSTR buf = 0;
	const char *ctrlr_option;
	TCHAR* t_ctrldir;

	// determine the ctrlr option
	ctrlr_option = options_get_string(opts, WINOPTION_CTRLR);
	if( ctrlr_option != NULL ) {
		buf = tstring_from_utf8(ctrlr_option);
		if( !buf )
			return;
		t_ctrlr_option = buf;
	}
	else {
		t_ctrlr_option = TEXT("");
	}

	// reset the controllers dropdown
	ComboBox_ResetContent(control);
	ComboBox_InsertString(control, index, TEXT("Standard"));
	ComboBox_SetItemData(control, index, "");
	index++;
	
	t_ctrldir = tstring_from_utf8(GetCtrlrDir());
	if( !t_ctrldir ) {
		if( buf )
			free(buf);
		return;
	}

	_stprintf (path, TEXT("%s\\*.*"), t_ctrldir);
	
	free(t_ctrldir);

	hFind = FindFirstFile(path, &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do 
		{
			// copy the filename
			_tcscpy (root,FindFileData.cFileName);

			// find the extension
			ext = _tcsrchr (root,'.');
			if (ext)
			{
				// check if it's a cfg file
				if (_tcscmp (ext, TEXT(".cfg")) == 0)
				{
					// and strip off the extension
					*ext = 0;

					// set the option?
					if (!_tcscmp(root, t_ctrlr_option))
						selected = index;

					// add it as an option
					ComboBox_InsertString(control, index, root);
					ComboBox_SetItemData(control, index, (void*)win_tstring_strdup(root));	// FIXME - leaks memory!
					index++;
				}
			}
		}
		while (FindNextFile (hFind, &FindFileData) != 0);
		
		FindClose (hFind);
	}

	ComboBox_SetCurSel(control, selected);
	
	if( buf )
		free(buf);
}



static void ResolutionSetOptionName(datamap *map, HWND dialog, HWND control, char *buffer, size_t buffer_size)
{
	snprintf(buffer, buffer_size, "resolution%d", GetSelectedScreen(dialog));
}



static void ResolutionReadControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	HWND refresh_control = GetDlgItem(dialog, IDC_REFRESH);
	HWND sizes_control = GetDlgItem(dialog, IDC_SIZES);
	int refresh_index, refresh_value, width, height;
	char option_value[256];
	TCHAR buffer[256];

	if (refresh_control && sizes_control)
	{
		ComboBox_GetText(sizes_control, buffer, ARRAY_LENGTH(buffer) - 1);
		if (_stscanf(buffer, TEXT("%d x %d"), &width, &height) == 2)
		{
			refresh_index = ComboBox_GetCurSel(refresh_control);
			refresh_value = ComboBox_GetItemData(refresh_control, refresh_index);
			snprintf(option_value, ARRAY_LENGTH(option_value), "%dx%d@%d", width, height, refresh_value);
		}
		else
		{
			snprintf(option_value, ARRAY_LENGTH(option_value), "auto");
		}
		options_set_string(opts, option_name, option_value);
	}
}



static void ResolutionPopulateControl(datamap *map, HWND dialog, HWND control_, core_options *opts, const char *option_name)
{
	HWND sizes_control = GetDlgItem(dialog, IDC_SIZES);
	HWND refresh_control = GetDlgItem(dialog, IDC_REFRESH);
	int width, height, refresh;
	const char *option_value;
	int sizes_index = 0;
	int refresh_index = 0;
	int sizes_selection = 0;
	int refresh_selection = 0;
	char screen_option[32];
	const char *screen;
	LPTSTR t_screen;
	TCHAR buf[16];
	int i;
	DEVMODE devmode;

	if (sizes_control && refresh_control)
	{
		// determine the resolution
		option_value = options_get_string(opts, option_name);
		if (sscanf(option_value, "%dx%d@%d", &width, &height, &refresh) != 3)
		{
			width = 0;
			height = 0;
			refresh = 0;
		}

		// reset sizes control
		ComboBox_ResetContent(sizes_control);
		ComboBox_InsertString(sizes_control, sizes_index, TEXT("Auto"));
		ComboBox_SetItemData(sizes_control, sizes_index, 0);
		sizes_index++;

		// reset refresh control
		ComboBox_ResetContent(refresh_control);
		ComboBox_InsertString(refresh_control, refresh_index, TEXT("Auto"));
		ComboBox_SetItemData(refresh_control, refresh_index, 0);
		refresh_index++;

		// determine which screen we're using
		snprintf(screen_option, ARRAY_LENGTH(screen_option), "screen%d", GetSelectedScreen(dialog));
		screen = options_get_string(opts, screen_option);
		t_screen = tstring_from_utf8(screen);

		// retrieve screen information
		devmode.dmSize = sizeof(devmode);
		for (i = 0; EnumDisplaySettings(t_screen, i, &devmode); i++)
		{
			if ((devmode.dmBitsPerPel == 32 ) // Only 32 bit depth supported by core
				&&(devmode.dmDisplayFrequency == refresh || refresh == 0))
			{
				_sntprintf(buf, ARRAY_LENGTH(buf), TEXT("%li x %li"),
					devmode.dmPelsWidth, devmode.dmPelsHeight);

				if (ComboBox_FindString(sizes_control, 0, buf) == CB_ERR)
				{
					ComboBox_InsertString(sizes_control, sizes_index, buf);

					if ((width == devmode.dmPelsWidth) && (height == devmode.dmPelsHeight))
						sizes_selection = sizes_index;
					sizes_index++;

				}
			}
			if (devmode.dmDisplayFrequency >= 10 ) 
			{
				// I have some devmode "vga" which specifes 1 Hz, which is probably bogus, so we filter it out

				_sntprintf(buf, ARRAY_LENGTH(buf), TEXT("%li Hz"), devmode.dmDisplayFrequency);

				if (ComboBox_FindString(refresh_control, 0, buf) == CB_ERR)
				{
					ComboBox_InsertString(refresh_control, refresh_index, buf);
					ComboBox_SetItemData(refresh_control, refresh_index, devmode.dmDisplayFrequency);

					if (refresh == devmode.dmDisplayFrequency)
						refresh_selection = refresh_index;

					refresh_index++;
				}
			}
		}
		free(t_screen);

		ComboBox_SetCurSel(sizes_control, sizes_selection);
		ComboBox_SetCurSel(refresh_control, refresh_selection);
	}
}



//============================================================

/************************************************************
 * DataMap initializers
 ************************************************************/

/* Initialize local helper variables */
static void ResetDataMap(HWND hWnd)
{
	char screen_option[32];

	snprintf(screen_option, ARRAY_LENGTH(screen_option), "screen%d", GetSelectedScreen(hWnd));

	if (options_get_string(pGameOpts, screen_option) == NULL 
		|| (mame_stricmp(options_get_string(pGameOpts, screen_option), "") == 0 )
		|| (mame_stricmp(options_get_string(pGameOpts, screen_option), "auto") == 0 ) )
	{
		options_set_string(pGameOpts, screen_option, "auto");
	}
}

/* Build the control mapping by adding all needed information to the DataMap */
static void BuildDataMap(void)
{
	properties_datamap = datamap_create();

	/* video */
	datamap_add(properties_datamap, IDC_D3D_VERSION,			DM_INT,		WINOPTION_D3DVERSION);
	datamap_add(properties_datamap, IDC_VIDEO_MODE,				DM_STRING,	WINOPTION_VIDEO);
	datamap_add(properties_datamap, IDC_PRESCALE,				DM_INT,		WINOPTION_PRESCALE);
	datamap_add(properties_datamap, IDC_PRESCALEDISP,			DM_INT,		WINOPTION_PRESCALE);
	datamap_add(properties_datamap, IDC_NUMSCREENS,				DM_INT,		WINOPTION_NUMSCREENS);
	datamap_add(properties_datamap, IDC_NUMSCREENSDISP,			DM_INT,		WINOPTION_NUMSCREENS);
	datamap_add(properties_datamap, IDC_AUTOFRAMESKIP,			DM_BOOL,	OPTION_AUTOFRAMESKIP);
	datamap_add(properties_datamap, IDC_FRAMESKIP,				DM_INT,		OPTION_FRAMESKIP);
	datamap_add(properties_datamap, IDC_WAITVSYNC,				DM_BOOL,	WINOPTION_WAITVSYNC);
	datamap_add(properties_datamap, IDC_TRIPLE_BUFFER,			DM_BOOL,	WINOPTION_TRIPLEBUFFER);
	datamap_add(properties_datamap, IDC_WINDOWED,				DM_BOOL,	WINOPTION_WINDOW);
	datamap_add(properties_datamap, IDC_HWSTRETCH,				DM_BOOL,	WINOPTION_HWSTRETCH);
	datamap_add(properties_datamap, IDC_SWITCHRES,				DM_BOOL,	WINOPTION_SWITCHRES);
	datamap_add(properties_datamap, IDC_MAXIMIZE,				DM_BOOL,	WINOPTION_MAXIMIZE);
	datamap_add(properties_datamap, IDC_KEEPASPECT,				DM_BOOL,	WINOPTION_KEEPASPECT);
	datamap_add(properties_datamap, IDC_SYNCREFRESH,			DM_BOOL,	WINOPTION_SYNCREFRESH);
	datamap_add(properties_datamap, IDC_THROTTLE,				DM_BOOL,	OPTION_THROTTLE);
	datamap_add(properties_datamap, IDC_FSGAMMA,				DM_FLOAT,	WINOPTION_FULLSCREENGAMMA);
	datamap_add(properties_datamap, IDC_FSGAMMADISP,			DM_FLOAT,	WINOPTION_FULLSCREENGAMMA);
	datamap_add(properties_datamap, IDC_FSBRIGHTNESS,			DM_FLOAT,	WINOPTION_FULLSCREENBRIGHTNESS);
	datamap_add(properties_datamap, IDC_FSBRIGHTNESSDISP,		DM_FLOAT,	WINOPTION_FULLSCREENBRIGHTNESS);
	datamap_add(properties_datamap, IDC_FSCONTRAST,				DM_FLOAT,	WINOPTION_FULLLSCREENCONTRAST);
	datamap_add(properties_datamap, IDC_FSCONTRASTDISP,			DM_FLOAT,	WINOPTION_FULLLSCREENCONTRAST);
	datamap_add(properties_datamap, IDC_EFFECT,					DM_STRING,	WINOPTION_EFFECT);
	datamap_add(properties_datamap, IDC_ASPECTRATIOD,			DM_STRING,  NULL);
	datamap_add(properties_datamap, IDC_ASPECTRATION,			DM_STRING,  NULL);
	datamap_add(properties_datamap, IDC_REFRESH,				DM_STRING,  NULL);
	datamap_add(properties_datamap, IDC_SIZES,					DM_STRING,  NULL);

	// direct3d
	datamap_add(properties_datamap, IDC_D3D_FILTER,				DM_BOOL,	WINOPTION_FILTER);

	// input
	datamap_add(properties_datamap, IDC_DEFAULT_INPUT,			DM_STRING,	WINOPTION_CTRLR);
	datamap_add(properties_datamap, IDC_USE_MOUSE,				DM_BOOL,	WINOPTION_MOUSE);
	datamap_add(properties_datamap, IDC_JOYSTICK,				DM_BOOL,	WINOPTION_JOYSTICK);
	datamap_add(properties_datamap, IDC_JDZ,					DM_FLOAT,	WINOPTION_JOY_DEADZONE);
	datamap_add(properties_datamap, IDC_JDZDISP,				DM_FLOAT,	WINOPTION_JOY_DEADZONE);
	datamap_add(properties_datamap, IDC_JSAT,					DM_FLOAT,	WINOPTION_JOY_SATURATION);
	datamap_add(properties_datamap, IDC_JSATDISP,				DM_FLOAT,	WINOPTION_JOY_SATURATION);
	datamap_add(properties_datamap, IDC_STEADYKEY,				DM_BOOL,	WINOPTION_STEADYKEY);
	datamap_add(properties_datamap, IDC_LIGHTGUN,				DM_BOOL,	WINOPTION_LIGHTGUN);
	datamap_add(properties_datamap, IDC_DUAL_LIGHTGUN,			DM_BOOL,	WINOPTION_DUAL_LIGHTGUN);
	datamap_add(properties_datamap, IDC_RELOAD,					DM_BOOL,	WINOPTION_OFFSCREEN_RELOAD);

	// controller mapping
	datamap_add(properties_datamap, IDC_PADDLE,					DM_STRING,	WINOPTION_PADDLE_DEVICE);
	datamap_add(properties_datamap, IDC_ADSTICK,				DM_STRING,	WINOPTION_ADSTICK_DEVICE);
	datamap_add(properties_datamap, IDC_PEDAL,					DM_STRING,	WINOPTION_PEDAL_DEVICE);
	datamap_add(properties_datamap, IDC_DIAL,					DM_STRING,	WINOPTION_DIAL_DEVICE);
	datamap_add(properties_datamap, IDC_TRACKBALL,				DM_STRING,	WINOPTION_TRACKBALL_DEVICE);
	datamap_add(properties_datamap, IDC_LIGHTGUNDEVICE,			DM_STRING,	WINOPTION_LIGHTGUN_DEVICE);

	// core video
	datamap_add(properties_datamap, IDC_BRIGHTCORRECT,			DM_FLOAT,	OPTION_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_BRIGHTCORRECTDISP,		DM_FLOAT,	OPTION_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_PAUSEBRIGHT,			DM_FLOAT,	OPTION_PAUSE_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_PAUSEBRIGHTDISP,		DM_FLOAT,	OPTION_PAUSE_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_ROTATE,					DM_INT,		NULL);
	datamap_add(properties_datamap, IDC_FLIPX,					DM_BOOL,	OPTION_FLIPX);
	datamap_add(properties_datamap, IDC_FLIPY,					DM_BOOL,	OPTION_FLIPY);
	datamap_add(properties_datamap, IDC_SCREEN,					DM_STRING,	NULL);
	datamap_add(properties_datamap, IDC_SCREENSELECT,			DM_STRING,	NULL);
	datamap_add(properties_datamap, IDC_VIEW,					DM_STRING,	NULL);

	// debugres
	datamap_add(properties_datamap, IDC_GAMMA,					DM_FLOAT,	OPTION_GAMMA);
	datamap_add(properties_datamap, IDC_GAMMADISP,				DM_FLOAT,	OPTION_GAMMA);
	datamap_add(properties_datamap, IDC_CONTRAST,				DM_FLOAT,	OPTION_CONTRAST);
	datamap_add(properties_datamap, IDC_CONTRASTDISP,			DM_FLOAT,	OPTION_CONTRAST);

	// vector
	datamap_add(properties_datamap, IDC_ANTIALIAS,				DM_BOOL,	OPTION_ANTIALIAS);
	datamap_add(properties_datamap, IDC_BEAM,					DM_FLOAT,	OPTION_BEAM);
	datamap_add(properties_datamap, IDC_BEAMDISP,				DM_FLOAT,	OPTION_BEAM);
	datamap_add(properties_datamap, IDC_FLICKER,				DM_FLOAT,	OPTION_FLICKER);
	datamap_add(properties_datamap, IDC_FLICKERDISP,			DM_FLOAT,	OPTION_FLICKER);

	// sound
	datamap_add(properties_datamap, IDC_SAMPLERATE,				DM_INT,		OPTION_SAMPLERATE);
	datamap_add(properties_datamap, IDC_SAMPLES,				DM_BOOL,	OPTION_SAMPLES);
	datamap_add(properties_datamap, IDC_USE_SOUND,				DM_BOOL,	OPTION_SOUND);
	datamap_add(properties_datamap, IDC_VOLUME,					DM_INT,		OPTION_VOLUME);
	datamap_add(properties_datamap, IDC_VOLUMEDISP,				DM_INT,		OPTION_VOLUME);
	datamap_add(properties_datamap, IDC_AUDIO_LATENCY,			DM_INT,		WINOPTION_AUDIO_LATENCY);
	datamap_add(properties_datamap, IDC_AUDIO_LATENCY_DISP,		DM_INT,		WINOPTION_AUDIO_LATENCY);

	// misc artwork options
	datamap_add(properties_datamap, IDC_BACKDROPS,				DM_BOOL,	OPTION_USE_BACKDROPS);
	datamap_add(properties_datamap, IDC_OVERLAYS,				DM_BOOL,	OPTION_USE_OVERLAYS);
	datamap_add(properties_datamap, IDC_BEZELS,					DM_BOOL,	OPTION_USE_BEZELS);
	datamap_add(properties_datamap, IDC_ARTWORK_CROP,			DM_BOOL,	OPTION_ARTWORK_CROP);

	// misc
	datamap_add(properties_datamap, IDC_CHEAT,					DM_BOOL,	OPTION_CHEAT);
	datamap_add(properties_datamap, IDC_LOG,					DM_BOOL,	OPTION_LOG);
	datamap_add(properties_datamap, IDC_SLEEP,					DM_BOOL,	OPTION_SLEEP);
	datamap_add(properties_datamap, IDC_HIGH_PRIORITY,			DM_INT,		WINOPTION_PRIORITY);
	datamap_add(properties_datamap, IDC_HIGH_PRIORITYTXT,		DM_INT,		WINOPTION_PRIORITY);
	datamap_add(properties_datamap, IDC_SKIP_GAME_INFO,			DM_BOOL,	OPTION_SKIP_GAMEINFO);
	datamap_add(properties_datamap, IDC_BIOS,					DM_STRING,	OPTION_BIOS);
	datamap_add(properties_datamap, IDC_ENABLE_AUTOSAVE,		DM_BOOL,	OPTION_AUTOSAVE);
	datamap_add(properties_datamap, IDC_MULTITHREAD_RENDERING,	DM_BOOL,	WINOPTION_MULTITHREADING);

	// set up callbacks
	datamap_set_callback(properties_datamap, IDC_ROTATE,		DCT_READ_CONTROL,		RotateReadControl);
	datamap_set_callback(properties_datamap, IDC_ROTATE,		DCT_POPULATE_CONTROL,	RotatePopulateControl);
	datamap_set_callback(properties_datamap, IDC_SCREEN,		DCT_READ_CONTROL,		ScreenReadControl);
	datamap_set_callback(properties_datamap, IDC_SCREEN,		DCT_POPULATE_CONTROL,	ScreenPopulateControl);
	datamap_set_callback(properties_datamap, IDC_VIEW,			DCT_POPULATE_CONTROL,	ViewPopulateControl);
	datamap_set_callback(properties_datamap, IDC_REFRESH,		DCT_READ_CONTROL,		ResolutionReadControl);
	datamap_set_callback(properties_datamap, IDC_REFRESH,		DCT_POPULATE_CONTROL,	ResolutionPopulateControl);
	datamap_set_callback(properties_datamap, IDC_SIZES,			DCT_READ_CONTROL,		ResolutionReadControl);
	datamap_set_callback(properties_datamap, IDC_SIZES,			DCT_POPULATE_CONTROL,	ResolutionPopulateControl);
	datamap_set_callback(properties_datamap, IDC_DEFAULT_INPUT,	DCT_POPULATE_CONTROL,	DefaultInputPopulateControl);
	datamap_set_option_name_callback(properties_datamap, IDC_VIEW,		ViewSetOptionName);
	datamap_set_option_name_callback(properties_datamap, IDC_REFRESH,	ResolutionSetOptionName);
	datamap_set_option_name_callback(properties_datamap, IDC_SIZES,		ResolutionSetOptionName);

	// formats
	datamap_set_int_format(properties_datamap, IDC_VOLUMEDISP,			"%ddB");
	datamap_set_int_format(properties_datamap, IDC_AUDIO_LATENCY_DISP,	"%d/5");
	datamap_set_float_format(properties_datamap, IDC_BEAMDISP,			"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_FLICKERDISP,		"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_GAMMADISP,			"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_BRIGHTCORRECTDISP,	"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_CONTRASTDISP,		"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_PAUSEBRIGHTDISP,	"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_FSGAMMADISP,		"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_FSBRIGHTNESSDISP,	"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_FSCONTRASTDISP,	"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_JDZDISP,			"%03.2f");
	datamap_set_float_format(properties_datamap, IDC_JSATDISP,			"%03.2f");

	// trackbar ranges
	datamap_set_trackbar_range(properties_datamap, IDC_PRESCALE,    1, 10, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_JDZ,         0.00,  1.00, 0.05);
	datamap_set_trackbar_range(properties_datamap, IDC_JSAT,        0.00,  1.00, 0.05);

#ifdef MESS
	// MESS specific stuff
	MessBuildDataMap(properties_datamap);
#endif // MESS
}

BOOL IsControlOptionValue(HWND hDlg,HWND hwnd_ctrl, core_options *opts )
{
	int control_id = GetWindowLong(hwnd_ctrl, GWL_ID);

	// certain controls we need to handle specially
	switch (control_id)
	{
		case IDC_ASPECTRATION :
		{
			char aspect_option[32];
			int n1 = 0, n2 = 0;
			int d1 = 0, d2 = 0;

			snprintf(aspect_option, ARRAY_LENGTH(aspect_option), "aspect%d", GetSelectedScreen(hDlg));

			sscanf(options_get_string(pGameOpts, aspect_option), "%d:%d", &n1, &d1);
			sscanf(options_get_string(opts, aspect_option), "%d:%d", &n2, &d2);

			return n1 == n2;
		}
		case IDC_ASPECTRATIOD :
		{
			char aspect_option[32];
			int n1 = 0, n2 = 0;
			int d1 = 0, d2 = 0;

			snprintf(aspect_option, ARRAY_LENGTH(aspect_option), "aspect%d", GetSelectedScreen(hDlg));

			sscanf(options_get_string(pGameOpts, aspect_option), "%d:%d", &n1, &d1);
			sscanf(options_get_string(opts, aspect_option), "%d:%d", &n2, &d2);

			return d1 == d2;
		}
		case IDC_SIZES :
		{
			char resolution_option[32];
			int x1=0,y1=0,x2=0,y2=0;

			snprintf(resolution_option, ARRAY_LENGTH(resolution_option), "resolution%d", GetSelectedScreen(hDlg));

			if ((strcmp(options_get_string(pGameOpts, resolution_option), "auto") == 0) &&
				(strcmp(options_get_string(opts, resolution_option), "auto") == 0))
				return TRUE;
			
			sscanf(options_get_string(pGameOpts, resolution_option),"%d x %d",&x1,&y1);
			sscanf(options_get_string(opts, resolution_option),"%d x %d",&x2,&y2);

			return x1 == x2 && y1 == y2;		
		}
		case IDC_ROTATE :
		{
			datamap_read_control(properties_datamap, hDlg, pGameOpts, control_id);

			return (options_get_bool(pGameOpts, OPTION_ROR) == options_get_bool(opts, OPTION_ROR))
				|| (options_get_bool(pGameOpts, OPTION_ROL) == options_get_bool(opts, OPTION_ROL));
		}
	}

	// most options we can compare using data in the data map
	//if (IsControlDifferent(hDlg,hwnd_ctrl,pGameOpts,opts))
	//	return FALSE;

	return TRUE;
}


static void SetStereoEnabled(HWND hWnd, int nIndex)
{
	BOOL enabled = FALSE;
	HWND hCtrl;
	machine_config drv;
	int speakernum, num_speakers;

	num_speakers = 0;

	if ( nIndex > -1)
	{
		expand_machine_driver(drivers[nIndex]->drv,&drv);
		for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
			if (drv.speaker[speakernum].tag != NULL)
				num_speakers++;
	}

	hCtrl = GetDlgItem(hWnd, IDC_STEREO);
	if (hCtrl)
	{
		if (nIndex <= -1 || num_speakers == 2)
			enabled = TRUE;

		EnableWindow(hCtrl, enabled);
	}
}

static void SetYM3812Enabled(HWND hWnd, int nIndex)
{
	int i;
	BOOL enabled;
	HWND hCtrl;
	machine_config drv;

	if (nIndex > -1)
		expand_machine_driver(drivers[nIndex]->drv,&drv);

	hCtrl = GetDlgItem(hWnd, IDC_USE_FM_YM3812);
	if (hCtrl)
	{
		enabled = FALSE;
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (nIndex <= -1
#if HAS_YM3812
			||  drv.sound[i].sound_type == SOUND_YM3812
#endif
#if HAS_YM3526
			||  drv.sound[i].sound_type == SOUND_YM3526
#endif
#if HAS_YM2413
			||  drv.sound[i].sound_type == SOUND_YM2413
#endif
			)
				enabled = TRUE;
		}
    
		EnableWindow(hCtrl, enabled);
	}
}

static void SetSamplesEnabled(HWND hWnd, int nIndex, BOOL bSoundEnabled)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)
	int i;
	BOOL enabled = FALSE;
	HWND hCtrl;
	machine_config drv;

	hCtrl = GetDlgItem(hWnd, IDC_SAMPLES);

	if ( nIndex > -1 )
		expand_machine_driver(drivers[nIndex]->drv,&drv);
	
	if (hCtrl)
	{
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (nIndex <= -1
			||  drv.sound[i].sound_type == SOUND_SAMPLES
#if HAS_VLM5030
			||  drv.sound[i].sound_type == SOUND_VLM5030
#endif
			)
				enabled = TRUE;
		}
		enabled = enabled && bSoundEnabled;
		EnableWindow(hCtrl, enabled);
	}
#endif
}

/* Moved here cause it's called in a few places */
static void InitializeOptions(HWND hDlg)
{
	InitializeSoundUI(hDlg);
	InitializeSkippingUI(hDlg);
	InitializeRotateUI(hDlg);
	InitializeSelectScreenUI(hDlg);
	InitializeAnalogAxesUI(hDlg);
	InitializeEffectUI(hDlg);
	InitializeBIOSUI(hDlg);
	InitializeControllerMappingUI(hDlg);
	InitializeD3DVersionUI(hDlg);
	InitializeVideoUI(hDlg);
}

/* Moved here because it is called in several places */
static void InitializeMisc(HWND hDlg)
{
	Button_Enable(GetDlgItem(hDlg, IDC_JOYSTICK), DIJoystick.Available());
}

static void OptOnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	if (hwndCtl == GetDlgItem(hwnd, IDC_NUMSCREENS))
	{
		NumScreensSelectionChange(hwnd);
	}
}

/* Handle changes to the Numscreens slider */
static void NumScreensSelectionChange(HWND hwnd)
{
	//Also Update the ScreenSelect Combo with the new number of screens
	UpdateSelectScreenUI(hwnd );
}

/* Handle changes to the Refresh drop down */
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl)
{
	int nCurSelection;

	nCurSelection = ComboBox_GetCurSel(hWndCtrl);
	if (nCurSelection != CB_ERR)
	{
		datamap_populate_control(properties_datamap, hWnd, pGameOpts, IDC_SIZES);
	}
}

/* Initialize the sound options */
static void InitializeSoundUI(HWND hwnd)
{
	HWND    hCtrl;
	int i = 0;

	hCtrl = GetDlgItem(hwnd, IDC_SAMPLERATE);
	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, TEXT("11025"));
		ComboBox_SetItemData(hCtrl, i++, 11025);
		ComboBox_AddString(hCtrl, TEXT("22050"));
		ComboBox_SetItemData(hCtrl, i++, 22050);
		ComboBox_AddString(hCtrl, TEXT("44100"));
		ComboBox_SetItemData(hCtrl, i++, 44100);
		ComboBox_AddString(hCtrl, TEXT("48000"));
		ComboBox_SetItemData(hCtrl, i++, 48000);
		ComboBox_SetCurSel(hCtrl, 1);
	}
}

/* Populate the Frame Skipping drop down */
static void InitializeSkippingUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_FRAMESKIP);
	int i = 0;

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, TEXT("Draw every frame"));
		ComboBox_SetItemData(hCtrl, i++, 0);
		ComboBox_AddString(hCtrl, TEXT("Skip 1 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 1);
		ComboBox_AddString(hCtrl, TEXT("Skip 2 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 2);
		ComboBox_AddString(hCtrl, TEXT("Skip 3 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 3);
		ComboBox_AddString(hCtrl, TEXT("Skip 4 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 4);
		ComboBox_AddString(hCtrl, TEXT("Skip 5 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 5);
		ComboBox_AddString(hCtrl, TEXT("Skip 6 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 6);
		ComboBox_AddString(hCtrl, TEXT("Skip 7 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 7);
		ComboBox_AddString(hCtrl, TEXT("Skip 8 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 8);
		ComboBox_AddString(hCtrl, TEXT("Skip 9 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 9);
		ComboBox_AddString(hCtrl, TEXT("Skip 10 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 10);
		ComboBox_AddString(hCtrl, TEXT("Skip 11 of 12 frames"));
		ComboBox_SetItemData(hCtrl, i++, 11);
	}
}

/* Populate the Rotate drop down */
static void InitializeRotateUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_ROTATE);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, TEXT("Default"));             // 0
		ComboBox_AddString(hCtrl, TEXT("Clockwise"));           // 1
		ComboBox_AddString(hCtrl, TEXT("Anti-clockwise"));      // 2
		ComboBox_AddString(hCtrl, TEXT("None"));                // 3
		ComboBox_AddString(hCtrl, TEXT("Auto clockwise"));      // 4
		ComboBox_AddString(hCtrl, TEXT("Auto anti-clockwise")); // 5
	}
}

/* Populate the Video Mode drop down */
static void InitializeVideoUI(HWND hwnd)
{
	HWND    hCtrl;

	hCtrl = GetDlgItem(hwnd, IDC_VIDEO_MODE);
	if (hCtrl)
	{
		int i;
		for (i = 0; i < NUMVIDEO; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxVideo[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxVideo[i].m_pData);
		}
	}
}

/* Populate the D3D Version drop down */
static void InitializeD3DVersionUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_D3D_VERSION);
	if (hCtrl)
	{
		int i;
		for (i = 0; i < NUMD3DVERSIONS; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxD3DVersion[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxD3DVersion[i].m_pData);
		}
	}
}

static void UpdateSelectScreenUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_SCREENSELECT);
	if (hCtrl)
	{
		int i, curSel;
		curSel = ComboBox_GetCurSel(hCtrl);
		if ((curSel < 0) || (curSel >= MAX_SCREENS))
			curSel = 0;
		ComboBox_ResetContent(hCtrl );
		for (i = 0; i < NUMSELECTSCREEN && i < options_get_int(pGameOpts, WINOPTION_NUMSCREENS) ; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxSelectScreen[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxSelectScreen[i].m_pData);
		}
		// Smaller Amount of screens was selected, so use 0
		if( i< curSel )
			ComboBox_SetCurSel(hCtrl, 0 );
		else
			ComboBox_SetCurSel(hCtrl, curSel );
	}
}

/* Populate the Select Screen drop down */
static void InitializeSelectScreenUI(HWND hwnd)
{
	UpdateSelectScreenUI(hwnd);
}

/*Populate the Analog axes Listview*/
static void InitializeAnalogAxesUI(HWND hwnd)
{
	int i=0, j=0, res = 0;
	int iEntryCounter = 0;
	TCHAR buf[256];
	LVITEM item;
	LVCOLUMN column;
	HWND hCtrl = GetDlgItem(hwnd, IDC_ANALOG_AXES);
	if( hCtrl )
	{
		//Enumerate the Joystick axes, and add them to the Listview...
		ListView_SetExtendedListViewStyle(hCtrl,LVS_EX_CHECKBOXES );
		//add two Columns...
		column.mask = LVCF_TEXT | LVCF_WIDTH |LVCF_SUBITEM;
		column.pszText = (TCHAR *)TEXT("Joystick");
		column.cchTextMax = _tcslen(column.pszText);
		column.iSubItem = 0;
		column.cx = 100;
		res = ListView_InsertColumn(hCtrl,0, &column );
		column.pszText = (TCHAR *)TEXT("Axis");
		column.cchTextMax = _tcslen(column.pszText);
		column.iSubItem = 1;
		column.cx = 100;
		res = ListView_InsertColumn(hCtrl,1, &column );
		column.pszText = (TCHAR *)TEXT("JoystickId");
		column.cchTextMax = _tcslen(column.pszText);
		column.iSubItem = 2;
		column.cx = 70;
		res = ListView_InsertColumn(hCtrl,2, &column );
		column.pszText = (TCHAR *)TEXT("AxisId");
		column.cchTextMax = _tcslen(column.pszText);
		column.iSubItem = 3;
		column.cx = 50;
		res = ListView_InsertColumn(hCtrl,3, &column );
		DIJoystick.init();
		memset(&item,0,sizeof(item) );
		item.mask = LVIF_TEXT;
		for( i=0;i<DIJoystick_GetNumPhysicalJoysticks();i++)
		{
			item.iItem = iEntryCounter;
			item.pszText = DIJoystick_GetPhysicalJoystickName(i);
			item.cchTextMax = _tcslen(item.pszText);

			for( j=0;j<DIJoystick_GetNumPhysicalJoystickAxes(i);j++)
			{
				ListView_InsertItem(hCtrl,&item );
				ListView_SetItemText(hCtrl,iEntryCounter,1, DIJoystick_GetPhysicalJoystickAxisName(i,j));
				_stprintf(buf, TEXT("%d"), i);
				ListView_SetItemText(hCtrl,iEntryCounter,2, buf);
				_stprintf(buf, TEXT("%d"), j);
				ListView_SetItemText(hCtrl,iEntryCounter++,3, buf);
				item.iItem = iEntryCounter;
			}
		}
	}
}

static void InitializeEffectUI(HWND hwnd)
{
}

static void InitializeControllerMappingUI(HWND hwnd)
{
	int i;
	HWND hCtrl = GetDlgItem(hwnd,IDC_PADDLE);
	HWND hCtrl1 = GetDlgItem(hwnd,IDC_ADSTICK);
	HWND hCtrl2 = GetDlgItem(hwnd,IDC_PEDAL);
	HWND hCtrl3 = GetDlgItem(hwnd,IDC_DIAL);
	HWND hCtrl4 = GetDlgItem(hwnd,IDC_TRACKBALL);
	HWND hCtrl5 = GetDlgItem(hwnd,IDC_LIGHTGUNDEVICE);

	if (hCtrl)
	{
		for (i = 0; i < NUMDEVICES; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxDevice[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxDevice[i].m_pData);
		}
	}
	if (hCtrl1)
	{
		for (i = 0; i < NUMDEVICES; i++)
		{
			ComboBox_InsertString(hCtrl1, i, g_ComboBoxDevice[i].m_pText);
			ComboBox_SetItemData( hCtrl1, i, g_ComboBoxDevice[i].m_pData);
		}
	}
	if (hCtrl2)
	{
		for (i = 0; i < NUMDEVICES; i++)
		{
			ComboBox_InsertString(hCtrl2, i, g_ComboBoxDevice[i].m_pText);
			ComboBox_SetItemData( hCtrl2, i, g_ComboBoxDevice[i].m_pData);
		}
	}
	if (hCtrl3)
	{
		for (i = 0; i < NUMDEVICES; i++)
		{
			ComboBox_InsertString(hCtrl3, i, g_ComboBoxDevice[i].m_pText);
			ComboBox_SetItemData( hCtrl3, i, g_ComboBoxDevice[i].m_pData);
		}
	}
	if (hCtrl4)
	{
		for (i = 0; i < NUMDEVICES; i++)
		{
			ComboBox_InsertString(hCtrl4, i, g_ComboBoxDevice[i].m_pText);
			ComboBox_SetItemData( hCtrl4, i, g_ComboBoxDevice[i].m_pData);
		}
	}
	if (hCtrl5)
	{
		for (i = 0; i < NUMDEVICES; i++)
		{
			ComboBox_InsertString(hCtrl5, i, g_ComboBoxDevice[i].m_pText);
			ComboBox_SetItemData( hCtrl5, i, g_ComboBoxDevice[i].m_pData);
		}
	}
}


static void InitializeBIOSUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd,IDC_BIOS);
	int i = 0;
	TCHAR* t_s;
	if (hCtrl)
	{
		const game_driver *gamedrv = drivers[g_nGame];
		const rom_entry *region, *rom;

		if (g_nGame == GLOBAL_OPTIONS)
		{
			ComboBox_InsertString(hCtrl, i, TEXT("None"));
			ComboBox_SetItemData( hCtrl, i++, "none");
			return;
		}
		if (g_nGame == FOLDER_OPTIONS) //Folder Options
		{
			gamedrv = drivers[g_nFolderGame];
			if (DriverHasOptionalBIOS(g_nFolderGame) == FALSE)
			{
				ComboBox_InsertString(hCtrl, i, TEXT("None"));
				ComboBox_SetItemData( hCtrl, i++, "default");
				return;
			}
			ComboBox_InsertString(hCtrl, i, TEXT("Default"));
			ComboBox_SetItemData( hCtrl, i++, "default");

			for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
			{
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					if (ROMENTRY_ISSYSTEM_BIOS(rom))
					{
						t_s = tstring_from_utf8(ROM_GETNAME(rom));
						if( !t_s )
							return;
						ComboBox_InsertString(hCtrl, i, win_tstring_strdup(t_s));
						ComboBox_SetItemData( hCtrl, i++, ROM_GETNAME(rom));
						free(t_s);
					}
				}
			}
			return;
		}

		if (DriverHasOptionalBIOS(g_nGame) == FALSE)
		{
			ComboBox_InsertString(hCtrl, i, TEXT("None"));
			ComboBox_SetItemData( hCtrl, i++, "none");
			return;
		}
		ComboBox_InsertString(hCtrl, i, TEXT("Default"));
		ComboBox_SetItemData( hCtrl, i++, "default");
/*		thisbios = gamedrv->bios;
		while (!BIOSENTRY_ISEND(thisbios))
		{
			t_s = tstring_from_utf8(thisbios->_description);
			if( !t_s )
				return;
			ComboBox_InsertString(hCtrl, i, win_tstring_strdup(t_s));
			ComboBox_SetItemData( hCtrl, i, thisbios->_name);
			if( strcmp( thisbios->_name, options_get_string(pGameOpts, OPTION_BIOS) ) == 0 )
			{
				ComboBox_SetCurSel( hCtrl, i );
			}
			i++;
			thisbios++;
			free(t_s);
		}
		*/

	}
}

static BOOL SelectEffect(HWND hWnd)
{
	char filename[MAX_PATH];
	BOOL changed = FALSE;

	*filename = 0;
	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_EFFECT_FILES))
	{
		//strip Path and extension
		char buff[MAX_PATH];
		int i, j = 0, k = 0, l = 0;
		for(i=0; i<strlen(filename); i++ )
		{
			if( filename[i] == '\\' )
				j = i;
			if( filename[i] == '.' )
				k = i;
		}
		for(i=j+1;i<k;i++)
		{
			buff[l++] = filename[i];
		}
		buff[l] = '\0';

		if (strcmp(buff, options_get_string(pGameOpts, WINOPTION_EFFECT)))
		{
			options_set_string(pGameOpts, WINOPTION_EFFECT, buff);
			datamap_populate_control(properties_datamap, hWnd, pGameOpts, IDC_EFFECT);
			changed = TRUE;
		}
	}
	return changed;
}

static BOOL ResetEffect(HWND hWnd)
{
	BOOL changed = FALSE;
	const char *new_value = "none";

	if (strcmp(new_value, options_get_string(pGameOpts, WINOPTION_EFFECT)))
	{
		options_set_string(pGameOpts, WINOPTION_EFFECT, new_value);
		datamap_populate_control(properties_datamap, hWnd, pGameOpts, IDC_EFFECT);
		changed = TRUE;
	}
	return changed;
}


void UpdateBackgroundBrush(HWND hwndTab)
{
    // Destroy old brush
    if (hBkBrush)
        DeleteObject(hBkBrush);

    hBkBrush = NULL;

    // Only do this if the theme is active
    if (SafeIsAppThemed())
    {
        RECT rc;
        HDC hDC, hDCMem;
		HBITMAP hBmp, hBmpOld;
        // Get tab control dimensions
        GetWindowRect( hwndTab, &rc);

        // Get the tab control DC
		hDC = GetDC(hwndTab);

        // Create a compatible DC
        hDCMem = CreateCompatibleDC(hDC);
        hBmp = CreateCompatibleBitmap(hDC, 
               rc.right - rc.left, rc.bottom - rc.top);
        hBmpOld = (HBITMAP)(SelectObject(hDCMem, hBmp));

        // Tell the tab control to paint in our DC
        SendMessage(hwndTab, WM_PRINTCLIENT, (WPARAM)(hDCMem), 
           (LPARAM)(PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT));

        // Create a pattern brush from the bitmap selected in our DC
        hBkBrush = CreatePatternBrush(hBmp);

        // Restore the bitmap
        SelectObject(hDCMem, hBmpOld);

        // Cleanup
        DeleteObject(hBmp);
        DeleteDC(hDCMem);
        ReleaseDC(hwndTab, hDC);
    }
}


/* End of source file */
