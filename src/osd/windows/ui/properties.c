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

#include <driver.h>
#include <info.h>
#include "audit.h"
#include "audit32.h"
#include "bitmask.h"
#include "options.h"
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

typedef HANDLE HTHEME;

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef MESS
// done like this until I figure out a better idea
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
static void BeamSelectionChange(HWND hwnd);
static void NumScreensSelectionChange(HWND hwnd);
static void FlickerSelectionChange(HWND hwnd);
static void PrescaleSelectionChange(HWND hwnd);
static void GammaSelectionChange(HWND hwnd);
static void BrightCorrectSelectionChange(HWND hwnd);
static void ContrastSelectionChange(HWND hwnd);
static void PauseBrightSelectionChange(HWND hwnd);
static void FullScreenGammaSelectionChange(HWND hwnd);
static void FullScreenBrightnessSelectionChange(HWND hwnd);
static void FullScreenContrastSelectionChange(HWND hwnd);
static void JDZSelectionChange(HWND hwnd);
static void JSATSelectionChange(HWND hwnd);
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl);
static void VolumeSelectionChange(HWND hwnd);
static void AudioLatencySelectionChange(HWND hwnd);
static void ThreadPrioritySelectionChange(HWND hwnd);
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwRefresh);
static void InitializeDisplayModeUI(HWND hwnd);
static void InitializeSoundUI(HWND hwnd);
static void InitializeSkippingUI(HWND hwnd);
static void InitializeRotateUI(HWND hwnd);
static void UpdateScreenUI(HWND hwnd);
static void InitializeScreenUI(HWND hwnd);
static void UpdateSelectScreenUI(HWND hwnd);
static void InitializeSelectScreenUI(HWND hwnd);
static void InitializeD3DVersionUI(HWND hwnd);
static void InitializeVideoUI(HWND hwnd);
static void InitializeViewUI(HWND hwnd);
static void InitializeRefreshUI(HWND hwnd);
static void UpdateRefreshUI(HWND hwnd);
static void InitializeDefaultInputUI(HWND hWnd);
static void InitializeAnalogAxesUI(HWND hWnd);
static void InitializeEffectUI(HWND hWnd);
static void InitializeBIOSUI(HWND hwnd);
static void InitializeControllerMappingUI(HWND hwnd);
static void PropToOptions(HWND hWnd, options_type *o);
static void OptionsToProp(HWND hWnd, options_type *o);
static void SetPropEnabledControls(HWND hWnd);
static void SelectEffect(HWND hWnd);
static void ResetEffect(HWND hWnd);

static void BuildDataMap(void);
static void ResetDataMap(void);

static BOOL IsControlOptionValue(HWND hDlg,HWND hwnd_ctrl, options_type *opts );

static void UpdateBackgroundBrush(HWND hwndTab);
HBRUSH hBkBrush;
BOOL bThemeActive;

/**************************************************************
 * Local private variables
 **************************************************************/

static options_type  origGameOpts;
static BOOL orig_uses_defaults;
static options_type* pGameOpts = NULL;

static int  g_nGame            = 0;
static int  g_nFolder          = 0;
static int  g_nFolderGame      = 0;
static int  g_nPropertyMode    = 0;
static BOOL g_bInternalSet     = FALSE;
static BOOL g_bUseDefaults     = FALSE;
static BOOL g_bReset           = FALSE;
static int  g_nSampleRateIndex = 0;
static int  g_nVolumeIndex     = 0;
static int  g_nPriorityIndex     = 0;
static int  g_nGammaIndex      = 0;
static int  g_nContrastIndex      = 0;
static int  g_nBrightIndex = 0;
static int  g_nPauseBrightIndex = 0;
static int  g_nBeamIndex       = 0;
static int  g_nFlickerIndex    = 0;
static int  g_nRotateIndex     = 0;
static int  g_nScreenIndex     = 0;
static int  g_nViewIndex     = 0;
static int  g_nSelectScreenIndex     = 0;
static int  g_nInputIndex      = 0;
static int  g_nPrescaleIndex      = 0;
static int  g_nFullScreenGammaIndex = 0;
static int  g_nFullScreenBrightnessIndex = 0;
static int  g_nFullScreenContrastIndex = 0;
static int  g_nEffectIndex     = 0;
static int  g_nBiosIndex     = 0;
static int  g_nJDZIndex = 0;
static int  g_nJSATIndex = 0;
static int  g_nPaddleIndex = 0;
static int  g_nADStickIndex = 0;
static int  g_nPedalIndex = 0;
static int  g_nDialIndex = 0;
static int  g_nTrackballIndex = 0;
static int  g_nLightgunIndex = 0;
static int  g_nVideoIndex = 0;
static int  g_nD3DVersionIndex = 0;
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
	IDC_OLD_TIMING,         HIDC_OLD_TIMING,
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

static struct ComboBoxEffect
{
	const char*	m_pText;
	const char* m_pData;
} g_ComboBoxEffect[] = 
{
	{ "None",                           "none"    },
	{ "25% scanlines",                  "scan25"  },
	{ "50% scanlines",                  "scan50"  },
	{ "75% scanlines",                  "scan75"  },
	{ "75% vertical scanlines",         "scan75v" },
	{ "RGB triad of 16 pixels",         "rgb16"   },
	{ "RGB triad of 6 pixels",          "rgb6"    },
	{ "RGB triad of 4 pixels",          "rgb4"    },
	{ "RGB triad of 4 vertical pixels", "rgb4v"   },
	{ "RGB triad of 3 pixels",          "rgb3"    },
	{ "RGB tiny",                       "rgbtiny" },
	{ "RGB sharp",                      "sharp"   }
};

#define NUMEFFECTS (sizeof(g_ComboBoxEffect) / sizeof(g_ComboBoxEffect[0]))


static struct ComboBoxVideo
{
	const char*	m_pText;
	const char*	m_pData;
} g_ComboBoxVideo[] = 
{
	{ "GDI",                  "gdi"    },
	{ "DirectDraw",           "ddraw"   },
	{ "Direct3D",             "d3d"     },
};
#define NUMVIDEO (sizeof(g_ComboBoxVideo) / sizeof(g_ComboBoxVideo[0]))

static struct ComboBoxD3DVersion
{
	const char*	m_pText;
	const int	m_pData;
} g_ComboBoxD3DVersion[] = 
{
	{ "Version 9",           9    },
	{ "Version 8",           8   },
};

#define NUMD3DVERSIONS (sizeof(g_ComboBoxD3DVersion) / sizeof(g_ComboBoxD3DVersion[0]))

static struct ComboBoxSelectScreen
{
	const char*	m_pText;
	const int	m_pData;
} g_ComboBoxSelectScreen[] = 
{
	{ "Screen 0",             0    },
	{ "Screen 1",             1    },
	{ "Screen 2",             2    },
	{ "Screen 3",             3    },
};
#define NUMSELECTSCREEN (sizeof(g_ComboBoxSelectScreen) / sizeof(g_ComboBoxSelectScreen[0]))

static struct ComboBoxView
{
	const char*	m_pText;
	const char*	m_pData;
} g_ComboBoxView[] = 
{
	{ "Auto",		      "auto"    },
	{ "Standard",         "standard"    }, 
	{ "Pixel Aspect",     "pixel"   }, 
	{ "Cocktail",         "cocktail"     },
};
#define NUMVIEW (sizeof(g_ComboBoxView) / sizeof(g_ComboBoxView[0]))



static struct ComboBoxDevices
{
	const char*	m_pText;
	const char* m_pData;
} g_ComboBoxDevice[] = 
{
	{ "None",                  "none"      },
	{ "Keyboard",              "keyboard"  },
	{ "Mouse",				   "mouse"     },
	{ "Joystick",              "joystick"  },
	{ "Lightgun",              "lightgun"  },
};

#define NUMDEVICES (sizeof(g_ComboBoxDevice) / sizeof(g_ComboBoxDevice[0]))

/***************************************************************
 * Public functions
 ***************************************************************/

typedef HTHEME (WINAPI *OpenThemeProc)(HWND hwnd, LPCWSTR pszClassList);

HMODULE hThemes;
OpenThemeProc fnOpenTheme;
FARPROC fnIsThemed;

void PropertiesInit(void)
{
	hThemes = LoadLibrary("uxtheme.dll");

	if (hThemes)
	{
		fnIsThemed = GetProcAddress(hThemes,"IsAppThemed");
	}
	bThemeActive = FALSE;
}

