//============================================================
//
//	mess32ui.c - MESS extensions to src/ui/win32ui.c
//
//============================================================

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

#include "ui/screenshot.h"
#include "ui/bitmask.h"
#include "ui/mame32.h"
#include "ui/resourcems.h"
#include "ui/m32opts.h"
#include "ui/picker.h"
#include "mess.h"
#include "configms.h"
#include "softwarepicker.h"
#include "devview.h"
#include "windows/window.h"
#include "utils.h"
#include "uitext.h"
#include "strconv.h"

#define LOG_SOFTWARE	0

static int requested_device_type(char *tchar);
static void MessCreateCommandLine(char *pCmdLine, core_options *pOpts, const game_driver *gamedrv);
static BOOL MessApproveImageList(HWND hParent, int nDriver);

static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn);

static LPCTSTR SoftwareTabView_GetTabShortName(int tab);
static LPCTSTR SoftwareTabView_GetTabLongName(int tab);
static void SoftwareTabView_OnSelectionChanged(void);
static void SoftwareTabView_OnMoveSize(void);
static void SetupSoftwareTabView(void);

static const char *mess_column_names[] =
{
    "Software",
	"Goodname",
    "Manufacturer",
    "Year",
    "Playable",
	"CRC",
	"SHA-1",
	"MD5"
};

static int *mess_icon_index;

static void InitMessPicker(void);
static void MyFillSoftwareList(int nGame, BOOL bForce);
static void MessUpdateSoftwareList(void);
static void MessOpenOtherSoftware(int iDevice);
static void MessCreateDevice(int iDevice);
static void MessReadMountedSoftware(int nGame);
static void MessRefreshPicker(int nGame);
static BOOL CreateMessIcons(void);

static BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);

static int SoftwarePicker_GetItemImage(HWND hwndPicker, int nItem);
static void SoftwarePicker_LeavingItem(HWND hwndSoftwarePicker, int nItem);
static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem);

static BOOL DevView_GetOpenFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength);
static BOOL DevView_GetCreateFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength);
static void DevView_SetSelectedSoftware(HWND hwndDevView, int nDriverIndex, const struct IODevice *dev, int nID, LPCTSTR pszFilename);
static LPCTSTR DevView_GetSelectedSoftware(HWND hwndDevView, int nDriverIndex, const struct IODevice *dev, int nID, LPTSTR pszBuffer, UINT nBufferLength);

#ifdef MAME_DEBUG
static void MessTestsBegin(void);
#endif /* MAME_DEBUG */

static int s_nGame;
static TCHAR s_szSelectedItem[MAX_PATH];
static BOOL s_bIgnoreSoftwarePickerNotifies;

#include "ui/win32ui.c"

struct deviceentry
{
	int dev_type;
	const char *icon_name;
	const char *dlgname;
};

static const struct PickerCallbacks s_softwareListCallbacks =
{
	SetMessSortColumn,					// pfnSetSortColumn
	GetMessSortColumn,					// pfnGetSortColumn
	SetMessSortReverse,					// pfnSetSortReverse
	GetMessSortReverse,					// pfnGetSortReverse
	NULL,								// pfnSetViewMode
	GetViewMode,						// pfnGetViewMode
	SetMessColumnWidths,				// pfnSetColumnWidths
	GetMessColumnWidths,				// pfnGetColumnWidths
	SetMessColumnOrder,					// pfnSetColumnOrder
	GetMessColumnOrder,					// pfnGetColumnOrder
	SetMessColumnShown,					// pfnSetColumnShown
	GetMessColumnShown,					// pfnGetColumnShown
	NULL,								// pfnGetOffsetChildren

	NULL,								// pfnCompare
	MamePlayGame,						// pfnDoubleClick
	SoftwarePicker_GetItemString,		// pfnGetItemString
	SoftwarePicker_GetItemImage,		// pfnGetItemImage
	SoftwarePicker_LeavingItem,			// pfnLeavingItem
	SoftwarePicker_EnteringItem,		// pfnEnteringItem
	NULL,								// pfnBeginListViewDrag
	NULL,								// pfnFindItemParent
	SoftwarePicker_Idle,				// pfnIdle
	SoftwarePicker_OnHeaderContextMenu,	// pfnOnHeaderContextMenu
	NULL								// pfnOnBodyContextMenu 
};



static const struct TabViewCallbacks s_softwareTabViewCallbacks =
{
	NULL,								// pfnGetShowTabCtrl
	SetCurrentSoftwareTab,				// pfnSetCurrentTab
	GetCurrentSoftwareTab,				// pfnGetCurrentTab
	NULL,								// pfnSetShowTab 
	NULL,								// pfnGetShowTab 

	SoftwareTabView_GetTabShortName,	// pfnGetTabShortName
	SoftwareTabView_GetTabLongName,		// pfnGetTabLongName

	SoftwareTabView_OnSelectionChanged,	// pfnOnSelectionChanged
	SoftwareTabView_OnMoveSize			// pfnOnMoveSize 
};



// ------------------------------------------------------------------------
// Image types
//
// IO_ZIP is used for ZIP files
// IO_ALIAS is used for unknown types
// IO_COUNT is used for bad files
// ------------------------------------------------------------------------

#define IO_ZIP		(IO_COUNT + 0)
#define IO_BAD		(IO_COUNT + 1)
#define IO_UNKNOWN	(IO_COUNT + 2)

// TODO - We need to make icons for Cylinders, Punch Cards, and Punch Tape!
static const struct deviceentry s_devices[] =
{
	{ IO_CARTSLOT,	"roms",		"Cartridge images" },
	{ IO_FLOPPY,	"floppy",	"Floppy disk images" },
	{ IO_HARDDISK,	"hard",		"Hard disk images" },
	{ IO_CYLINDER,	NULL,		"Cylinders" },			
	{ IO_CASSETTE,	NULL,		"Cassette images" },
	{ IO_PUNCHCARD,	NULL,		"Punchcard images" },
	{ IO_PUNCHTAPE,	NULL,		"Punchtape images" },
	{ IO_PRINTER,	NULL,		"Printer Output" },
	{ IO_SERIAL,	NULL,		"Serial Output" },
	{ IO_PARALLEL,	NULL,		"Parallel Output" },
	{ IO_SNAPSHOT,	"snapshot",	"Snapshots" },
	{ IO_QUICKLOAD,	"snapshot",	"Quickloads" },
	{ IO_MEMCARD,	NULL,		"Memory cards" },
	{ IO_CDROM,		NULL,		"CD-ROM images" }
};



static void AssertValidDevice(int d)
{
	assert((sizeof(s_devices) / sizeof(s_devices[0])) == IO_COUNT);
	assert(((d >= 0) && (d < IO_COUNT)) || (d == IO_UNKNOWN) || (d == IO_BAD) || (d == IO_ZIP));
}



static const struct deviceentry *lookupdevice(int d)
{
	assert(d >= 0);
	assert(d < IO_COUNT);
	AssertValidDevice(d);
	return &s_devices[d];
}



// ------------------------------------------------------------------------

static int requested_device_type(char *tchar)
{
	int device = -1;
	int i;

    logerror("Requested device is %s\n", tchar);

	if (*tchar == '-')
	{
		tchar++;

		for (i = 1; i < IO_COUNT; i++)
		{
			if (!mame_stricmp(tchar, device_typename(i)) || !mame_stricmp(tchar, device_brieftypename(i)))
			{
				device = i;
				break;
			}
		}
	}

	return device;
}



// ------------------------------------------------------------------------
// UI
// ------------------------------------------------------------------------

static BOOL CreateMessIcons(void)
{
    int i;

	// create the icon index, if we havn't already
    if (!mess_icon_index)
	{
		mess_icon_index = auto_malloc(driver_get_count() * IO_COUNT * sizeof(*mess_icon_index));
    }

    for (i = 0; i < (driver_get_count() * IO_COUNT); i++)
        mess_icon_index[i] = 0;

	// Associate the image lists with the list view control.
	ListView_SetImageList(GetDlgItem(hMain, IDC_SWLIST), hSmall, LVSIL_SMALL);
	ListView_SetImageList(GetDlgItem(hMain, IDC_SWLIST), hLarge, LVSIL_NORMAL);
	return TRUE;
}



static int GetMessIcon(int nGame, int nSoftwareType)
{
    int the_index;
    int nIconPos = 0;
    HICON hIcon = 0;
    const game_driver *drv;
    char buffer[32];
	const char *iconname;

	assert(nGame >= 0);
	assert(nGame < driver_get_count());

    if ((nSoftwareType >= 0) && (nSoftwareType < IO_COUNT))
	{
		iconname = device_brieftypename(nSoftwareType);
        the_index = (nGame * IO_COUNT) + nSoftwareType;

        nIconPos = mess_icon_index[the_index];
        if (nIconPos >= 0)
		{
			for (drv = drivers[nGame]; drv; drv = driver_get_clone(drv))
			{
				_snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s/%s", drv->name, iconname);
				hIcon = LoadIconFromFile(buffer);
				if (hIcon)
					break;
			}

			if (hIcon)
			{
				nIconPos = ImageList_AddIcon(hSmall, hIcon);
				ImageList_AddIcon(hLarge, hIcon);
				if (nIconPos != -1)
					mess_icon_index[the_index] = nIconPos;
			}
        }
    }
    return nIconPos;
}



static void MessHashErrorProc(const char *message)
{
	SetStatusBarTextF(0, "Hash file: %s", message);
}