DWORD GetHelpIDs(void)
{
	return (DWORD) (LPSTR) dwHelpIDs;
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
		if (!bOnlyDefault || g_propSheets[i].bOnDefaultPage)
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

	g_nGame = GLOBAL_OPTIONS;

	/* Get default options to populate property sheets */
	pGameOpts = GetDefaultOptions(-1, FALSE);
	g_bUseDefaults = FALSE;
	/* Stash the result for comparing later */
	CopyGameOptions(pGameOpts,&origGameOpts);
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
	pshead.pszCaption                 = "Default Game";
	pshead.DUMMYUNIONNAME2.nStartPage = 0;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		MessageBox(0, temp, "Error", IDOK);
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
		SetGameUsesDefaults(game_num, g_bUseDefaults);
		/* Stash the result for comparing later */
		CopyGameOptions(pGameOpts,&origGameOpts);
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
		CopyGameOptions(pGameOpts,&origGameOpts);
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
		pshead.pszCaption = ModifyThe(drivers[g_nGame]->description);
	}
	else
	{
		pshead.pszCaption = GetFolderNameByID(g_nFolder);
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
		MessageBox(0, temp, "Error", IDOK);
	}

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
					drv.screen[0].defstate.refresh);
		else
			sprintf(buf,"%d x %d (H) %f Hz",
					drv.screen[0].defstate.visarea.max_x - drv.screen[0].defstate.visarea.min_x + 1,
					drv.screen[0].defstate.visarea.max_y - drv.screen[0].defstate.visarea.min_y + 1,
					drv.screen[0].defstate.refresh);
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

		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE),         GameInfoTitle(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_MANUFACTURED),  GameInfoManufactured(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_STATUS),        GameInfoStatus(g_nGame, FALSE));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_CPU),           GameInfoCPU(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SOUND),         GameInfoSound(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SCREEN),        GameInfoScreen(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_COLORS),        GameInfoColors(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_CLONEOF),       GameInfoCloneOf(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SOURCE),        GameInfoSource(g_nGame));
		
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
		g_nSelectScreenIndex = 0; // Start out wth screen 0
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(g_nGame));
		InitializeOptions(hDlg);
		InitializeMisc(hDlg);

		PopulateControls(hDlg);
		OptionsToProp(hDlg, pGameOpts);
		if( g_nGame >= 0)
		{
			if( !GetGameUsesDefaults(g_nGame) )
			{
				SetGameUsesDefaults( g_nGame, FALSE);
				g_bUseDefaults = FALSE;
			}
			else
			{
				g_bUseDefaults = TRUE;
			}
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
		ReadControls(hDlg);
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

			switch (wID)
			{
			case IDC_SIZES:
			case IDC_FRAMESKIP:
			case IDC_DEFAULT_INPUT:
			case IDC_ROTATE:
			case IDC_VIEW:
			case IDC_SAMPLERATE:
			case IDC_VIDEO_MODE:
				if (wNotifyCode == CBN_SELCHANGE)
				{
					changed = TRUE;
				}
				break;
			case IDC_SCREEN:
				if (wNotifyCode == CBN_SELCHANGE)
				{
					changed = TRUE;
					OptionsToProp( hDlg, pGameOpts );
				}
				break;
			case IDC_SCREENSELECT:
				if (wNotifyCode == CBN_SELCHANGE)
				{
					//First save the changes for this screen index
					PropToOptions( hDlg, pGameOpts );
					nCurSelection = ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_SCREENSELECT));
					if (nCurSelection != CB_ERR)
						g_nSelectScreenIndex = ComboBox_GetItemData(GetDlgItem(hDlg,IDC_SCREENSELECT), nCurSelection);
					changed = TRUE;
					//Load settings for new Index
					OptionsToProp( hDlg, pGameOpts );
				}
				break;
			case IDC_WINDOWED:
				changed = ReadControl(hDlg, wID);
				break;

			case IDC_D3D_VERSION:
				if (wNotifyCode == CBN_SELCHANGE)
				{
					changed = TRUE;
				}
				break;

			case IDC_PADDLE:
			case IDC_ADSTICK:
			case IDC_PEDAL:
			case IDC_DIAL:
			case IDC_TRACKBALL:
			case IDC_LIGHTGUNDEVICE:
				if (wNotifyCode == CBN_SELCHANGE)
				{
					changed = TRUE;
				}
				break;

			case IDC_REFRESH:
				if (wNotifyCode == LBN_SELCHANGE)
				{
					RefreshSelectionChange(hDlg, hWndCtrl);
					changed = TRUE;
				}
				break;

			case IDC_ASPECTRATION:
			case IDC_ASPECTRATIOD:
				if (wNotifyCode == EN_CHANGE)
				{
					if (g_bInternalSet == FALSE)
						changed = TRUE;
				}
				break;	
			case IDC_EFFECT:
				if (wNotifyCode == EN_CHANGE)
				{
					changed = TRUE;
				}
				break;

			case IDC_TRIPLE_BUFFER:
				changed = ReadControl(hDlg, wID);
				break;

			case IDC_ASPECT:
				nCurSelection = Button_GetCheck( GetDlgItem(hDlg, IDC_ASPECT));
				if( g_bAutoAspect[g_nSelectScreenIndex] != nCurSelection )
				{
					changed = TRUE;
					g_bAutoAspect[g_nSelectScreenIndex] = nCurSelection;
				}
				break;

			case IDC_BIOS :
				if (wNotifyCode == CBN_SELCHANGE)
					changed = TRUE;
				break;

			case IDC_SELECT_EFFECT:
				SelectEffect(hDlg);
				break;

			case IDC_RESET_EFFECT:
				ResetEffect(hDlg);
				break;

			case IDC_PROP_RESET:
				if (wNotifyCode != BN_CLICKED)
					break;

				FreeGameOptions(pGameOpts);
				CopyGameOptions(&origGameOpts,pGameOpts);
				if (g_nGame > -1)
					SetGameUsesDefaults(g_nGame,orig_uses_defaults);
				BuildDataMap();
				PopulateControls(hDlg);
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
					if( g_nGame != FOLDER_OPTIONS )
					{
						SetGameUsesDefaults(g_nGame,TRUE);
						if( DriverIsClone(g_nGame) )
						{
							int nParentIndex = -1;
							nParentIndex = GetParentIndex( drivers[g_nGame] );
							if( nParentIndex >= 0)
								CopyGameOptions(GetGameOptions(nParentIndex, g_nFolder), pGameOpts );
							else
								//No Parent found, use source
								CopyGameOptions(GetSourceOptions(g_nGame), pGameOpts);
						}
						else
							//No Parent found, use source
							CopyGameOptions(GetSourceOptions(g_nGame), pGameOpts);
						g_bUseDefaults = TRUE;
					}
					else
					{
						if( g_nFolder == FOLDER_VECTOR)
							CopyGameOptions(GetDefaultOptions(GLOBAL_OPTIONS, TRUE), pGameOpts);
						//Not Vector Folder, but Source Folder of Vector Games
						else if ( DriverIsVector(g_nFolderGame) )
							CopyGameOptions(GetVectorOptions(), pGameOpts);
						// every other folder
						else
							CopyGameOptions(GetDefaultOptions(GLOBAL_OPTIONS, FALSE), pGameOpts);
						g_bUseDefaults = TRUE;
					}
					BuildDataMap();
					PopulateControls(hDlg);
					OptionsToProp(hDlg, pGameOpts);
					SetPropEnabledControls(hDlg);
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

			default:
#ifdef MESS
				if (MessPropertiesCommand(g_nGame, hDlg, wNotifyCode, wID, &changed))
#endif
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
			PropToOptions(hDlg, pGameOpts);
			ReadControls(hDlg);

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
				PopulateControls(hDlg);
				OptionsToProp(hDlg, pGameOpts);
				SetPropEnabledControls(hDlg);
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				break;

			case PSN_APPLY:
				/* Save and apply the options here */
				PropToOptions(hDlg, pGameOpts);
				ReadControls(hDlg);
				if (g_nGame == GLOBAL_OPTIONS)
					pGameOpts = GetDefaultOptions(g_nGame, FALSE);
				else if (g_nGame == FOLDER_OPTIONS)
					pGameOpts = pGameOpts;
					//origGameOpts = GetFolderOptions(g_nFolder);
				else
				{
					SetGameUsesDefaults(g_nGame,g_bUseDefaults);
					orig_uses_defaults = g_bUseDefaults;
					//RS Problem is Data is synced in from disk, and changed games properties are not yet saved
					//pGameOpts = GetGameOptions(g_nGame, -1);
				}

				FreeGameOptions(&origGameOpts);
				CopyGameOptions(pGameOpts,&origGameOpts);

				BuildDataMap();
				PopulateControls(hDlg);
				OptionsToProp(hDlg, pGameOpts);
				SetPropEnabledControls(hDlg);
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				g_bReset = FALSE;
				PropSheet_UnChanged(GetParent(hDlg), hDlg);
				SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
				return PSNRET_NOERROR;

			case PSN_KILLACTIVE:
				/* Save Changes to the options here. */
				PropToOptions(hDlg, pGameOpts);
				ReadControls(hDlg);
				ResetDataMap();
				if (g_nGame > -1)
					SetGameUsesDefaults(g_nGame,g_bUseDefaults);
				SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
				return 1;  

			case PSN_RESET:
				// Reset to the original values. Disregard changes
				FreeGameOptions(pGameOpts);
				CopyGameOptions(&origGameOpts,pGameOpts);
				if (g_nGame > -1)
					SetGameUsesDefaults(g_nGame,orig_uses_defaults);
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
		if( GetControlID(hDlg,(HWND)lParam) < 0)
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
		else if ( (g_nGame >= 0) && (IsControlOptionValue(hDlg,(HWND)lParam, &origGameOpts ) ) )
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

		//	SetBkColor((HDC)wParam,GetSysColor(COLOR_3DFACE) );
			if( hThemes )
			{
				if( fnIsThemed && fnIsThemed() )
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
				SetBkColor((HDC) wParam,GetSysColor(COLOR_3DFACE) );
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
static void PropToOptions(HWND hWnd, options_type *o)
{
	HWND hCtrl;
	HWND hCtrl2;
	HWND hCtrl3;
	int  nIndex;


	if (g_nGame > -1)
		SetGameUsesDefaults(g_nGame,g_bUseDefaults);

	/* resolution size */
	hCtrl = GetDlgItem(hWnd, IDC_SIZES);
	if (hCtrl)
	{
		char buffer[200];

		/* Screen size control */
		nIndex = ComboBox_GetCurSel(hCtrl);
		
		if (nIndex == 0)
			sprintf(buffer, "%dx%d", 0, 0); // auto
		else
		{
			int w, h;

			ComboBox_GetText(hCtrl, buffer, sizeof(buffer)-1);
			if (sscanf(buffer, "%d x %d", &w, &h) == 2)
			{
				sprintf(buffer, "%dx%d", w, h);
			}
			else
			{
				sprintf(buffer, "%dx%d", 0, 0); // auto
			}
		}   

		/* refresh */
		hCtrl = GetDlgItem(hWnd, IDC_REFRESH);
		if (hCtrl)
		{
			int nRefresh = 0;
			char refresh[10];

			nIndex = ComboBox_GetCurSel(hCtrl);
			if (nIndex != CB_ERR)
				nRefresh = ComboBox_GetItemData(hCtrl, nIndex);
			sprintf(refresh, "@%d", nRefresh );
			strcat(buffer, refresh);
		}
		if (strcmp(buffer,"0x0@0") == 0)
			sprintf(buffer,"auto");
		FreeIfAllocated(&o->screen_params[g_nSelectScreenIndex].resolution);
		o->screen_params[g_nSelectScreenIndex].resolution = mame_strdup(buffer);
	}


	hCtrl = GetDlgItem(hWnd, IDC_EFFECT);
	if (hCtrl)
	{
		char buffer[200];
		Static_GetText(hCtrl, buffer, sizeof(buffer)-1);
		FreeIfAllocated(&o->effect);
		o->effect = mame_strdup(buffer);
	}

	/* refresh */
	hCtrl = GetDlgItem(hWnd, IDC_REFRESH);
	if (hCtrl)
	{
		nIndex = ComboBox_GetCurSel(hCtrl);
		
		pGameOpts->gfx_refresh = ComboBox_GetItemData(hCtrl, nIndex);
	}
	/* bios */
	hCtrl = GetDlgItem(hWnd, IDC_BIOS);
	if (hCtrl)
	{
		g_nBiosIndex = ComboBox_GetCurSel(hCtrl);
		FreeIfAllocated(&pGameOpts->bios);
		pGameOpts->bios = mame_strdup((char*)ComboBox_GetItemData(hCtrl, g_nBiosIndex));
	}

	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	hCtrl3 = GetDlgItem(hWnd, IDC_ASPECT);
	if (hCtrl && hCtrl2 && hCtrl3)
	{
		BOOL bAutoAspect = Button_GetCheck(hCtrl3);
		if( bAutoAspect )
		{
			FreeIfAllocated(&o->screen_params[g_nSelectScreenIndex].aspect);
			o->screen_params[g_nSelectScreenIndex].aspect = mame_strdup("auto");
		}
		else
		{
			int n = 0;
			int d = 0;
			char buffer[200];

			Edit_GetText(hCtrl,buffer,sizeof(buffer));
			sscanf(buffer,"%d",&n);

			Edit_GetText(hCtrl2,buffer,sizeof(buffer));
			sscanf(buffer,"%d",&d);

			if (n == 0 || d == 0)
			{
				n = 4;
				d = 3;
			}

			snprintf(buffer,sizeof(buffer),"%d:%d",n,d);
			FreeIfAllocated(&o->screen_params[g_nSelectScreenIndex].aspect);
			o->screen_params[g_nSelectScreenIndex].aspect = mame_strdup(buffer);
		}
	}
	/*analog axes*/
	hCtrl = GetDlgItem(hWnd, IDC_ANALOG_AXES);	
	if (hCtrl)
	{
		int nCount;
		char buffer[200];
		char digital[200];
		int oldJoyId = -1;
		int joyId = 0;
		int axisId = 0;
		BOOL bFirst = TRUE;
		memset(digital,0,sizeof(digital));
		// Get the number of items in the control
		for(nCount=0;nCount < ListView_GetItemCount(hCtrl);nCount++)
		{
			if( ListView_GetCheckState(hCtrl,nCount) )
			{
				//Get The JoyId
				ListView_GetItemText(hCtrl, nCount,2, buffer, sizeof(buffer));
				joyId = atoi(buffer);
				if( oldJoyId != joyId) 
				{
					oldJoyId = joyId;
					//add new JoyId
					if( bFirst )
					{
						strcat(digital, "j");
						bFirst = FALSE;
					}
					else
					{
						strcat(digital, ",j");
					}
					strcat(digital, buffer);
				}
				//Get The AxisId
				ListView_GetItemText(hCtrl, nCount,3, buffer, sizeof(buffer));
				axisId = atoi(buffer);
				strcat(digital,"a");
				strcat(digital, buffer);
			}
		}
		if (mame_stricmp (digital,o->digital) != 0)
		{
			// save the new setting
			FreeIfAllocated(&o->digital);
			o->digital = mame_strdup(digital);
		}
	}
#ifdef MESS
	MessPropToOptions(g_nGame, hWnd, o);
#endif
}

/* Populate controls that are not handled in the DataMap */
static void OptionsToProp(HWND hWnd, options_type* o)
{
	HWND hCtrl;
	HWND hCtrl2;
	char buf[100];
	int  h = 0;
	int  w = 0;
	int  n = 0;
	int  d = 0;
	o->gfx_refresh = 0;
	g_bInternalSet = TRUE;

	/* video */

	/* get desired resolution */
	if (mame_stricmp(o->screen_params[g_nSelectScreenIndex].resolution, "auto") == 0)
	{
		w = h = o->gfx_refresh = 0;
	}
	else
	if (sscanf(o->screen_params[g_nSelectScreenIndex].resolution, "%dx%d@%d", &w, &h, &o->gfx_refresh) < 2)
	{
		w = h = o->gfx_refresh = 0;
	}
	/* Setup Screen*/
	UpdateScreenUI(hWnd );
	/* Setup sizes list based on depth. */
	UpdateDisplayModeUI(hWnd, o->gfx_refresh);
	/* Setup refresh list based on depth. */
	UpdateRefreshUI(hWnd );
	/* Setup Select screen*/
	UpdateSelectScreenUI(hWnd );

	hCtrl = GetDlgItem(hWnd, IDC_ASPECT);
	if (hCtrl)
	{
		Button_SetCheck(hCtrl, g_bAutoAspect[g_nSelectScreenIndex] )	;
	}
	/* Screen size drop down list */
	hCtrl = GetDlgItem(hWnd, IDC_SIZES);
	if (hCtrl)
	{
		if (w == 0 && h == 0)
		{
			/* default to auto */
			ComboBox_SetCurSel(hCtrl, 0);
		}
		else
		{
			/* Select the mode in the list. */
			int nSelection = 0;
			int nCount = 0;

			/* Get the number of items in the control */
			nCount = ComboBox_GetCount(hCtrl);
        
			while (0 < nCount--)
			{
				int nWidth, nHeight;
            
				/* Get the screen size */
				ComboBox_GetLBText(hCtrl, nCount, buf);
            
				if (sscanf(buf, "%d x %d", &nWidth, &nHeight) == 2)
				{
					/* If we match, set nSelection to the right value */
					if (w == nWidth
					&&  h == nHeight)
					{
						nSelection = nCount;
						break;
					}
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
	}

	/* Screen refresh list */
	hCtrl = GetDlgItem(hWnd, IDC_REFRESH);
	if (hCtrl)
	{
		if (o->gfx_refresh == 0)
		{
			/* default to auto */
			ComboBox_SetCurSel(hCtrl, 0);
		}
		else
		{
			/* Select the mode in the list. */
			int nSelection = 0;
			int nCount = 0;

			/* Get the number of items in the control */
			nCount = ComboBox_GetCount(hCtrl);
        
			while (0 < nCount--)
			{
				int nRefresh;
            
				/* Get the screen Refresh */
				nRefresh = ComboBox_GetItemData(hCtrl, nCount);
            
				/* If we match, set nSelection to the right value */
				if (o->gfx_refresh == nRefresh)
				{
					nSelection = nCount;
					break;
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
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
			if( strcmp( cBuffer, pGameOpts->bios ) == 0 )
			{
				g_nBiosIndex = i;
				break;
			}

		}
		ComboBox_SetCurSel(hCtrl, g_nBiosIndex);
	}

	/* Screen select list */
	hCtrl = GetDlgItem(hWnd, IDC_SCREENSELECT);
	if (hCtrl)
	{
		ComboBox_SetCurSel(hCtrl, g_nSelectScreenIndex);
	}
	/* View select list */
	hCtrl = GetDlgItem(hWnd, IDC_VIEW);
	if (hCtrl)
	{
		int nCount = 0;
		nCount = ComboBox_GetCount(hCtrl);
    
		while (0 < nCount--)
		{
        
			/* Get the view name */
			const char* ptr = (const char*)ComboBox_GetItemData(hCtrl, nCount);
        
			/* If we match, set nSelection to the right value */
			if (strcmp (o->screen_params[g_nSelectScreenIndex].view, ptr ) == 0)
				break;
		}
		ComboBox_SetCurSel(hCtrl, nCount);
	}

	hCtrl = GetDlgItem(hWnd, IDC_ASPECT);
	if (hCtrl)
	{
		if( strcmp( o->screen_params[g_nSelectScreenIndex].aspect, "auto") == 0)
		{
			Button_SetCheck(hCtrl, TRUE);
			g_bAutoAspect[g_nSelectScreenIndex] = TRUE;
		}
		else
		{
			Button_SetCheck(hCtrl, FALSE);
			g_bAutoAspect[g_nSelectScreenIndex] = FALSE;
		}
	}
	hCtrl = GetDlgItem(hWnd, IDC_FSGAMMADISP);
	if (hCtrl)
	{
		snprintf(buf,sizeof(buf), "%03.2f", o->gfx_gamma);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_FSBRIGHTNESSDISP);
	if (hCtrl)
	{
		snprintf(buf,sizeof(buf), "%03.2f", o->gfx_brightness);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_FSCONTRASTDISP);
	if (hCtrl)
	{
		snprintf(buf,sizeof(buf), "%03.2f", o->gfx_contrast);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_NUMSCREENSDISP);
	if (hCtrl)
	{
		snprintf(buf,sizeof(buf), "%d", o->numscreens);
		Static_SetText(hCtrl, buf);
	}


	hCtrl = GetDlgItem(hWnd, IDC_EFFECT);
	if (hCtrl)
	{
		Static_SetText(hCtrl, o->effect);
	}


	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	if (hCtrl && hCtrl2)
	{
		n = 0;
		d = 0;
		if(o->screen_params[g_nSelectScreenIndex].aspect)
		{
			if (sscanf(o->screen_params[g_nSelectScreenIndex].aspect, "%d:%d", &n, &d) == 2 && n != 0 && d != 0)
			{
				sprintf(buf, "%d", n);
				Edit_SetText(hCtrl, buf);
				sprintf(buf, "%d", d);
				Edit_SetText(hCtrl2, buf);
			}
			else
			{
				Edit_SetText(hCtrl,  "4");
				Edit_SetText(hCtrl2, "3");
			}
		}
		else
		{
			Edit_SetText(hCtrl,  "4");
			Edit_SetText(hCtrl2, "3");
		}
	}

	/* core video */
	hCtrl = GetDlgItem(hWnd, IDC_GAMMADISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_gamma_correct);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_CONTRASTDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_contrast_correct);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_BRIGHTCORRECTDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_bright_correct);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_PAUSEBRIGHTDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_pause_bright);
		Static_SetText(hCtrl, buf);
	}

	/* Input */
	hCtrl = GetDlgItem(hWnd, IDC_JDZDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_jdz);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_JSATDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_jsat);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_ANALOG_AXES);	
	if (hCtrl)
	{
		int nCount;

		/* Get the number of items in the control */
		char buffer[200];
		char digital[200];
		char *pDest = NULL;
		char *pDest2 = NULL;
		char *pDest3 = NULL;
		int result = 0;
		int result2 = 0;
		int result3 = 0;
		int joyId = 0;
		int axisId = 0;
		memset(digital,0,200);
		// Get the number of items in the control
		for(nCount=0;nCount < ListView_GetItemCount(hCtrl);nCount++)
		{
			//Get The JoyId
			ListView_GetItemText(hCtrl, nCount,2, buffer, sizeof(buffer));
			joyId = atoi(buffer);
			sprintf(digital,"j%s",buffer);
			//First find the JoyId in the saved String
			pDest = strstr (o->digital,digital);
			result = pDest - o->digital + 1;
			if ( pDest != NULL)
			{
				//TrimRight pDest to the first Comma, as there starts a new Joystick
				pDest2 = strchr(pDest,',');
				if( pDest2 != NULL )
				{
					result2 = pDest2 - pDest + 1;
				}
				//Get The AxisId
				ListView_GetItemText(hCtrl, nCount,3, buffer, sizeof(buffer));
				axisId = atoi(buffer);
				sprintf(digital,"a%s",buffer);
				//Now find the AxisId in the saved String
				pDest3 = strstr (pDest,digital);
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
	}
	/* vector */
	hCtrl = GetDlgItem(hWnd, IDC_BEAMDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_beam);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_FLICKERDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_flicker);
		Static_SetText(hCtrl, buf);
	}

	/* sound */
	hCtrl = GetDlgItem(hWnd, IDC_VOLUMEDISP);
	if (hCtrl)
	{
		sprintf(buf, "%ddB", o->attenuation);
		Static_SetText(hCtrl, buf);
	}
	AudioLatencySelectionChange(hWnd);
	
	PrescaleSelectionChange(hWnd);

	ThreadPrioritySelectionChange(hWnd);

	g_bInternalSet = FALSE;

	g_nInputIndex = 0;
	hCtrl = GetDlgItem(hWnd, IDC_DEFAULT_INPUT);	
	if (hCtrl)
	{
		int nCount;

		/* Get the number of items in the control */
		nCount = ComboBox_GetCount(hCtrl);
        
		while (0 < nCount--)
		{
			ComboBox_GetLBText(hCtrl, nCount, buf);

			if (mame_stricmp (buf,o->ctrlr) == 0)
			{
				g_nInputIndex = nCount;
			}
		}

		ComboBox_SetCurSel(hCtrl, g_nInputIndex);
	}

#ifdef MESS
	MessOptionsToProp(g_nGame, hWnd, o);
#endif
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

#ifdef MESS
	MessSetPropEnabledControls(hWnd, pGameOpts);
#endif

	nIndex = g_nGame;
	
	if( ! mame_stricmp(pGameOpts->videomode, "d3d" ) )
	{
		d3d = TRUE;
		ddraw = FALSE;
		gdi = FALSE;
	}else if ( ! mame_stricmp(pGameOpts->videomode, "ddraw" ) )
	{
		d3d = FALSE;
		ddraw = TRUE;
		gdi = FALSE;
	}else
	{
		d3d = FALSE;
		ddraw = FALSE;
		gdi = TRUE;
	}

	in_window = pGameOpts->window_mode;
	Button_SetCheck(GetDlgItem(hWnd, IDC_ASPECT), g_bAutoAspect[g_nSelectScreenIndex] );

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

	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOTEXT), !g_bAutoAspect[g_nSelectScreenIndex]);
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATION), !g_bAutoAspect[g_nSelectScreenIndex]);
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOD), !g_bAutoAspect[g_nSelectScreenIndex]);

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

/**************************************************************
 * Control Helper functions for data exchange
 **************************************************************/

static void AssignSampleRate(HWND hWnd)
{
	switch (g_nSampleRateIndex)
	{
		case 0:  pGameOpts->samplerate = 11025; break;
		case 1:  pGameOpts->samplerate = 22050; break;
		case 2:  pGameOpts->samplerate = 44100; break;
		case 3:  pGameOpts->samplerate = 48000; break;
		default: pGameOpts->samplerate = 44100; break;
	}
}

static void AssignVolume(HWND hWnd)
{
	pGameOpts->attenuation = g_nVolumeIndex - 32;
}

static void AssignPriority(HWND hWnd)
{
	pGameOpts->priority = g_nPriorityIndex - 15;
}

static void AssignBrightCorrect(HWND hWnd)
{
	/* "1.0", 0.5, 2.0 */
	pGameOpts->f_bright_correct = g_nBrightIndex / 20.0 + 0.1;
	
}

static void AssignPauseBright(HWND hWnd)
{
	/* "0.65", 0.5, 2.0 */
	pGameOpts->f_pause_bright = g_nPauseBrightIndex / 20.0 + 0.5;
	
}