static BOOL AddSoftwarePickerDirs(HWND hwndPicker, LPCTSTR pszDirectories, LPCTSTR pszSubDir)
{
	LPCTSTR s;
	LPTSTR pszNewString;
	TCHAR cSeparator = ';';
	int nLength;

	do
	{
		s = pszDirectories;
		while(*s && (*s != cSeparator))
			s++;

		nLength = s - pszDirectories;
		if (nLength > 0)
		{
			pszNewString = (LPTSTR) alloca((nLength + 1 + (pszSubDir ? _tcslen(pszSubDir) + 1 : 0))
				* sizeof(TCHAR));
			memcpy(pszNewString, pszDirectories, nLength * sizeof(TCHAR));
			pszNewString[nLength] = '\0';

			if (pszSubDir)
			{
				pszNewString[nLength++] = '\\';
				_tcscpy(&pszNewString[nLength], pszSubDir);
			}

			if (!SoftwarePicker_AddDirectory(hwndPicker, pszNewString))
				return FALSE;
		}
		pszDirectories = s + 1;
	}
	while(*s);
	return TRUE;
}



static void MyFillSoftwareList(int nGame, BOOL bForce)
{
	const game_driver *drv;
	HWND hwndSoftwarePicker;
	HWND hwndSoftwareDevView;
	
	hwndSoftwarePicker = GetDlgItem(hMain, IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(hMain, IDC_SWDEVVIEW);

	// Set up the device view
	DevView_SetDriver(hwndSoftwareDevView, nGame);

	if (!bForce && (s_nGame == nGame))
		return;
	s_nGame = nGame;

	// Set up the software picker
	SoftwarePicker_Clear(hwndSoftwarePicker);
	drv = drivers[nGame];
	SoftwarePicker_SetDriver(hwndSoftwarePicker, drv);
	while(drv)
	{
		AddSoftwarePickerDirs(hwndSoftwarePicker, GetSoftwareDirs(), drv->name);
		drv = mess_next_compatible_driver(drv);
	}
	AddSoftwarePickerDirs(hwndSoftwarePicker, GetExtraSoftwarePaths(nGame), NULL);
}



static void MessUpdateSoftwareList(void)
{
	MyFillSoftwareList(Picker_GetSelectedItem(hwndList), TRUE);
}



static BOOL MessApproveImageList(HWND hParent, int nGame)
{
	int i, nPos;
	const struct IODevice *pDevice;
	const game_driver *pDriver;
	char szMessage[256];
	LPCTSTR pszMessage;
	LPCTSTR pszSoftware;
	BOOL bResult = FALSE;

	begin_resource_tracking();

	pDriver = drivers[nGame];
	pDevice = devices_allocate(pDriver);

	nPos = 0;
	while(pDevice->type < IO_COUNT)
	{
		// confirm any mandatory devices are loaded
		if (pDevice->must_be_loaded)
		{
			for (i = 0; i < pDevice->count; i++)
			{
				pszSoftware = GetSelectedSoftware(nGame, &pDevice->devclass, i);
				if (!pszSoftware || !*pszSoftware)
				{
					snprintf(szMessage, sizeof(szMessage) / sizeof(szMessage[0]),
						"System '%s' requires that device %s must have an image to load\n",
						pDriver->description,
						device_typename(pDevice->type));
					goto done;
				}
			}
		}

		nPos += pDevice->count;
		pDevice++;
	}
	bResult = TRUE;

done:
	if (!bResult)
	{
		pszMessage = tstring_from_utf8(szMessage);
		MessageBox(hParent, pszMessage, MAME32NAME, MB_OK);
		free((void *) pszMessage);
	}

	end_resource_tracking();
	return bResult;
}



// this is a wrapper call to wrap the idiosycracies of SetSelectedSoftware()
static void InternalSetSelectedSoftware(int nGame, const device_class *devclass, int device_inst, const char *pszSoftware)
{
	if (!pszSoftware)
		pszSoftware = TEXT("");

	// only call SetSelectedSoftware() if this value is different
	if (strcmp(GetSelectedSoftware(nGame, devclass, device_inst), pszSoftware))
	{
		SetSelectedSoftware(nGame, devclass, device_inst, pszSoftware);
		SetGameUsesDefaults(nGame, FALSE);
		SaveGameOptions(nGame);
	}
}



// Places the specified image in the specified slot; nID = -1 means don't matter
static void MessSpecifyImage(int nGame, const device_class *devclass, int nID, LPCTSTR pszFilename)
{
	const struct IODevice *devices;
	const struct IODevice *dev;
	const char *s;
	int i;

	begin_resource_tracking();
	devices = devices_allocate(devclass->gamedrv);

	for (dev = devices; dev->type < IO_COUNT; dev++)
	{
		if (dev->devclass.get_info == devclass->get_info)
			break;
	}

	if (LOG_SOFTWARE)
	{
		dprintf("MessSpecifyImage(): nID=%d pszFilename='%s'\n", nID, pszFilename);
	}

	if (nID < 0)
	{
		// special case; first try to find existing image
		for (i = 0; i < dev->count; i++)
		{
			s = GetSelectedSoftware(nGame, &dev->devclass, i);
			if (s && !_tcsicmp(s, pszFilename))
			{
				nID = i;
				break;
			}
		}
	}

	if (nID < 0)
	{
		// still not found?  locate an empty slot
		for (i = 0; i < dev->count; i++)
		{
			s = GetSelectedSoftware(nGame, &dev->devclass, i);
			if (!s || !*s || !_tcsicmp(s, pszFilename))
			{
				nID = i;
				break;
			}
		}
	}

	if (nID >= 0)
		InternalSetSelectedSoftware(nGame, &dev->devclass, nID, pszFilename);
	else if (LOG_SOFTWARE)
		dprintf("MessSpecifyImage(): Failed to place image '%s'\n", pszFilename);

	end_resource_tracking();
}



static void MessRemoveImage(int nGame, device_class devclass, LPCTSTR pszFilename)
{
	int i, nPos;
	const struct IODevice *devices;
	const struct IODevice *dev;

	if (devclass.gamedrv)
	{
		begin_resource_tracking();
		devices = devices_allocate(devclass.gamedrv);

		nPos = 0;
		for (dev = devices; dev->type < IO_COUNT; dev++)
		{
			if (dev->devclass.get_info == devclass.get_info)
				break;
			nPos += dev->count;
		}

		if (dev->type < IO_COUNT)
		{
			for (i = 0; i < dev->count; i++)
			{
				if (!_tcsicmp(pszFilename, GetSelectedSoftware(nGame, &dev->devclass, i)))
				{
					MessSpecifyImage(nGame, &devclass, i, NULL);
					break;
				}
			}
		}

		end_resource_tracking();
	}
}



static void MessReadMountedSoftware(int nGame)
{
	// First read stuff into device view
	DevView_Refresh(GetDlgItem(hMain, IDC_SWDEVVIEW));

	// Now read stuff into picker
	MessRefreshPicker(nGame);
}



static void MessRefreshPicker(int nGame)
{
	HWND hwndSoftware;
	int i, id;
	LVFINDINFO lvfi;
	const game_driver *pDriver;
	const struct IODevice *pDeviceList;
	const struct IODevice *pDevice;
	LPCTSTR pszSoftware;

	begin_resource_tracking();

	hwndSoftware = GetDlgItem(hMain, IDC_SWLIST);
	pDriver = drivers[nGame];
	pDeviceList = devices_allocate(pDriver);
	if (!pDeviceList)
		goto done;

	s_bIgnoreSoftwarePickerNotifies = TRUE;

	// Now clear everything out; this may call back into us but it should not
	// be problematic
	ListView_SetItemState(hwndSoftware, -1, 0, LVIS_SELECTED);

	for (pDevice = pDeviceList; pDevice->type < IO_COUNT; pDevice++)
	{
		for (id = 0; id < pDevice->count; id++)
		{
			pszSoftware = GetSelectedSoftware(nGame, &pDevice->devclass, id);
			if (pszSoftware && *pszSoftware)
			{
				i = SoftwarePicker_LookupIndex(hwndSoftware, pszSoftware);
				if (i < 0)
				{
					SoftwarePicker_AddFile(hwndSoftware, pszSoftware);
					i = SoftwarePicker_LookupIndex(hwndSoftware, pszSoftware);
				}
				if (i >= 0)
				{
					memset(&lvfi, 0, sizeof(lvfi));
					lvfi.flags = LVFI_PARAM;
					lvfi.lParam = i;
					i = ListView_FindItem(hwndSoftware, -1, &lvfi);
					ListView_SetItemState(hwndSoftware, i, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
		}
	}


done:
	end_resource_tracking();
	s_bIgnoreSoftwarePickerNotifies = FALSE;
}



static void InitMessPicker(void)
{
	struct PickerOptions opts;
	HWND hwndSoftware;

	s_nGame = -1;
	hwndSoftware = GetDlgItem(hMain, IDC_SWLIST);

	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_softwareListCallbacks;
	opts.nColumnCount = MESS_COLUMN_MAX;
	opts.ppszColumnNames = mess_column_names;
	SetupSoftwarePicker(hwndSoftware, &opts);

	SetWindowLong(hwndSoftware, GWL_STYLE, GetWindowLong(hwndSoftware, GWL_STYLE)
		| LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED);

	SetupSoftwareTabView();

	{
		static const struct DevViewCallbacks s_devViewCallbacks =
		{
			DevView_GetOpenFileName,
			DevView_GetCreateFileName,
			DevView_SetSelectedSoftware,
			DevView_GetSelectedSoftware
		};
		DevView_SetCallbacks(GetDlgItem(hMain, IDC_SWDEVVIEW), &s_devViewCallbacks);
	}
}



static void MessCreateCommandLine(char *pCmdLine, core_options *pOpts, const game_driver *pDriver)
{
	sprintf(&pCmdLine[strlen(pCmdLine)], " -writeconfig");
}



// ------------------------------------------------------------------------
// Open others dialog
// ------------------------------------------------------------------------

typedef struct
{
	const struct IODevice *dev;
    const char *ext;
} mess_image_type;

static BOOL CommonFileImageDialog(char *the_last_directory, common_file_dialog_proc cfd, char *filename, mess_image_type *imagetypes)
{
    BOOL success;
    OPENFILENAME of;
    char szFilter[2048];
    LPSTR s;
	const char *typname;
    int i;

	s = szFilter;
    *filename = 0;

    // Common image types
    strcpy(s, "Common image types");
    s += strlen(s) + 1;
    for (i = 0; imagetypes[i].ext; i++)
	{
		assert(!IsBadStringPtr(imagetypes[i].ext, ~0));
        *(s++) = '*';
        *(s++) = '.';
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = ';';
    }
    *(s++) = '\0';

    // All files
    strcpy(s, "All files (*.*)");
    s += strlen(s) + 1;
    strcpy(s, "*.*");
    s += strlen(s) + 1;

    // The others
    for (i = 0; imagetypes[i].ext; i++)
	{
		if (!imagetypes[i].dev)
			typname = "Compressed images";
		else
			typname = lookupdevice(imagetypes[i].dev->type)->dlgname;

        strcpy(s, typname);
        //strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        strcpy(s, " (*.");
        //strcpy(s, " files (*.");
        s += strlen(s);
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = ')';
        *(s++) = '\0';
        *(s++) = '*';
        *(s++) = '.';
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = '\0';
    }
    *(s++) = '\0';

    of.lStructSize = sizeof(of);
    of.hwndOwner = hMain;
    of.hInstance = NULL;
    of.lpstrFilter = szFilter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 1;
    of.lpstrFile = filename;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = NULL;
    of.nMaxFileTitle = 0;
    of.lpstrInitialDir = last_directory;
    of.lpstrTitle = NULL;
    of.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    of.nFileOffset = 0;
    of.nFileExtension = 0;
    of.lpstrDefExt = "rom";
    of.lCustData = 0;
    of.lpfnHook = NULL;
    of.lpTemplateName = NULL;

    success = cfd(&of);
    if (success)
    {
        //GetDirectory(filename,last_directory,sizeof(last_directory));
    }

    return success;
}



/* Specify IO_COUNT for type if you want all types */
static void SetupImageTypes(int nDriver, mess_image_type *types, int count, BOOL bZip, int type)
{
    const struct IODevice *dev;
    int num_extensions = 0;

	memset(types, 0, sizeof(*types) * count);
    count--;

    if (bZip)
	{
		types[num_extensions].ext = "zip";
		types[num_extensions].dev = NULL;
		num_extensions++;
    }

	dev = devices_allocate(drivers[nDriver]);
	if (!dev)
		return;

	while(dev->type < IO_COUNT)
	{
		if (dev->type != IO_PRINTER)
		{
			const char *ext = dev->file_extensions;
			if (ext)
			{
				while(*ext)
				{
					if ((type == IO_COUNT) || (type == dev->type))
					{
						if (num_extensions < count)
						{
							types[num_extensions].dev = dev;
							types[num_extensions].ext = ext;
							num_extensions++;
						}
					}
					ext += strlen(ext) + 1;
				}
			}
		}
		dev++;
    }
}



static void MessSetupDevice(common_file_dialog_proc cfd, int iDevice)
{
	char filename[MAX_PATH];
	mess_image_type imagetypes[64];
	int nGame;

	begin_resource_tracking();

	nGame = Picker_GetSelectedItem(hwndList);
	SetupImageTypes(nGame, imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, iDevice);

	if (CommonFileImageDialog(last_directory, cfd, filename, imagetypes))
	{
		// TODO - this should go against InternalSetSelectedSoftware()
		SoftwarePicker_AddFile(GetDlgItem(hMain, IDC_SWLIST), filename);
	}
	end_resource_tracking();
}



static void MessOpenOtherSoftware(int iDevice)
{
	MessSetupDevice(GetOpenFileName, iDevice);
}



static void MessCreateDevice(int iDevice)
{
	MessSetupDevice(GetSaveFileName, iDevice);
}



static BOOL DevView_GetOpenFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength)
{
	BOOL bResult;
	mess_image_type imagetypes[64];

	begin_resource_tracking();

	SetupImageTypes(Picker_GetSelectedItem(hwndList), imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, dev->type);
	bResult = CommonFileImageDialog(last_directory, GetOpenFileName, pszFilename, imagetypes);

	end_resource_tracking();
	return bResult;
}



static BOOL DevView_GetCreateFileName(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength)
{
	BOOL bResult;
	mess_image_type imagetypes[64];

	begin_resource_tracking();

	SetupImageTypes(Picker_GetSelectedItem(hwndList), imagetypes, sizeof(imagetypes) / sizeof(imagetypes[0]), TRUE, dev->type);
	bResult = CommonFileImageDialog(last_directory, GetSaveFileName, pszFilename, imagetypes);

	end_resource_tracking();
	return bResult;
}



static void DevView_SetSelectedSoftware(HWND hwndDevView, int nGame,
	const struct IODevice *dev, int nID, LPCTSTR pszFilename)
{
	MessSpecifyImage(nGame, &dev->devclass, nID, pszFilename);
	MessRefreshPicker(nGame);
}



static LPCTSTR DevView_GetSelectedSoftware(HWND hwndDevView, int nDriverIndex,
	const struct IODevice *dev, int nID, LPTSTR pszBuffer, UINT nBufferLength)
{
	return GetSelectedSoftware(nDriverIndex, &dev->devclass, nID);
}



// ------------------------------------------------------------------------
// Software List Class
// ------------------------------------------------------------------------

static int LookupIcon(const char *icon_name)
{
	int i;
	for (i = 0; g_iconData[i].icon_name; i++)
	{
		if (!strcmp(g_iconData[i].icon_name, icon_name))
			return i;
	}
	return -1;
}



static int SoftwarePicker_GetItemImage(HWND hwndPicker, int nItem)
{
    int nType;
    int nIcon;
	int nGame;
	const char *icon_name;
	HWND hwndGamePicker;
	HWND hwndSoftwarePicker;

	hwndGamePicker = GetDlgItem(hMain, IDC_LIST);
	hwndSoftwarePicker = GetDlgItem(hMain, IDC_SWLIST);
	nGame = Picker_GetSelectedItem(hwndGamePicker);
    nType = SoftwarePicker_GetImageType(hwndSoftwarePicker, nItem);
    nIcon = GetMessIcon(nGame, nType);
    if (!nIcon)
	{
		if (nType < 0)
			nType = IO_BAD;
		switch(nType)
		{
			case IO_UNKNOWN:
				// Unknowns 
				nIcon = FindIconIndex(IDI_WIN_UNKNOWN);
				break;

			case IO_BAD:
			case IO_ZIP:
				// Bad files
				nIcon = FindIconIndex(IDI_WIN_REDX);
				break;

			default:
				icon_name = lookupdevice(nType)->icon_name;
				if (!icon_name)
					icon_name = device_typename(nType);
				nIcon = LookupIcon(icon_name);
				if (nIcon < 0)
					nIcon = LookupIcon("unknown");
				break;
		}
    }
    return nIcon;
}



static void SoftwarePicker_LeavingItem(HWND hwndSoftwarePicker, int nItem)
{
	int nGame;
	device_class devclass;
	LPCTSTR pszFullName;

	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		nGame = Picker_GetSelectedItem(hwndList);
		devclass = SoftwarePicker_LookupDevice(hwndSoftwarePicker, nItem);
		pszFullName = SoftwarePicker_LookupFilename(hwndSoftwarePicker, nItem);

		MessRemoveImage(nGame, devclass, pszFullName);
	}
}



static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem)
{
	LPCTSTR pszFullName;
	LPCTSTR pszName;
	LPTSTR s;
	int nGame;
	device_class devclass;

	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		nGame = Picker_GetSelectedItem(hwndList);

		// Get the fullname and partialname for this file
		pszFullName = SoftwarePicker_LookupFilename(hwndSoftwarePicker, nItem);
		s = strrchr(pszFullName, '\\');
		pszName = s ? s + 1 : pszFullName;

		// Do the dirty work
		devclass = SoftwarePicker_LookupDevice(hwndSoftwarePicker, nItem);
		if (!devclass.gamedrv)
			return;
		MessSpecifyImage(nGame, &devclass, -1, pszFullName);

		// Set up s_szSelecteItem, for the benefit of UpdateScreenShot()
		strncpyz(s_szSelectedItem, pszName, sizeof(s_szSelectedItem) / sizeof(s_szSelectedItem[0]));
		s = strrchr(s_szSelectedItem, '.');
		if (s)
			*s = '\0';

		UpdateScreenShot();
	}
}