static void AssignGamma(HWND hWnd)
{
	pGameOpts->f_gamma_correct = g_nGammaIndex / 20.0 + 0.1;
}

static void AssignContrast(HWND hWnd)
{
	pGameOpts->f_contrast_correct = g_nContrastIndex / 20.0 + 0.1;
}

static void AssignFullScreenGamma(HWND hWnd)
{
	pGameOpts->gfx_gamma = g_nFullScreenGammaIndex / 20.0 + 0.1;
}

static void AssignFullScreenBrightness(HWND hWnd)
{
	pGameOpts->gfx_brightness = g_nFullScreenBrightnessIndex / 20.0 + 0.1;
}

static void AssignFullScreenContrast(HWND hWnd)
{
	pGameOpts->gfx_contrast = g_nFullScreenContrastIndex / 20.0 + 0.1;
}

static void AssignBeam(HWND hWnd)
{
	pGameOpts->f_beam = g_nBeamIndex / 20.0 + 1.0;
}

static void AssignFlicker(HWND hWnd)
{
	pGameOpts->f_flicker = g_nFlickerIndex;
}

static void AssignJDZ(HWND hWnd)
{
	pGameOpts->f_jdz = g_nJDZIndex / 20.0;
}

static void AssignJSAT(HWND hWnd)
{
	pGameOpts->f_jsat = g_nJSATIndex / 20.0;
}

static void AssignRotate(HWND hWnd)
{
	pGameOpts->ror = 0;
	pGameOpts->rol = 0;
	pGameOpts->rotate = 1;
	pGameOpts->auto_ror = 0;
	pGameOpts->auto_rol = 0;

	switch (g_nRotateIndex)
	{
	case 1 : pGameOpts->ror = 1; break;
	case 2 : pGameOpts->rol = 1; break;
	case 3 : pGameOpts->rotate = 0; break;
	case 4 : pGameOpts->auto_ror = 1; break;
	case 5 : pGameOpts->auto_rol = 1; break;
	default : break;
	}
}

static void AssignScreen(HWND hWnd)
{
	const char* ptr = NULL;
	
	if( ComboBox_GetCount(hWnd) > 0 )
	{
		if( g_nScreenIndex != CB_ERR )
			ptr = (const char*)ComboBox_GetItemData(hWnd, g_nScreenIndex);
	}
	FreeIfAllocated(&pGameOpts->screen_params[g_nSelectScreenIndex].screen);
	if (ptr != NULL)
		pGameOpts->screen_params[g_nSelectScreenIndex].screen = mame_strdup(ptr);
	else
		//default to auto
		pGameOpts->screen_params[g_nSelectScreenIndex].screen = mame_strdup("auto");
}

static void AssignView(HWND hWnd)
{
	const char* ptr = NULL;
	int nIndex = CB_ERR;
	if( ComboBox_GetCount(hWnd) > 0 )
	{
		nIndex = ComboBox_GetCurSel(hWnd);
		if( nIndex != CB_ERR )
			ptr = (const char*)ComboBox_GetItemData(hWnd, nIndex);
	
	}

	FreeIfAllocated(&pGameOpts->screen_params[g_nSelectScreenIndex].view);
	if( ptr != NULL )
		pGameOpts->screen_params[g_nSelectScreenIndex].view = mame_strdup(ptr);
	else
		//default to auto
		pGameOpts->screen_params[g_nSelectScreenIndex].view = mame_strdup("auto");
}


static void AssignInput(HWND hWnd)
{
	int new_length;

	FreeIfAllocated(&pGameOpts->ctrlr);

	new_length = ComboBox_GetLBTextLen(hWnd,g_nInputIndex);
	if (new_length == CB_ERR)
	{
		dprintf("error getting text len");
		return;
	}
	pGameOpts->ctrlr = (char *)malloc(new_length + 1);
	ComboBox_GetLBText(hWnd, g_nInputIndex, pGameOpts->ctrlr);
	if (strcmp(pGameOpts->ctrlr,"Standard") == 0)
	{
		// we display Standard, but keep it blank internally
		FreeIfAllocated(&pGameOpts->ctrlr);
		pGameOpts->ctrlr = mame_strdup("");
	}

}

static void AssignVideo(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nVideoIndex);

	FreeIfAllocated(&pGameOpts->videomode);
	if (ptr != NULL)
		pGameOpts->videomode = mame_strdup(ptr);
}



static void AssignD3DVersion(HWND hWnd)
{
	const int ptr = (int)ComboBox_GetItemData(hWnd, g_nD3DVersionIndex);
	pGameOpts->d3d_version = ptr;
}


static void AssignAnalogAxes(HWND hWnd)
{
	int nCheckCounter = 0;
	int nStickCount = 1;
	int nAxisCount = 1;
	int i = 0;
	BOOL bJSet = FALSE;
	BOOL bFirstTime = TRUE;
	char joyname[256];
	char old_joyname[256];
	char mapping[256];
	char j_entry[16];
	char a_entry[16];
	memset(&joyname,0,sizeof(joyname));
	memset(&old_joyname,0,sizeof(old_joyname));
	memset(&mapping,0,sizeof(mapping));
	memset(&a_entry,0,sizeof(a_entry));
	memset(&j_entry,0,sizeof(j_entry));

	FreeIfAllocated(&pGameOpts->digital);
	
	for( i=0;i<ListView_GetItemCount(hWnd);i++)
	{
		//determine Id of selected entry
		ListView_GetItemText(hWnd, i, 0, joyname, 256);
		if( strlen(old_joyname) == 0 )
		{
			//New Stick
			strcpy(old_joyname, joyname);
			sprintf(j_entry,"j%d",nStickCount );
			bJSet = FALSE;
		}
		//Check if Stick has changed
		if( strcmp(joyname, old_joyname ) != 0 )
		{
			strcpy(old_joyname, joyname);
			nStickCount++;
			nAxisCount = 0;
			sprintf(j_entry,"j%d",nStickCount );
			bJSet = FALSE;
		}
		if( ListView_GetCheckState(hWnd, i ) )
		{
			if( bJSet == FALSE )
			{
				if( bFirstTime )
					strcat(mapping,j_entry);
				else
				{
					strcat(mapping,", ");
					strcat(mapping,j_entry);
				}
				bJSet = TRUE;
			}
			nCheckCounter++;
			sprintf(a_entry,"a%d",nAxisCount );
			strcat(mapping,a_entry);
		}
		nAxisCount++;
	}
	if( nCheckCounter == ListView_GetItemCount(hWnd) )
	{
		//all axes on all joysticks are digital
		FreeIfAllocated(&pGameOpts->digital);
		pGameOpts->digital = mame_strdup("all");
	}
	if( nCheckCounter == 0 )
	{
		// no axes are treated as digital, which is the default...
		FreeIfAllocated(&pGameOpts->digital);
		pGameOpts->digital = mame_strdup("");
	}
}

static void AssignBios(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nBiosIndex);

	FreeIfAllocated(&pGameOpts->bios);
	if (ptr != NULL)
		pGameOpts->bios = mame_strdup(ptr);
}

static void AssignPaddle(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nPaddleIndex);
	FreeIfAllocated(&pGameOpts->paddle);
	if (ptr != NULL && strlen(ptr)>0 && strcmp(ptr,"none") != 0 )
		pGameOpts->paddle = mame_strdup(ptr);
	else
		pGameOpts->paddle = mame_strdup("");

}

static void AssignADStick(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nADStickIndex);
	FreeIfAllocated(&pGameOpts->adstick);
	if (ptr != NULL && strlen(ptr)>0 && strcmp(ptr,"none") != 0 )
		pGameOpts->adstick = mame_strdup(ptr);
	else
		pGameOpts->adstick = mame_strdup("");
}

static void AssignPedal(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nPedalIndex);
	FreeIfAllocated(&pGameOpts->pedal);
	if (ptr != NULL && strlen(ptr)>0 && strcmp(ptr,"none") != 0 )
		pGameOpts->pedal = mame_strdup(ptr);
	else
		pGameOpts->pedal = mame_strdup("");
}

static void AssignDial(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nDialIndex);
	FreeIfAllocated(&pGameOpts->dial);
	if (ptr != NULL && strlen(ptr)>0 && strcmp(ptr,"none") != 0 )
		pGameOpts->dial = mame_strdup(ptr);
	else
		pGameOpts->dial = mame_strdup("");
}

static void AssignTrackball(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nTrackballIndex);
	FreeIfAllocated(&pGameOpts->trackball);
	if (ptr != NULL && strlen(ptr)>0 && strcmp(ptr,"none") != 0 )
		pGameOpts->trackball = mame_strdup(ptr);
	else
		pGameOpts->trackball = mame_strdup("");
}

static void AssignLightgun(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nLightgunIndex);
	FreeIfAllocated(&pGameOpts->lightgun_device);
	if (ptr != NULL && strlen(ptr)>0 && strcmp(ptr,"none") != 0 )
		pGameOpts->lightgun_device = mame_strdup(ptr);
	else
		pGameOpts->lightgun_device = mame_strdup("");
}


/************************************************************
 * DataMap initializers
 ************************************************************/

/* Initialize local helper variables */
static void ResetDataMap(void)
{
	int i;
	// add the 0.001 to make sure it truncates properly to the integer
	// (we don't want 35.99999999 to be cut down to 35 because of floating point error)
	g_nPrescaleIndex = pGameOpts->prescale;
	g_nGammaIndex			= (int)((pGameOpts->f_gamma_correct  - 0.1) * 20.0 + 0.001);
	g_nFullScreenGammaIndex	= (int)((pGameOpts->gfx_gamma   - 0.1) * 20.0 + 0.001);
	g_nFullScreenBrightnessIndex= (int)((pGameOpts->gfx_brightness   - 0.1) * 20.0 + 0.001);
	g_nFullScreenContrastIndex	= (int)((pGameOpts->gfx_contrast   - 0.1) * 20.0 + 0.001);
	g_nBrightIndex	= (int)((pGameOpts->f_bright_correct - 0.1) * 20.0 + 0.001);
	g_nContrastIndex	= (int)((pGameOpts->f_contrast_correct - 0.1) * 20.0 + 0.001);
	g_nPauseBrightIndex   	= (int)((pGameOpts->f_pause_bright   - 0.5) * 20.0 + 0.001);
	g_nBeamIndex			= (int)((pGameOpts->f_beam           - 1.0) * 20.0 + 0.001);
	g_nFlickerIndex			= (int)(pGameOpts->f_flicker);
	g_nJDZIndex				= (int)(pGameOpts->f_jdz                    * 20.0 + 0.001);
	g_nJSATIndex				= (int)(pGameOpts->f_jsat                    * 20.0 + 0.001);

	// if no controller type was specified or it was standard
	if (pGameOpts->ctrlr == NULL || mame_stricmp(pGameOpts->ctrlr,"Standard") == 0)
	{
		FreeIfAllocated(&pGameOpts->ctrlr);
		pGameOpts->ctrlr = mame_strdup("");
	}
	g_nViewIndex = 0;
	//TODO HOW DO VIEWS work, where do I get the input for the combo from ???	
	for (i = 0; i < NUMVIDEO; i++)
	{
		if( pGameOpts->screen_params[g_nSelectScreenIndex].view != NULL )
		{
			if (!mame_stricmp(pGameOpts->screen_params[g_nSelectScreenIndex].view, ""))
				g_nViewIndex = i;
		}
	}	
	g_nScreenIndex = 0;
	if (pGameOpts->screen_params[g_nSelectScreenIndex].screen == NULL 
		|| (mame_stricmp(pGameOpts->screen_params[g_nSelectScreenIndex].screen,"") == 0 )
		|| (mame_stricmp(pGameOpts->screen_params[g_nSelectScreenIndex].screen,"auto") == 0 ) )
	{
		FreeIfAllocated(&pGameOpts->screen_params[g_nSelectScreenIndex].screen);
		pGameOpts->screen_params[g_nSelectScreenIndex].screen = mame_strdup("auto");
		g_nScreenIndex = 0;
	}
	else
	{
		//get the selected Index
		int iMonitors;
		DISPLAY_DEVICE dd;
		int i= 0;
		//enumerating the Monitors
		iMonitors = GetSystemMetrics(SM_CMONITORS); // this gets the count of monitors attached
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
		for(i=0; EnumDisplayDevices(NULL, i, &dd, 0); i++)
		{
			if( !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) )
			{
				if ( mame_stricmp(pGameOpts->screen_params[g_nSelectScreenIndex].screen,dd.DeviceName) == 0 )
					g_nScreenIndex = i+1; // To account for "Auto" on first index
			}
		}
	}
	g_nRotateIndex = 0;
	if (pGameOpts->ror == TRUE && pGameOpts->rol == FALSE)
		g_nRotateIndex = 1;
	if (pGameOpts->ror == FALSE && pGameOpts->rol == TRUE)
		g_nRotateIndex = 2;
	if (!pGameOpts->rotate)
		g_nRotateIndex = 3;
	if (pGameOpts->auto_ror)
		g_nRotateIndex = 4;
	if (pGameOpts->auto_rol)
		g_nRotateIndex = 5;

	g_nVolumeIndex = pGameOpts->attenuation + 32;
	g_nPriorityIndex = pGameOpts->priority + 15;
	switch (pGameOpts->samplerate)
	{
		case 11025:  g_nSampleRateIndex = 0; break;
		case 22050:  g_nSampleRateIndex = 1; break;
		case 48000:  g_nSampleRateIndex = 3; break;
		default:
		case 44100:  g_nSampleRateIndex = 2; break;
	}

	g_nEffectIndex = 0;
	for (i = 0; i < NUMEFFECTS; i++)
	{
		if( pGameOpts->effect != NULL )
		{
			if (!mame_stricmp(pGameOpts->effect, g_ComboBoxEffect[i].m_pData))
				g_nEffectIndex = i;
		}
	}
	g_nBiosIndex = 0;

	g_nVideoIndex = 0;
	for (i = 0; i < NUMVIDEO; i++)
	{
		if (!mame_stricmp(pGameOpts->videomode, g_ComboBoxVideo[i].m_pData))
			g_nVideoIndex = i;
	}
	g_nViewIndex = 0;
	for (i = 0; i < NUMVIEW; i++)
	{
		if (!mame_stricmp(pGameOpts->screen_params[g_nSelectScreenIndex].view, g_ComboBoxView[i].m_pData))
			g_nViewIndex = i;
	}
	g_nD3DVersionIndex = 0;
	for (i = 0; i < NUMD3DVERSIONS; i++)
	{
		if (pGameOpts->d3d_version == g_ComboBoxD3DVersion[i].m_pData ) 
			g_nD3DVersionIndex = i;
	}
	g_nPaddleIndex = 0;
	for (i = 0; i < NUMDEVICES; i++)
	{
		if (!mame_stricmp(pGameOpts->paddle, g_ComboBoxDevice[i].m_pData))
			g_nPaddleIndex = i;
	}
	g_nADStickIndex = 0;
	for (i = 0; i < NUMDEVICES; i++)
	{
		if (!mame_stricmp(pGameOpts->adstick, g_ComboBoxDevice[i].m_pData))
			g_nADStickIndex = i;
	}
	g_nPedalIndex = 0;
	for (i = 0; i < NUMDEVICES; i++)
	{
		if (!mame_stricmp(pGameOpts->pedal, g_ComboBoxDevice[i].m_pData))
			g_nPedalIndex = i;
	}
	g_nDialIndex = 0;
	for (i = 0; i < NUMDEVICES; i++)
	{
		if (!mame_stricmp(pGameOpts->dial, g_ComboBoxDevice[i].m_pData))
			g_nDialIndex = i;
	}
	g_nTrackballIndex = 0;
	for (i = 0; i < NUMDEVICES; i++)
	{
		if (!mame_stricmp(pGameOpts->trackball, g_ComboBoxDevice[i].m_pData))
			g_nTrackballIndex = i;
	}
	g_nLightgunIndex = 0;
	for (i = 0; i < NUMDEVICES; i++)
	{
		if (!mame_stricmp(pGameOpts->lightgun_device, g_ComboBoxDevice[i].m_pData))
			g_nLightgunIndex = i;
	}

}