// ------------------------------------------------------------------------
// Header context menu stuff
// ------------------------------------------------------------------------

static HWND MyColumnDialogProc_hwndPicker;
static int *MyColumnDialogProc_order;
static int *MyColumnDialogProc_shown;

static void MyGetRealColumnOrder(int *order)
{
	int i, nColumnCount;
	nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);
	for (i = 0; i < nColumnCount; i++)
		order[i] = Picker_GetRealColumnFromViewColumn(MyColumnDialogProc_hwndPicker, i);
}



static void MyGetColumnInfo(int *order, int *shown)
{
	const struct PickerCallbacks *pCallbacks;
	pCallbacks = Picker_GetCallbacks(MyColumnDialogProc_hwndPicker);
	pCallbacks->pfnGetColumnOrder(order);
	pCallbacks->pfnGetColumnShown(shown);
}



static void MySetColumnInfo(int *order, int *shown)
{
	const struct PickerCallbacks *pCallbacks;
	pCallbacks = Picker_GetCallbacks(MyColumnDialogProc_hwndPicker);
	pCallbacks->pfnSetColumnOrder(order);
	pCallbacks->pfnSetColumnShown(shown);
}



static INT_PTR CALLBACK MyColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);
	LPCTSTR *ppszColumnNames = Picker_GetColumnNames(MyColumnDialogProc_hwndPicker);
	return InternalColumnDialogProc(hDlg, Msg, wParam, lParam, nColumnCount,
		MyColumnDialogProc_shown, MyColumnDialogProc_order, ppszColumnNames,
		MyGetRealColumnOrder, MyGetColumnInfo, MySetColumnInfo);
}