/* Build the control mapping by adding all needed information to the DataMap */
static void BuildDataMap(void)
{
	InitDataMap();


	ResetDataMap();
	/* video */
	DataMapAdd(IDC_D3D_VERSION,   DM_INT,  CT_COMBOBOX, &g_nD3DVersionIndex,       DM_INT, &pGameOpts->d3d_version,     0, 0, AssignD3DVersion);
	DataMapAdd(IDC_VIDEO_MODE,    DM_INT,  CT_COMBOBOX, &g_nVideoIndex,            DM_STRING, &pGameOpts->videomode,     0, 0, AssignVideo);
	DataMapAdd(IDC_PRESCALE,      DM_INT,  CT_SLIDER,   &pGameOpts->prescale,      DM_INT, &pGameOpts->prescale,  0, 0, 0);
	DataMapAdd(IDC_PRESCALEDISP,  DM_NONE, CT_NONE,     NULL,					   DM_INT, &pGameOpts->prescale,  0, 0, 0);
	DataMapAdd(IDC_NUMSCREENS,    DM_INT,  CT_SLIDER,   &pGameOpts->numscreens,    DM_INT, &pGameOpts->numscreens,  0, 0, 0);
	DataMapAdd(IDC_NUMSCREENSDISP,DM_NONE, CT_NONE,     NULL,					   DM_INT, &pGameOpts->numscreens,  0, 0, 0);
	DataMapAdd(IDC_AUTOFRAMESKIP, DM_BOOL, CT_BUTTON,   &pGameOpts->autoframeskip, DM_BOOL, &pGameOpts->autoframeskip, 0, 0, 0);
	DataMapAdd(IDC_FRAMESKIP,     DM_INT,  CT_COMBOBOX, &pGameOpts->frameskip,     DM_INT, &pGameOpts->frameskip,     0, 0, 0);
	DataMapAdd(IDC_WAITVSYNC,     DM_BOOL, CT_BUTTON,   &pGameOpts->wait_vsync,    DM_BOOL, &pGameOpts->wait_vsync,    0, 0, 0);
	DataMapAdd(IDC_TRIPLE_BUFFER, DM_BOOL, CT_BUTTON,   &pGameOpts->use_triplebuf, DM_BOOL, &pGameOpts->use_triplebuf, 0, 0, 0);
	DataMapAdd(IDC_WINDOWED,      DM_BOOL, CT_BUTTON,   &pGameOpts->window_mode,   DM_BOOL, &pGameOpts->window_mode,   0, 0, 0);
	DataMapAdd(IDC_HWSTRETCH,     DM_BOOL, CT_BUTTON,   &pGameOpts->ddraw_stretch, DM_BOOL, &pGameOpts->ddraw_stretch, 0, 0, 0);
	DataMapAdd(IDC_SWITCHRES,     DM_BOOL, CT_BUTTON,   &pGameOpts->switchres,     DM_BOOL, &pGameOpts->switchres,     0, 0, 0);
	DataMapAdd(IDC_MAXIMIZE,      DM_BOOL, CT_BUTTON,   &pGameOpts->maximize,      DM_BOOL, &pGameOpts->maximize,      0, 0, 0);
	DataMapAdd(IDC_KEEPASPECT,    DM_BOOL, CT_BUTTON,   &pGameOpts->keepaspect,    DM_BOOL, &pGameOpts->keepaspect,    0, 0, 0);
	DataMapAdd(IDC_SYNCREFRESH,   DM_BOOL, CT_BUTTON,   &pGameOpts->syncrefresh,   DM_BOOL, &pGameOpts->syncrefresh,   0, 0, 0);
	DataMapAdd(IDC_THROTTLE,      DM_BOOL, CT_BUTTON,   &pGameOpts->throttle,      DM_BOOL, &pGameOpts->throttle,      0, 0, 0);
	DataMapAdd(IDC_FSGAMMA,		  DM_INT,  CT_SLIDER,   &g_nFullScreenGammaIndex,  DM_DOUBLE, &pGameOpts->gfx_gamma,   0, 0, AssignFullScreenGamma);
	DataMapAdd(IDC_FSGAMMADISP,	  DM_NONE, CT_NONE,     NULL,                      DM_DOUBLE, &pGameOpts->gfx_gamma,   0, 0, 0);
	DataMapAdd(IDC_FSBRIGHTNESS,  DM_INT,  CT_SLIDER,   &g_nFullScreenBrightnessIndex,  DM_DOUBLE, &pGameOpts->gfx_brightness,   0, 0, AssignFullScreenBrightness);
	DataMapAdd(IDC_FSBRIGHTNESSDISP, DM_NONE, CT_NONE,     NULL,                      DM_DOUBLE, &pGameOpts->gfx_brightness,   0, 0, 0);
	DataMapAdd(IDC_FSCONTRAST,	  DM_INT,  CT_SLIDER,   &g_nFullScreenContrastIndex,  DM_DOUBLE, &pGameOpts->gfx_contrast,   0, 0, AssignFullScreenContrast);
	DataMapAdd(IDC_FSCONTRASTDISP, DM_NONE, CT_NONE,     NULL,                      DM_DOUBLE, &pGameOpts->gfx_contrast,   0, 0, 0);
	/* pGameOpts->frames_to_display */
	DataMapAdd(IDC_EFFECT,        DM_STRING,  CT_NONE, NULL,       DM_STRING, &pGameOpts->effect,		   0, 0, 0);
	DataMapAdd(IDC_ASPECTRATIOD,  DM_NONE, CT_NONE, &pGameOpts->screen_params[g_nSelectScreenIndex].aspect,    DM_STRING, &pGameOpts->screen_params[g_nSelectScreenIndex].aspect, 0, 0, 0);
	DataMapAdd(IDC_ASPECTRATION,  DM_NONE, CT_NONE, &pGameOpts->screen_params[g_nSelectScreenIndex].aspect,    DM_STRING, &pGameOpts->screen_params[g_nSelectScreenIndex].aspect, 0, 0, 0);
	DataMapAdd(IDC_SIZES,         DM_NONE, CT_NONE, &pGameOpts->screen_params[g_nSelectScreenIndex].resolution,    DM_STRING, &pGameOpts->screen_params[g_nSelectScreenIndex].resolution, 0, 0, 0);

	// direct3d
	DataMapAdd(IDC_D3D_FILTER,    DM_BOOL,  CT_BUTTON, &pGameOpts->d3d_filter,    DM_BOOL, &pGameOpts->d3d_filter, 0, 0, 0);

	/* input */
	DataMapAdd(IDC_DEFAULT_INPUT, DM_INT,  CT_COMBOBOX, &g_nInputIndex,            DM_STRING, &pGameOpts->ctrlr, 0, 0, AssignInput);
	DataMapAdd(IDC_USE_MOUSE,     DM_BOOL, CT_BUTTON,   &pGameOpts->use_mouse,     DM_BOOL, &pGameOpts->use_mouse,     0, 0, 0);   
	DataMapAdd(IDC_JOYSTICK,      DM_BOOL, CT_BUTTON,   &pGameOpts->use_joystick,  DM_BOOL, &pGameOpts->use_joystick,  0, 0, 0);
	DataMapAdd(IDC_JDZ,           DM_INT,  CT_SLIDER,   &g_nJDZIndex,              DM_DOUBLE, &pGameOpts->f_jdz, 0, 0, AssignJDZ);
	DataMapAdd(IDC_JDZDISP,       DM_NONE, CT_NONE,     NULL,  DM_DOUBLE, &pGameOpts->f_jdz, 0, 0, 0);
	DataMapAdd(IDC_JSAT,           DM_INT,  CT_SLIDER,   &g_nJSATIndex,              DM_DOUBLE, &pGameOpts->f_jsat, 0, 0, AssignJSAT);
	DataMapAdd(IDC_JSATDISP,       DM_NONE, CT_NONE,     NULL,  DM_DOUBLE, &pGameOpts->f_jsat, 0, 0, 0);
	DataMapAdd(IDC_STEADYKEY,     DM_BOOL, CT_BUTTON,   &pGameOpts->steadykey,     DM_BOOL, &pGameOpts->steadykey,     0, 0, 0);   
	DataMapAdd(IDC_LIGHTGUN,      DM_BOOL, CT_BUTTON,   &pGameOpts->lightgun,      DM_BOOL, &pGameOpts->lightgun,      0, 0, 0);
	DataMapAdd(IDC_DUAL_LIGHTGUN, DM_BOOL, CT_BUTTON,   &pGameOpts->dual_lightgun, DM_BOOL, &pGameOpts->dual_lightgun,      0, 0, 0);
	DataMapAdd(IDC_RELOAD,        DM_BOOL, CT_BUTTON,   &pGameOpts->offscreen_reload,DM_BOOL,&pGameOpts->offscreen_reload, 0, 0, 0);
	DataMapAdd(IDC_ANALOG_AXES,   DM_NONE, CT_NONE,     &pGameOpts->digital,DM_STRING,&pGameOpts->digital, 0, 0, AssignAnalogAxes);
	/*Controller mapping*/
	DataMapAdd(IDC_PADDLE,        DM_INT, CT_COMBOBOX,  &g_nPaddleIndex,			DM_STRING,&pGameOpts->paddle, 0, 0, AssignPaddle);
	DataMapAdd(IDC_ADSTICK,       DM_INT, CT_COMBOBOX,  &g_nADStickIndex,			DM_STRING,&pGameOpts->adstick, 0, 0, AssignADStick);
	DataMapAdd(IDC_PEDAL,         DM_INT, CT_COMBOBOX,  &g_nPedalIndex,				DM_STRING,&pGameOpts->pedal, 0, 0, AssignPedal);
	DataMapAdd(IDC_DIAL,		  DM_INT, CT_COMBOBOX,  &g_nDialIndex,				DM_STRING,&pGameOpts->dial, 0, 0, AssignDial);
	DataMapAdd(IDC_TRACKBALL,     DM_INT, CT_COMBOBOX,  &g_nTrackballIndex,			DM_STRING,&pGameOpts->trackball, 0, 0, AssignTrackball);
	DataMapAdd(IDC_LIGHTGUNDEVICE,DM_INT, CT_COMBOBOX,  &g_nLightgunIndex,			DM_STRING,&pGameOpts->lightgun_device, 0, 0, AssignLightgun);


	/* core video */
	DataMapAdd(IDC_BRIGHTCORRECT, DM_INT,  CT_SLIDER,   &g_nBrightIndex,    DM_DOUBLE, &pGameOpts->f_bright_correct, 0, 0, AssignBrightCorrect);
	DataMapAdd(IDC_BRIGHTCORRECTDISP,DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_bright_correct, 0, 0, 0);
	DataMapAdd(IDC_PAUSEBRIGHT,   DM_INT,  CT_SLIDER,   &g_nPauseBrightIndex,      DM_DOUBLE, &pGameOpts->f_pause_bright,      0, 0, AssignPauseBright);
	DataMapAdd(IDC_PAUSEBRIGHTDISP,DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_pause_bright, 0, 0, 0);
	DataMapAdd(IDC_ROTATE,        DM_INT,  CT_COMBOBOX, &g_nRotateIndex,           DM_INT, &pGameOpts->ror, 0, 0, AssignRotate);
	DataMapAdd(IDC_FLIPX,         DM_BOOL, CT_BUTTON,   &pGameOpts->flipx,         DM_BOOL, &pGameOpts->flipx,         0, 0, 0);
	DataMapAdd(IDC_FLIPY,         DM_BOOL, CT_BUTTON,   &pGameOpts->flipy,         DM_BOOL, &pGameOpts->flipy,         0, 0, 0);
	DataMapAdd(IDC_SCREEN,        DM_INT,  CT_COMBOBOX, &g_nScreenIndex,		   DM_STRING, &pGameOpts->screen_params[g_nSelectScreenIndex].screen, 0, 0, AssignScreen);
	DataMapAdd(IDC_VIEW,          DM_INT,  CT_COMBOBOX, &g_nViewIndex,		       DM_STRING, &pGameOpts->screen_params[g_nSelectScreenIndex].view, 0, 0, AssignView);
	/* debugres */
	DataMapAdd(IDC_GAMMA,         DM_INT,  CT_SLIDER,   &g_nGammaIndex,            DM_DOUBLE, &pGameOpts->f_gamma_correct, 0, 0, AssignGamma);
	DataMapAdd(IDC_GAMMADISP,     DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_gamma_correct, 0, 0, 0);
	DataMapAdd(IDC_CONTRAST,      DM_INT,  CT_SLIDER,   &g_nContrastIndex,            DM_DOUBLE, &pGameOpts->f_contrast_correct, 0, 0, AssignContrast);
	DataMapAdd(IDC_CONTRASTDISP,  DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_contrast_correct, 0, 0, 0);

	/* vector */
	DataMapAdd(IDC_ANTIALIAS,     DM_BOOL, CT_BUTTON,   &pGameOpts->antialias,     DM_BOOL, &pGameOpts->antialias,     0, 0, 0);
	DataMapAdd(IDC_BEAM,          DM_INT,  CT_SLIDER,   &g_nBeamIndex,             DM_DOUBLE, &pGameOpts->f_beam, 0, 0, AssignBeam);
	DataMapAdd(IDC_BEAMDISP,      DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_beam, 0, 0, 0);
	DataMapAdd(IDC_FLICKER,       DM_INT,  CT_SLIDER,   &g_nFlickerIndex,          DM_DOUBLE, &pGameOpts->f_flicker, 0, 0, AssignFlicker);
	DataMapAdd(IDC_FLICKERDISP,   DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_flicker, 0, 0, 0);

	/* sound */
	DataMapAdd(IDC_SAMPLERATE,    DM_INT,  CT_COMBOBOX, &g_nSampleRateIndex,       DM_INT, &pGameOpts->samplerate, 0, 0, AssignSampleRate);
	DataMapAdd(IDC_SAMPLES,       DM_BOOL, CT_BUTTON,   &pGameOpts->use_samples,   DM_BOOL, &pGameOpts->use_samples,   0, 0, 0);
	DataMapAdd(IDC_USE_SOUND,     DM_BOOL, CT_BUTTON,   &pGameOpts->enable_sound,  DM_BOOL, &pGameOpts->enable_sound,  0, 0, 0);
	DataMapAdd(IDC_VOLUME,        DM_INT,  CT_SLIDER,   &g_nVolumeIndex,           DM_INT, &pGameOpts->attenuation, 0, 0, AssignVolume);
	DataMapAdd(IDC_VOLUMEDISP,    DM_NONE, CT_NONE,  NULL,  DM_INT, &pGameOpts->attenuation, 0, 0, 0);
	DataMapAdd(IDC_AUDIO_LATENCY, DM_INT,  CT_SLIDER,   &pGameOpts->audio_latency, DM_INT, &pGameOpts->audio_latency, 0, 0, 0);
	DataMapAdd(IDC_AUDIO_LATENCY_DISP, DM_NONE,  CT_NONE,   NULL, DM_INT, &pGameOpts->audio_latency, 0, 0, 0);

	/* misc artwork options */
	DataMapAdd(IDC_BACKDROPS,     DM_BOOL, CT_BUTTON,   &pGameOpts->backdrops,     DM_BOOL, &pGameOpts->backdrops,     0, 0, 0);
	DataMapAdd(IDC_OVERLAYS,      DM_BOOL, CT_BUTTON,   &pGameOpts->overlays,      DM_BOOL, &pGameOpts->overlays,      0, 0, 0);
	DataMapAdd(IDC_BEZELS,        DM_BOOL, CT_BUTTON,   &pGameOpts->bezels,        DM_BOOL, &pGameOpts->bezels,        0, 0, 0);
	DataMapAdd(IDC_ARTWORK_CROP,  DM_BOOL, CT_BUTTON,   &pGameOpts->artwork_crop,  DM_BOOL, &pGameOpts->artwork_crop,  0, 0, 0);

	/* misc */
	DataMapAdd(IDC_CHEAT,         DM_BOOL, CT_BUTTON,   &pGameOpts->cheat,         DM_BOOL, &pGameOpts->cheat,         0, 0, 0);
/*	DataMapAdd(IDC_DEBUG,         DM_BOOL, CT_BUTTON,   &pGameOpts->mame_debug,    DM_BOOL, &pGameOpts->mame_debug,    0, 0, 0);*/
	DataMapAdd(IDC_LOG,           DM_BOOL, CT_BUTTON,   &pGameOpts->errorlog,      DM_BOOL, &pGameOpts->errorlog,      0, 0, 0);
	DataMapAdd(IDC_SLEEP,         DM_BOOL, CT_BUTTON,   &pGameOpts->sleep,         DM_BOOL, &pGameOpts->sleep,         0, 0, 0);
	DataMapAdd(IDC_OLD_TIMING,    DM_BOOL, CT_BUTTON,   &pGameOpts->old_timing,    DM_BOOL, &pGameOpts->old_timing,    0, 0, 0);
	DataMapAdd(IDC_HIGH_PRIORITY, DM_INT, CT_SLIDER,   &g_nPriorityIndex, DM_INT, &pGameOpts->priority, 0, 0, AssignPriority);
	DataMapAdd(IDC_HIGH_PRIORITYTXT, DM_NONE,  CT_NONE,   NULL, DM_INT, &pGameOpts->priority, 0, 0, 0);
	DataMapAdd(IDC_SKIP_GAME_INFO, DM_BOOL, CT_BUTTON,  &pGameOpts->skip_gameinfo, DM_BOOL, &pGameOpts->skip_gameinfo, 0, 0, 0);
	DataMapAdd(IDC_BIOS,          DM_INT,  CT_COMBOBOX, &g_nBiosIndex,          DM_STRING, &pGameOpts->bios,        0, 0, AssignBios);
	DataMapAdd(IDC_ENABLE_AUTOSAVE, DM_BOOL, CT_BUTTON,  &pGameOpts->autosave, DM_BOOL, &pGameOpts->autosave, 0, 0, 0);
	DataMapAdd(IDC_MULTITHREAD_RENDERING, DM_BOOL, CT_BUTTON,  &pGameOpts->mt_render, DM_BOOL, &pGameOpts->mt_render, 0, 0, 0);
#ifdef MESS
	DataMapAdd(IDC_SKIP_WARNINGS, DM_BOOL, CT_BUTTON,   &pGameOpts->skip_warnings, DM_BOOL, &pGameOpts->skip_warnings, 0, 0, 0);
	DataMapAdd(IDC_USE_NEW_UI,    DM_BOOL, CT_BUTTON,   &pGameOpts->mess.use_new_ui,DM_BOOL, &pGameOpts->mess.use_new_ui, 0, 0, 0);
#endif

}

BOOL IsControlOptionValue(HWND hDlg,HWND hwnd_ctrl, options_type *opts )
{
	int control_id = GetControlID(hDlg,hwnd_ctrl);

	// certain controls we need to handle specially
	switch (control_id)
	{
	case IDC_ASPECTRATION :
	{
		int n1=0, n2=0;

		sscanf(pGameOpts->screen_params[g_nSelectScreenIndex].aspect,"%i",&n1);
		sscanf(opts->screen_params[g_nSelectScreenIndex].aspect,"%i",&n2);

		return n1 == n2;
	}
	case IDC_ASPECTRATIOD :
	{
		int temp, d1=0, d2=0;

		sscanf(pGameOpts->screen_params[g_nSelectScreenIndex].aspect,"%i:%i",&temp,&d1);
		sscanf(opts->screen_params[g_nSelectScreenIndex].aspect,"%i:%i",&temp,&d2);

		return d1 == d2;
	}
	case IDC_SIZES :
	{
		int x1=0,y1=0,x2=0,y2=0;

		if (strcmp(pGameOpts->screen_params[g_nSelectScreenIndex].resolution,"auto") == 0 &&
			strcmp(opts->screen_params[g_nSelectScreenIndex].resolution,"auto") == 0)
			return TRUE;
		
		sscanf(pGameOpts->screen_params[g_nSelectScreenIndex].resolution,"%d x %d",&x1,&y1);
		sscanf(opts->screen_params[g_nSelectScreenIndex].resolution,"%d x %d",&x2,&y2);

		return x1 == x2 && y1 == y2;		
	}
	case IDC_ROTATE :
	{
		ReadControl(hDlg,control_id);
	
		return pGameOpts->ror == opts->ror &&
			pGameOpts->rol == opts->rol;

	}
	}
	// most options we can compare using data in the data map
	if (IsControlDifferent(hDlg,hwnd_ctrl,pGameOpts,opts))
		return FALSE;

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
	InitializeRefreshUI(hDlg);
	InitializeDisplayModeUI(hDlg);
	InitializeSoundUI(hDlg);
	InitializeSkippingUI(hDlg);
	InitializeRotateUI(hDlg);
	InitializeScreenUI(hDlg);
	InitializeSelectScreenUI(hDlg);
	InitializeDefaultInputUI(hDlg);
	InitializeAnalogAxesUI(hDlg);
	InitializeEffectUI(hDlg);
	InitializeBIOSUI(hDlg);
	InitializeControllerMappingUI(hDlg);
	InitializeD3DVersionUI(hDlg);
	InitializeVideoUI(hDlg);
	InitializeViewUI(hDlg);
}

/* Moved here because it is called in several places */
static void InitializeMisc(HWND hDlg)
{
	Button_Enable(GetDlgItem(hDlg, IDC_JOYSTICK), DIJoystick.Available());

	SendDlgItemMessage(hDlg, IDC_GAMMA, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 58)); /* [0.10, 3.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_NUMSCREENS, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(1, 4)); /* [1, 8] in 1 increments, core says upto 8 is supported, but params can only be specified for 4 */

	SendDlgItemMessage(hDlg, IDC_CONTRAST, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 38)); /* [0.10, 2.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_BRIGHTCORRECT, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 38)); /* [0.10, 2.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_PAUSEBRIGHT, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 30)); /* [0.50, 2.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_FSGAMMA, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 58)); /* [0.10, 3.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_FSBRIGHTNESS, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 38)); /* [0.10, 2.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_FSCONTRAST, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 38)); /* [0.10, 2.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_JDZ, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 20)); /* [0.00, 1.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_JSAT, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 20)); /* [0.00, 1.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_FLICKER, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); /* [0.0, 100.0] in 1.0 increments */

	SendDlgItemMessage(hDlg, IDC_BEAM, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 300)); /* [1.00, 16.00] in .05 increments */

	SendDlgItemMessage(hDlg, IDC_VOLUME, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 32)); /* [-32, 0] */
	SendDlgItemMessage(hDlg, IDC_AUDIO_LATENCY, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(1, 5)); // [1, 5]
	SendDlgItemMessage(hDlg, IDC_PRESCALE, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(1, 10)); // [1, 10] //10 enough ?
	SendDlgItemMessage(hDlg, IDC_HIGH_PRIORITY, TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 16)); // [-15, 1]
}

static void OptOnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	if (hwndCtl == GetDlgItem(hwnd, IDC_FLICKER))
	{
		FlickerSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_GAMMA))
	{
		GammaSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_BRIGHTCORRECT))
	{
		BrightCorrectSelectionChange(hwnd);
	}
	if (hwndCtl == GetDlgItem(hwnd, IDC_CONTRAST))
	{
		ContrastSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_PAUSEBRIGHT))
	{
		PauseBrightSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_FSGAMMA))
	{
		FullScreenGammaSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_FSBRIGHTNESS))
	{
		FullScreenBrightnessSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_FSCONTRAST))
	{
		FullScreenContrastSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_BEAM))
	{
		BeamSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_NUMSCREENS))
	{
		NumScreensSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_FLICKER))
	{
		FlickerSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_VOLUME))
	{
		VolumeSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_JDZ))
	{
		JDZSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_JSAT))
	{
		JSATSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_AUDIO_LATENCY))
	{
		AudioLatencySelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_HIGH_PRIORITY))
	{
		ThreadPrioritySelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_PRESCALE))
	{
		PrescaleSelectionChange(hwnd);
	}
	

}

/* Handle changes to the Beam slider */
static void BeamSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dBeam;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_BEAM, TBM_GETPOS, 0, 0);

	dBeam = nValue / 20.0 + 1.0;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%03.2f", dBeam);
	Static_SetText(GetDlgItem(hwnd, IDC_BEAMDISP), buf);
}

/* Handle changes to the Numscreens slider */
static void NumScreensSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	int iNumScreens;
	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_NUMSCREENS, TBM_GETPOS, 0, 0);

	iNumScreens = nValue;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%d", iNumScreens);
	Static_SetText(GetDlgItem(hwnd, IDC_NUMSCREENSDISP), buf);
	//Also Update the ScreenSelect Combo with the new number of screens
	UpdateSelectScreenUI(hwnd );

}

/* Handle changes to the Flicker slider */
static void FlickerSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dFlicker;
	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_FLICKER, TBM_GETPOS, 0, 0);

	dFlicker = nValue;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%03.2f", dFlicker);
	Static_SetText(GetDlgItem(hwnd, IDC_FLICKERDISP), buf);
}

/* Handle changes to the Gamma slider */
static void GammaSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dGamma;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_GAMMA, TBM_GETPOS, 0, 0);

	dGamma = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%03.2f", dGamma);
	Static_SetText(GetDlgItem(hwnd,	IDC_GAMMADISP), buf);
}

/* Handle changes to the Brightness Correction slider */
static void BrightCorrectSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dValue;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_BRIGHTCORRECT, TBM_GETPOS, 0, 0);

	dValue = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	snprintf(buf, sizeof(buf), "%03.2f", dValue);
	Static_SetText(GetDlgItem(hwnd, IDC_BRIGHTCORRECTDISP), buf);
}

/* Handle changes to the Contrast slider */
static void ContrastSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dContrast;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_CONTRAST, TBM_GETPOS, 0, 0);

	dContrast = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%03.2f", dContrast);
	Static_SetText(GetDlgItem(hwnd, IDC_CONTRASTDISP), buf);
}



/* Handle changes to the Pause Brightness slider */
static void PauseBrightSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dValue;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_PAUSEBRIGHT, TBM_GETPOS, 0, 0);

	dValue = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	snprintf(buf, sizeof(buf), "%03.2f", dValue);
	Static_SetText(GetDlgItem(hwnd, IDC_PAUSEBRIGHTDISP), buf);
}

/* Handle changes to the Fullscreen Gamma slider */
static void FullScreenGammaSelectionChange(HWND hwnd)
{
	char   buf[100];
	int    nValue;
	double dGamma;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_FSGAMMA, TBM_GETPOS, 0, 0);

	dGamma = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf),"%03.2f", dGamma);
	Static_SetText(GetDlgItem(hwnd, IDC_FSGAMMADISP), buf);
}

/* Handle changes to the Fullscreen Brightness slider */
static void FullScreenBrightnessSelectionChange(HWND hwnd)
{
	char   buf[100];
	int    nValue;
	double dBrightness;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_FSBRIGHTNESS, TBM_GETPOS, 0, 0);

	dBrightness = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf),"%03.2f", dBrightness);
	Static_SetText(GetDlgItem(hwnd, IDC_FSBRIGHTNESSDISP), buf);
}

/* Handle changes to the Fullscreen Contrast slider */
static void FullScreenContrastSelectionChange(HWND hwnd)
{
	char   buf[100];
	int    nValue;
	double dContrast;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_FSCONTRAST, TBM_GETPOS, 0, 0);

	dContrast = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf),"%03.2f", dContrast);
	Static_SetText(GetDlgItem(hwnd, IDC_FSCONTRASTDISP), buf);
}

/* Handle changes to the JDZ slider */
static void JDZSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dJDZ;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_JDZ, TBM_GETPOS, 0, 0);

	dJDZ = nValue / 20.0;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%03.2f", dJDZ);
	Static_SetText(GetDlgItem(hwnd, IDC_JDZDISP), buf);
}

/* Handle changes to the JSAT slider */
static void JSATSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dJSAT;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_JSAT, TBM_GETPOS, 0, 0);

	dJSAT = nValue / 20.0;

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%03.2f", dJSAT);
	Static_SetText(GetDlgItem(hwnd, IDC_JSATDISP), buf);
}

/* Handle changes to the Refresh drop down */
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl)
{
	int nCurSelection;

	nCurSelection = ComboBox_GetCurSel(hWndCtrl);
	if (nCurSelection != CB_ERR)
	{
		int nRefresh  = 0;
    
		nRefresh = ComboBox_GetItemData(hWndCtrl, nCurSelection);

		UpdateDisplayModeUI(hWnd, nRefresh);
	}
}

/* Handle changes to the Volume slider */
static void VolumeSelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendDlgItemMessage(hwnd, IDC_VOLUME, TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	snprintf(buf,sizeof(buf), "%ddB", nValue - 32);
	Static_SetText(GetDlgItem(hwnd, IDC_VOLUMEDISP), buf);
}