static BOOL RunColumnDialog(HWND hwndPicker)
{
	BOOL bResult;
	int nColumnCount;

	MyColumnDialogProc_hwndPicker = hwndPicker;
	nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);

	MyColumnDialogProc_order = alloca(nColumnCount * sizeof(int));
	MyColumnDialogProc_shown = alloca(nColumnCount * sizeof(int));
	bResult = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLUMNS), hMain, MyColumnDialogProc);
	return bResult;
}



static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	HMENU hMenu;
	HMENU hMenuLoad;
	HWND hwndPicker;
	int nMenuItem;

	hMenuLoad = LoadMenu(NULL, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
	hMenu = GetSubMenu(hMenuLoad, 0);

	nMenuItem = (int) TrackPopupMenu(hMenu,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		pt.x,
		pt.y,
		0,
		hMain,
		NULL);

	DestroyMenu(hMenuLoad);

	hwndPicker = GetDlgItem(hMain, IDC_SWLIST);

	switch(nMenuItem) {
	case ID_SORT_ASCENDING:
		SetMessSortReverse(FALSE);
		SetMessSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
		Picker_Sort(hwndPicker);
		break;

	case ID_SORT_DESCENDING:
		SetMessSortReverse(TRUE);
		SetMessSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
		Picker_Sort(hwndPicker);
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (RunColumnDialog(hwndPicker))
			Picker_ResetColumnDisplay(hwndPicker);
		break;
	}
}