static void AudioLatencySelectionChange(HWND hwnd)
{
	char buffer[100];
	int value;

	// Get the current value of the control
	value = SendDlgItemMessage(hwnd,IDC_AUDIO_LATENCY, TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	snprintf(buffer,sizeof(buffer),"%d/5",value);
	Static_SetText(GetDlgItem(hwnd,IDC_AUDIO_LATENCY_DISP),buffer);

}

static void PrescaleSelectionChange(HWND hwnd)
{
	char buffer[100];
	int value;

	// Get the current value of the control
	value = SendDlgItemMessage(hwnd,IDC_PRESCALE, TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	snprintf(buffer,sizeof(buffer),"%d",value);
	Static_SetText(GetDlgItem(hwnd,IDC_PRESCALEDISP),buffer);

}

static void ThreadPrioritySelectionChange(HWND hwnd)
{
	char buffer[100];
	int value;

	// Get the current value of the control
	value = SendDlgItemMessage(hwnd,IDC_HIGH_PRIORITY, TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	snprintf(buffer,sizeof(buffer),"%i",value-15);
	Static_SetText(GetDlgItem(hwnd,IDC_HIGH_PRIORITYTXT),buffer);

}

/* Adjust possible choices in the Screen Size drop down */
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwRefresh)
{
	int                   i;
	char                  buf[100];
	char                  buffer[50];
	int                   nPick;
	int                   nCount = 0;
	int                   nSelection = 0;
	DWORD                 w = 0, h = 0;
	HWND                  hCtrl = GetDlgItem(hwnd, IDC_SIZES);
	DEVMODE devmode;

	if (!hCtrl)
		return;

	/* Find out what is currently selected if anything. */
	nPick = ComboBox_GetCurSel(hCtrl);
	if (nPick != 0 && nPick != CB_ERR)
	{
		ComboBox_GetText(GetDlgItem(hwnd, IDC_SIZES), buf, 100);
		if (sscanf(buf, "%lu x %lu", &w, &h) != 2)
		{
			w = 0;
			h = 0;
		}
	}

	/* Remove all items in the list. */
	ComboBox_ResetContent(hCtrl);

	ComboBox_AddString(hCtrl, "Auto");
	//retrieve the screen Infos
	devmode.dmSize = sizeof(devmode);
	ComboBox_GetText(GetDlgItem(hwnd, IDC_SCREEN), buffer, sizeof(buffer)-1);
	for(i=0; EnumDisplaySettings(buffer, i, &devmode); i++)
	{
		if ((devmode.dmBitsPerPel == 32 ) // Only 32 bit depth supported by core
		&&  (devmode.dmDisplayFrequency == dwRefresh || dwRefresh == 0))
		{
			sprintf(buf, "%li x %li", devmode.dmPelsWidth,
									  devmode.dmPelsHeight);

			if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
			{
				ComboBox_AddString(hCtrl, buf);
				nCount++;

				if (w == devmode.dmPelsWidth
				&&  h == devmode.dmPelsHeight)
					nSelection = nCount;
			}
		}
	}
	ComboBox_SetCurSel(hCtrl, nSelection);
}

/* Initialize the Display options to auto mode */
static void InitializeDisplayModeUI(HWND hwnd)
{
	UpdateDisplayModeUI(hwnd, 0);
}

/* Initialize the sound options */
static void InitializeSoundUI(HWND hwnd)
{
	HWND    hCtrl;

	hCtrl = GetDlgItem(hwnd, IDC_SAMPLERATE);
	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "11025");
		ComboBox_AddString(hCtrl, "22050");
		ComboBox_AddString(hCtrl, "44100");
		ComboBox_AddString(hCtrl, "48000");
		ComboBox_SetCurSel(hCtrl, 1);
	}
}

/* Populate the Frame Skipping drop down */
static void InitializeSkippingUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_FRAMESKIP);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Draw every frame");
		ComboBox_AddString(hCtrl, "Skip 1 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 2 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 3 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 4 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 5 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 6 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 7 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 8 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 9 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 10 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 11 of 12 frames");
	}
}

/* Populate the Rotate drop down */
static void InitializeRotateUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_ROTATE);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Default");             // 0
		ComboBox_AddString(hCtrl, "Clockwise");           // 1
		ComboBox_AddString(hCtrl, "Anti-clockwise");      // 2
		ComboBox_AddString(hCtrl, "None");                // 3
		ComboBox_AddString(hCtrl, "Auto clockwise");      // 4
		ComboBox_AddString(hCtrl, "Auto anti-clockwise"); // 5
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

static void InitializeViewUI(HWND hwnd)
{
	HWND    hCtrl;

	hCtrl = GetDlgItem(hwnd, IDC_VIEW);
	if (hCtrl)
	{
		int i;
		for (i = 0; i < NUMVIEW; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxView[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxView[i].m_pData);
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
		ComboBox_ResetContent(hCtrl );
		for (i = 0; i < NUMSELECTSCREEN && i < pGameOpts->numscreens ; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxSelectScreen[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxSelectScreen[i].m_pData);
		}
		// Smaller AMount of screens was selected, so use 0
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

static void UpdateScreenUI(HWND hwnd )
{
	int iMonitors;
	DISPLAY_DEVICE dd;
	int i= 0;
	int nSelection  = 0;
	HWND hCtrl = GetDlgItem(hwnd, IDC_SCREEN);
	if (hCtrl)
	{
		/* Remove all items in the list. */
		ComboBox_ResetContent(hCtrl);
		ComboBox_InsertString(hCtrl, 0, "Auto");
		ComboBox_SetItemData( hCtrl, 0, (const char*)mame_strdup("auto"));

		//Dynamically populate it, by enumerating the Monitors
		iMonitors = GetSystemMetrics(SM_CMONITORS); // this gets the count of monitors attached
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
		for(i=0; EnumDisplayDevices(NULL, i, &dd, 0); i++)
		{
			if( !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) )
			{
				//we have to add 1 to account for the "auto" entry
				ComboBox_InsertString(hCtrl, i+1, mame_strdup(dd.DeviceName));
				ComboBox_SetItemData( hCtrl, i+1, (const char*)mame_strdup(dd.DeviceName));
				if (strcmp (pGameOpts->screen_params[g_nSelectScreenIndex].screen , dd.DeviceName ) == 0)
					nSelection = i+1;
			}
		}
		ComboBox_SetCurSel(hCtrl, nSelection);
	}
}

/* Populate the Screen drop down */
static void InitializeScreenUI(HWND hwnd)
{
	UpdateScreenUI(hwnd );
}

/* Update the refresh drop down */
static void UpdateRefreshUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_REFRESH);
	char buffer[50];

	if (hCtrl)
	{
		int nCount = 0;
		int i;
		DEVMODE devmode;

		/* Remove all items in the list. */
		ComboBox_ResetContent(hCtrl);

		ComboBox_AddString(hCtrl, "Auto");
		ComboBox_SetItemData(hCtrl, nCount++, 0);

		//retrieve the screen Infos
		devmode.dmSize = sizeof(devmode);
		ComboBox_GetText(GetDlgItem(hwnd, IDC_SCREEN), buffer, sizeof(buffer)-1);
		//for(i=0; EnumDisplaySettings(pGameOpts->screen_params[g_nSelectScreenIndex].screen, i, &devmode); i++)
		for(i=0; EnumDisplaySettings(buffer, i, &devmode); i++)
		{
			if (devmode.dmDisplayFrequency >= 10 ) 
			{
				// I have some devmode "vga" which specifes 1 Hz, which is probably bogus, so we filter it out
				char buf[16];

				sprintf(buf, "%li Hz", devmode.dmDisplayFrequency);

				if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
				{
					ComboBox_InsertString(hCtrl, nCount, buf);
					ComboBox_SetItemData(hCtrl, nCount++, devmode.dmDisplayFrequency);
				}
			}
		}
	}
}

/* Populate the refresh drop down */
static void InitializeRefreshUI(HWND hwnd)
{
	UpdateRefreshUI(hwnd);
}
/*Populate the Analog axes Listview*/
static void InitializeAnalogAxesUI(HWND hwnd)
{
	int i=0, j=0, res = 0;
	int iEntryCounter = 0;
	char buf[256];
	LVITEM item;
	LVCOLUMN column;
	HWND hCtrl = GetDlgItem(hwnd, IDC_ANALOG_AXES);
	if( hCtrl )
	{
		//Enumerate the Joystick axes, and add them to the Listview...
		ListView_SetExtendedListViewStyle(hCtrl,LVS_EX_CHECKBOXES );
		//add two Columns...
		column.mask = LVCF_TEXT | LVCF_WIDTH |LVCF_SUBITEM;
		column.pszText = (TCHAR *)"Joystick";
		column.cchTextMax = strlen(column.pszText);
		column.iSubItem = 0;
		column.cx = 100;
		res = ListView_InsertColumn(hCtrl,0, &column );
		column.pszText = (TCHAR *)"Axis";
		column.cchTextMax = strlen(column.pszText);
		column.iSubItem = 1;
		column.cx = 100;
		res = ListView_InsertColumn(hCtrl,1, &column );
		column.pszText = (TCHAR *)"JoystickId";
		column.cchTextMax = strlen(column.pszText);
		column.iSubItem = 2;
		column.cx = 70;
		res = ListView_InsertColumn(hCtrl,2, &column );
		column.pszText = (TCHAR *)"AxisId";
		column.cchTextMax = strlen(column.pszText);
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
			item.cchTextMax = strlen(item.pszText);

			for( j=0;j<DIJoystick_GetNumPhysicalJoystickAxes(i);j++)
			{
				ListView_InsertItem(hCtrl,&item );
				ListView_SetItemText(hCtrl,iEntryCounter,1, DIJoystick_GetPhysicalJoystickAxisName(i,j));
				sprintf(buf, "%d", i);
				ListView_SetItemText(hCtrl,iEntryCounter,2, buf);
				sprintf(buf, "%d", j);
				ListView_SetItemText(hCtrl,iEntryCounter++,3, buf);
				item.iItem = iEntryCounter;
			}
		}
	}
}
/* Populate the Default Input drop down */
static void InitializeDefaultInputUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_DEFAULT_INPUT);

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char *ext;
	char root[256];
	char path[256];

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Standard");

		sprintf (path, "%s\\*.*", GetCtrlrDir());

		hFind = FindFirstFile(path, &FindFileData);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do 
			{
				// copy the filename
				strcpy (root,FindFileData.cFileName);

				// find the extension
				ext = strrchr (root,'.');
				if (ext)
				{
					// check if it's a cfg file
					if (strcmp (ext, ".cfg") == 0)
					{
						// and strip off the extension
						*ext = 0;

						// add it as an option
						ComboBox_AddString(hCtrl, root);
					}
				}
			}
			while (FindNextFile (hFind, &FindFileData) != 0);
			
			FindClose (hFind);
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
	if (hCtrl)
	{
		const game_driver *gamedrv = drivers[g_nGame];
		const bios_entry *thisbios;

		if (g_nGame == GLOBAL_OPTIONS)
		{
			ComboBox_InsertString(hCtrl, i, "None");
			ComboBox_SetItemData( hCtrl, i++, "none");
			return;
		}
		if (g_nGame == FOLDER_OPTIONS) //Folder Options
		{
			gamedrv = drivers[g_nFolderGame];
			if (DriverHasOptionalBIOS(g_nFolderGame) == FALSE)
			{
				ComboBox_InsertString(hCtrl, i, "None");
				ComboBox_SetItemData( hCtrl, i++, "none");
				return;
			}
			ComboBox_InsertString(hCtrl, i, "Default");
			ComboBox_SetItemData( hCtrl, i++, "default");
			thisbios = gamedrv->bios;
			while (!BIOSENTRY_ISEND(thisbios))
			{
				ComboBox_InsertString(hCtrl, i, thisbios->_description);
				ComboBox_SetItemData( hCtrl, i++, thisbios->_name);
				thisbios++;
			}
			return;
		}

		if (DriverHasOptionalBIOS(g_nGame) == FALSE)
		{
			ComboBox_InsertString(hCtrl, i, "None");
			ComboBox_SetItemData( hCtrl, i++, "none");
			return;
		}
		ComboBox_InsertString(hCtrl, i, "Default");
		ComboBox_SetItemData( hCtrl, i++, "default");
		thisbios = gamedrv->bios;
		while (!BIOSENTRY_ISEND(thisbios))
		{
			ComboBox_InsertString(hCtrl, i, thisbios->_description);
			ComboBox_SetItemData( hCtrl, i, thisbios->_name);
			if( strcmp( thisbios->_name, pGameOpts->bios ) == 0 )
			{
				ComboBox_SetCurSel( hCtrl, i );
				g_nBiosIndex = i;
			}
			i++;
			thisbios++;
		}
	}
}

static void SelectEffect(HWND hWnd)
{
	char filename[MAX_PATH];
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
		SetWindowText(GetDlgItem(hWnd, IDC_EFFECT), buff );
	}
}

static void ResetEffect(HWND hWnd)
{
	SetWindowText(GetDlgItem(hWnd, IDC_EFFECT), "none" );
}


void UpdateBackgroundBrush(HWND hwndTab)
{
    // Check if the application is themed
    if (hThemes)
    {
        if(fnIsThemed)
            bThemeActive = fnIsThemed();
    }
    // Destroy old brush
    if (hBkBrush)
        DeleteObject(hBkBrush);

    hBkBrush = NULL;

    // Only do this if the theme is active
    if (bThemeActive)
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