// ------------------------------------------------------------------------
// MessCommand
// ------------------------------------------------------------------------

static BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id) {
	case ID_MESS_OPEN_SOFTWARE:
		MessOpenOtherSoftware(IO_COUNT);
		break;

	case ID_MESS_CREATE_SOFTWARE:
		MessCreateDevice(IO_COUNT);
		break;

#ifdef MAME_DEBUG
	case ID_MESS_RUN_TESTS:
		MessTestsBegin();
		break;
#endif /* MAME_DEBUG */
	}
	return FALSE;
}



// ------------------------------------------------------------------------
// Software Tab View 
// ------------------------------------------------------------------------

static LPCTSTR s_tabs[] =
{
	TEXT("picker\0Picker"),
	TEXT("devview\0Device View")
};



static LPCTSTR SoftwareTabView_GetTabShortName(int tab)
{
	return s_tabs[tab];
}



static LPCTSTR SoftwareTabView_GetTabLongName(int tab)
{
	return s_tabs[tab] + _tcslen(s_tabs[tab]) + 1;
}



static void SoftwareTabView_OnSelectionChanged(void)
{
	int nTab;
	HWND hwndSoftwarePicker;
	HWND hwndSoftwareDevView;

	hwndSoftwarePicker = GetDlgItem(hMain, IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(hMain, IDC_SWDEVVIEW);

	nTab = TabView_GetCurrentTab(GetDlgItem(hMain, IDC_SWTAB));

	switch(nTab)
	{
		case 0:
			ShowWindow(hwndSoftwarePicker, SW_SHOW);
			ShowWindow(hwndSoftwareDevView, SW_HIDE);
			break;

		case 1:
			ShowWindow(hwndSoftwarePicker, SW_HIDE);
			ShowWindow(hwndSoftwareDevView, SW_SHOW);
			break;
	}
}



static void SoftwareTabView_OnMoveSize(void)
{
	HWND hwndSoftwareTabView;
	HWND hwndSoftwarePicker;
	HWND hwndSoftwareDevView;
	RECT rMain, rSoftwareTabView, rClient, rTab;

	hwndSoftwareTabView = GetDlgItem(hMain, IDC_SWTAB);
	hwndSoftwarePicker = GetDlgItem(hMain, IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(hMain, IDC_SWDEVVIEW);

	GetWindowRect(hwndSoftwareTabView, &rSoftwareTabView);
	GetClientRect(hMain, &rMain);
	ClientToScreen(hMain, &((POINT *) &rMain)[0]);
	ClientToScreen(hMain, &((POINT *) &rMain)[1]);

	// Calculate rClient from rSoftwareTabView in terms of rMain coordinates
	rClient.left = rSoftwareTabView.left - rMain.left;
	rClient.top = rSoftwareTabView.top - rMain.top;
	rClient.right = rSoftwareTabView.right - rMain.left;
	rClient.bottom = rSoftwareTabView.bottom - rMain.top;

	// If the tabs are visible, then make sure that the tab view's tabs are
	// not being covered up
	if (GetWindowLong(hwndSoftwareTabView, GWL_STYLE) & WS_VISIBLE)
	{
		TabCtrl_GetItemRect(hwndSoftwareTabView, 0, &rTab);
		rClient.top += rTab.bottom - rTab.top + 4;
	}

	/* Now actually move the controls */
	MoveWindow(hwndSoftwarePicker, rClient.left, rClient.top,
		rClient.right - rClient.left, rClient.bottom - rClient.top,
		TRUE);
	MoveWindow(hwndSoftwareDevView, rClient.left, rClient.top,
		rClient.right - rClient.left, rClient.bottom - rClient.top,
		TRUE);
}



static void SetupSoftwareTabView(void)
{
	struct TabViewOptions opts;

	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_softwareTabViewCallbacks;
	opts.nTabCount = sizeof(s_tabs) / sizeof(s_tabs[0]);

	SetupTabView(GetDlgItem(hMain, IDC_SWTAB), &opts);
}



// ------------------------------------------------------------------------
// Mess32 Diagnostics
// ------------------------------------------------------------------------

#ifdef MAME_DEBUG

static int s_nOriginalPick;
static UINT_PTR s_nTestingTimer;
static BOOL s_bInTimerProc;

static void MessTestsColumns(void)
{
	int i, j;
	int oldshown[MESS_COLUMN_MAX];
	int shown[MESS_COLUMN_MAX];

	GetMessColumnShown(oldshown);

	shown[0] = 1;
	for (i = 0; i < (1<<(MESS_COLUMN_MAX-1)); i++)
	{
		for (j = 1; j < MESS_COLUMN_MAX; j++)
			shown[j] = (i & (1<<(j-1))) ? 1 : 0;

		SetMessColumnShown(shown);
		Picker_ResetColumnDisplay(GetDlgItem(hMain, IDC_SWLIST));
	}

	SetMessColumnShown(oldshown);
	Picker_ResetColumnDisplay(GetDlgItem(hMain, IDC_SWLIST));
}




static void CALLBACK MessTestsTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	int nNewGame;

	// if either of the pickers have further idle work to do, or we are
	// already in this timerproc, do not do anything
	if (s_bInTimerProc || Picker_IsIdling(GetDlgItem(hMain, IDC_LIST))
		|| Picker_IsIdling(GetDlgItem(hMain, IDC_SWLIST)))
		return;
	s_bInTimerProc = TRUE;

	nNewGame = GetSelectedPick() + 1;

	if (nNewGame >= driver_get_count())
	{
		/* We are done */
		Picker_SetSelectedPick(hwndList, s_nOriginalPick);

		KillTimer(NULL, s_nTestingTimer);
		s_nTestingTimer = 0;

		MessageBox(hMain, "Tests successfully completed!", MAME32NAME, MB_OK);
	}
	else
	{
//		MessTestsFlex(GetDlgItem(hMain, IDC_SWLIST), drivers[Picker_GetSelectedItem(hwndList)]);
		Picker_SetSelectedPick(hwndList, nNewGame);
	}
	s_bInTimerProc = FALSE;
}



static void MessTestsBegin(void)
{
	int nOriginalPick;

	nOriginalPick = GetSelectedPick();

	// If we are not currently running tests, set up the timer and keep track
	// of the original selected pick item
	if (!s_nTestingTimer)
	{
		s_nTestingTimer = SetTimer(NULL, 0, 50, MessTestsTimerProc);
		if (!s_nTestingTimer)
			return;
		s_nOriginalPick = nOriginalPick;
	}

	MessTestsColumns();

	if (nOriginalPick == 0)
		Picker_SetSelectedPick(hwndList, 1);
	Picker_SetSelectedPick(hwndList, 0);
}



#endif // MAME_DEBUG
