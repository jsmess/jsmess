//============================================================
//
//  messui.c - MESS extensions to src/osd/winui/winui.c
//
//============================================================

// standard windows headers
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

// MAME/MAMEOS/MAMEUI/MESS headers
#include "screenshot.h"
#include "bitmask.h"
#include "winui.h"
#include "resourcems.h"
#include "mui_opts.h"
#include "picker.h"
#include "tabview.h"
#include "mui_util.h"
#include "columnedit.h"
#include "mess.h"
#include "configms.h"
#include "softwarepicker.h"
#include "devview.h"
#include "windows/window.h"
#include "utils.h"
#include "strconv.h"
#include "messui.h"
#include "winutf8.h"
#include "swconfig.h"
#include "device.h"
#include "zippath.h"


//============================================================
//  PARAMETERS
//============================================================

#define LOG_SOFTWARE	0


//============================================================
//  TYPEDEFS
//============================================================

typedef struct _mess_image_type mess_image_type;
struct _mess_image_type
{
	const device_config *dev;
    char *ext;
	const char *dlgname;
};


typedef struct _device_entry device_entry;
struct _device_entry
{
	iodevice_t dev_type;
	const char *icon_name;
	const char *dlgname;
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

char g_szSelectedItem[MAX_PATH];



//============================================================
//  LOCAL VARIABLES
//============================================================

static software_config *s_config;
static BOOL s_bIgnoreSoftwarePickerNotifies;

static int *mess_icon_index;

// TODO - We need to make icons for Cylinders, Punch Cards, and Punch Tape!
static const device_entry s_devices[] =
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
	{ IO_CDROM,		NULL,		"CD-ROM images" },
	{ IO_MAGTAPE,	NULL,		"Magnetic tapes" }
};



static const LPCTSTR mess_column_names[] =
{
	TEXT("Software"),
	TEXT("Goodname"),
	TEXT("Manufacturer"),
	TEXT("Year"),
	TEXT("Playable"),
	TEXT("CRC"),
	TEXT("SHA-1"),
	TEXT("MD5")
};



//============================================================
//  PROTOTYPES
//============================================================

static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn);

static LPCSTR SoftwareTabView_GetTabShortName(int tab);
static LPCSTR SoftwareTabView_GetTabLongName(int tab);
static void SoftwareTabView_OnSelectionChanged(void);
static void SoftwareTabView_OnMoveSize(void);
static void SetupSoftwareTabView(void);

static void MessOpenOtherSoftware(const device_config *dev);
static void MessCreateDevice(const device_config *dev);
static void MessRefreshPicker(void);

static int SoftwarePicker_GetItemImage(HWND hwndPicker, int nItem);
static void SoftwarePicker_LeavingItem(HWND hwndSoftwarePicker, int nItem);
static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem);

static BOOL DevView_GetOpenFileName(HWND hwndDevView, const machine_config *config, const device_config *dev, LPTSTR pszFilename, UINT nFilenameLength);
static BOOL DevView_GetCreateFileName(HWND hwndDevView, const machine_config *config, const device_config *dev, LPTSTR pszFilename, UINT nFilenameLength);
static void DevView_SetSelectedSoftware(HWND hwndDevView, int nDriverIndex, const machine_config *config, const device_config *dev, LPCTSTR pszFilename);
static LPCTSTR DevView_GetSelectedSoftware(HWND hwndDevView, int nDriverIndex, const machine_config *config, const device_config *dev, LPTSTR pszBuffer, UINT nBufferLength);

#ifdef MAME_DEBUG
static void MessTestsBegin(void);
#endif /* MAME_DEBUG */



//============================================================
//  PICKER/TABVIEW CALLBACKS
//============================================================

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



//============================================================
//  Image types
//
//  IO_ZIP is used for ZIP files
//  IO_UNKNOWN is used for unknown types
//  IO_BAD is used for bad files
//============================================================

#define IO_ZIP		(IO_COUNT + 0)
#define IO_BAD		(IO_COUNT + 1)
#define IO_UNKNOWN	(IO_COUNT + 2)

//============================================================
//  IMPLEMENTATION
//============================================================

static const device_entry *lookupdevice(iodevice_t d)
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(s_devices); i++)
	{
		if (s_devices[i].dev_type == d)
			return &s_devices[i];
	}
	return NULL;
}



BOOL CreateMessIcons(void)
{
    int i;
	HWND hwndSoftwareList;
	HIMAGELIST hSmall;
	HIMAGELIST hLarge;

	// create the icon index, if we havn't already
    if (!mess_icon_index)
	{
		mess_icon_index = (int*)pool_malloc_lib(GetMameUIMemoryPool(), driver_list_get_count(drivers) * IO_COUNT * sizeof(*mess_icon_index));
    }

    for (i = 0; i < (driver_list_get_count(drivers) * IO_COUNT); i++)
        mess_icon_index[i] = 0;

	// Associate the image lists with the list view control.
	hwndSoftwareList = GetDlgItem(GetMainWindow(), IDC_SWLIST);
	hSmall = GetSmallImageList();
	hLarge = GetLargeImageList();
	(void)ListView_SetImageList(hwndSoftwareList, hSmall, LVSIL_SMALL);
	(void)ListView_SetImageList(hwndSoftwareList, hLarge, LVSIL_NORMAL);
	return TRUE;
}



static int GetMessIcon(int drvindex, int nSoftwareType)
{
    int the_index;
    int nIconPos = 0;
    HICON hIcon = 0;
    const game_driver *drv;
    char buffer[32];
	const char *iconname;

	assert(drvindex >= 0);
	assert(drvindex < driver_list_get_count(drivers));

    if ((nSoftwareType >= 0) && (nSoftwareType < IO_COUNT))
	{
		iconname = device_brieftypename((iodevice_t)nSoftwareType);
        the_index = (drvindex * IO_COUNT) + nSoftwareType;

        nIconPos = mess_icon_index[the_index];
        if (nIconPos >= 0)
		{
			for (drv = drivers[drvindex]; drv; drv = driver_get_clone(drv))
			{
				_snprintf(buffer, ARRAY_LENGTH(buffer), "%s/%s", drv->name, iconname);
				hIcon = LoadIconFromFile(buffer);
				if (hIcon)
					break;
			}

			if (hIcon)
			{
				nIconPos = ImageList_AddIcon(GetSmallImageList(), hIcon);
				ImageList_AddIcon(GetLargeImageList(), hIcon);
				if (nIconPos != -1)
					mess_icon_index[the_index] = nIconPos;
			}
        }
    }
    return nIconPos;
}


#ifdef UNUSED_FUNCTION
static void MessHashErrorProc(const char *message)
{
	SetStatusBarTextF(0, "Hash file: %s", message);
}
#endif


static BOOL AddSoftwarePickerDirs(HWND hwndPicker, LPCSTR pszDirectories, LPCSTR pszSubDir)
{
	LPCSTR s;
	LPSTR pszNewString;
	char cSeparator = ';';
	int nLength;

	do
	{
		s = pszDirectories;
		while(*s && (*s != cSeparator))
			s++;

		nLength = s - pszDirectories;
		if (nLength > 0)
		{
			pszNewString = (LPSTR) alloca((nLength + 1 + (pszSubDir ? strlen(pszSubDir) + 1 : 0)));
			memcpy(pszNewString, pszDirectories, nLength);
			pszNewString[nLength] = '\0';

			if (pszSubDir)
			{
				pszNewString[nLength++] = '\\';
				strcpy(&pszNewString[nLength], pszSubDir);
			}

			if (!SoftwarePicker_AddDirectory(hwndPicker, pszNewString))
				return FALSE;
		}
		pszDirectories = s + 1;
	}
	while(*s);
	return TRUE;
}

void MySoftwareListClose(void)
{
	// free the machine config, if necessary
	if (s_config != NULL)
	{
		software_config_free(s_config);
		s_config = NULL;
	}
}

void MyFillSoftwareList(int drvindex, BOOL bForce)
{
	BOOL is_same;
	const game_driver *drv;
	HWND hwndSoftwarePicker;
	HWND hwndSoftwareDevView;

	// do we have to do anything?
	if (!bForce)
	{
		if (s_config != NULL)
			is_same = (drvindex == s_config->driver_index);
		else
			is_same = (drvindex < 0);
		if (is_same)
			return;
	}

	// free the machine config, if necessary
	MySoftwareListClose();

	// allocate the machine config, if necessary
	if (drvindex >= 0)
	{
		s_config = software_config_alloc(drvindex, MameUIGlobal(), NULL);
	}

	// locate key widgets
	hwndSoftwarePicker = GetDlgItem(GetMainWindow(), IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(GetMainWindow(), IDC_SWDEVVIEW);

	// set up the device view
	DevView_SetDriver(hwndSoftwareDevView, s_config);

	// set up the software picker
	SoftwarePicker_Clear(hwndSoftwarePicker);
	SoftwarePicker_SetDriver(hwndSoftwarePicker, s_config);

	// add the relevant paths
	drv = drivers[drvindex];
	while(drv != NULL)
	{
		AddSoftwarePickerDirs(hwndSoftwarePicker, GetSoftwareDirs(), drv->name);
		drv = driver_get_compatible(drv);
	}
	AddSoftwarePickerDirs(hwndSoftwarePicker, GetExtraSoftwarePaths(drvindex), NULL);
}



void MessUpdateSoftwareList(void)
{
	HWND hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);
	MyFillSoftwareList(Picker_GetSelectedItem(hwndList), TRUE);
}



BOOL MessApproveImageList(HWND hParent, int drvindex)
{
	machine_config *config;
	const device_config *dev;
	//int nPos;
	char szMessage[256];
	LPCSTR pszSoftware;
	BOOL bResult = FALSE;

//  begin_resource_tracking();

	// allocate the machine config
	config = machine_config_alloc(drivers[drvindex]->machine_config);

	//nPos = 0;
	for (dev = config->devicelist.first(); dev != NULL; dev = dev->next)
	{
		if (is_image_device(dev))
		{
			image_device_info info = image_device_getinfo(config, dev);

			// confirm any mandatory devices are loaded
			if (info.must_be_loaded)
			{
				pszSoftware = GetSelectedSoftware(drvindex, config, dev);
				if (!pszSoftware || !*pszSoftware)
				{
					snprintf(szMessage, ARRAY_LENGTH(szMessage),
						"System '%s' requires that device %s must have an image to load\n",
						drivers[drvindex]->description,
						device_typename(info.type));
					goto done;
				}
			}

			//nPos++;
		}
	}
	bResult = TRUE;

done:
	if (!bResult)
	{
		win_message_box_utf8(hParent, szMessage, MAMEUINAME, MB_OK);
	}

	machine_config_free(config);
	return bResult;
}



// this is a wrapper call to wrap the idiosycracies of SetSelectedSoftware()
static void InternalSetSelectedSoftware(int drvindex, const machine_config *config, const device_config *device, const char *pszSoftware)
{
	if (!pszSoftware)
		pszSoftware = "";

	// only call SetSelectedSoftware() if this value is different
	if (strcmp(GetSelectedSoftware(drvindex, config, device), pszSoftware))
	{
		SetSelectedSoftware(drvindex, config, device, pszSoftware);
	}
}



static int is_null_or_empty(const char *s)
{
	return (s == NULL) || (s[0] == '\0');
}



// Places the specified image in the specified slot; nID = -1 means don't matter
static void MessSpecifyImage(int drvindex, const device_config *device, LPCSTR pszFilename)
{
	const char *s;

	if (LOG_SOFTWARE)
		dprintf("MessSpecifyImage(): device=%p pszFilename='%s'\n", device, pszFilename);

	// if device is NULL, this is a special case; try to find existing image
	if (device == NULL)
	{
		for (device = s_config->mconfig->devicelist.first(); device != NULL;device = device->next)
		{
			if (is_image_device(device))
			{
				s = GetSelectedSoftware(drvindex, s_config->mconfig, device);
				if ((s != NULL) && !mame_stricmp(s, pszFilename))
					break;
			}
		}
	}

	// still not found?  find an empty slot for which the device uses the
	// same file extension
	if (device == NULL)
	{
		const char *file_extension;

		// identify the file extension
		file_extension = strrchr(pszFilename, '.');
		file_extension = file_extension ? file_extension + 1 : NULL;

		if (file_extension != NULL)
		{
			for (device = s_config->mconfig->devicelist.first(); device != NULL;device = device->next)
			{
				if (is_image_device(device))
				{
					s = GetSelectedSoftware(drvindex, s_config->mconfig, device);
					if (is_null_or_empty(s) && image_device_uses_file_extension(device, file_extension))
						break;
				}
			}
		}
	}

	if (device != NULL)
	{
		// place the image
		InternalSetSelectedSoftware(drvindex, s_config->mconfig, device, pszFilename);
	}
	else
	{
		// could not place the image
		if (LOG_SOFTWARE)
			dprintf("MessSpecifyImage(): Failed to place image '%s'\n", pszFilename);
	}
}



static void MessRemoveImage(int drvindex, const char *pszFilename)
{
	const device_config *device;
	const char *s;

	for (device = s_config->mconfig->devicelist.first(); device != NULL;device = device->next)
	{
		if (is_image_device(device))
		{
			s = GetSelectedSoftware(drvindex, s_config->mconfig, device);
			if ((s != NULL) && !strcmp(pszFilename, s))
				MessSpecifyImage(drvindex, device, NULL);
		}
	}
}



void MessReadMountedSoftware(int drvindex)
{
	// First read stuff into device view
	DevView_Refresh(GetDlgItem(GetMainWindow(), IDC_SWDEVVIEW));

	// Now read stuff into picker
	MessRefreshPicker();
}



static void MessRefreshPicker(void)
{
	HWND hwndSoftware;
	int i;
	LVFINDINFO lvfi;
	const device_config *dev;
	LPCSTR pszSoftware;

	hwndSoftware = GetDlgItem(GetMainWindow(), IDC_SWLIST);

	s_bIgnoreSoftwarePickerNotifies = TRUE;

	// Now clear everything out; this may call back into us but it should not
	// be problematic
	ListView_SetItemState(hwndSoftware, -1, 0, LVIS_SELECTED);

	for (dev = s_config->mconfig->devicelist.first(); dev != NULL;dev = dev->next)
	{
		if (is_image_device(dev))
		{
			pszSoftware = GetSelectedSoftware(s_config->driver_index, s_config->mconfig, dev);
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

	s_bIgnoreSoftwarePickerNotifies = FALSE;
}



void InitMessPicker(void)
{
	struct PickerOptions opts;
	HWND hwndSoftware;

	s_config = NULL;
	hwndSoftware = GetDlgItem(GetMainWindow(), IDC_SWLIST);

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
		DevView_SetCallbacks(GetDlgItem(GetMainWindow(), IDC_SWDEVVIEW), &s_devViewCallbacks);
	}
}



// ------------------------------------------------------------------------
// Open others dialog
// ------------------------------------------------------------------------

static BOOL CommonFileImageDialog(LPTSTR the_last_directory, common_file_dialog_proc cfd, LPTSTR filename, const machine_config *config, mess_image_type *imagetypes)
{
    BOOL success;
    OPENFILENAME of;
    char szFilter[2048];
    LPSTR s;
	const char *typname;
    int i;
    TCHAR* t_filter;
    TCHAR* t_buffer;

	s = szFilter;
    *filename = 0;

    // Common image types
    strcpy(s, "Common image types");
    s += strlen(s);
    *(s++) = '|';
    for (i = 0; imagetypes[i].ext; i++)
	{
        *(s++) = '*';
        *(s++) = '.';
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = ';';
    }
    *(s++) = '|';

    // All files
    strcpy(s, "All files (*.*)");
    s += strlen(s);
    *(s++) = '|';
    strcpy(s, "*.*");
    s += strlen(s);
    *(s++) = '|';

    // The others
    for (i = 0; imagetypes[i].ext; i++)
	{
		if (!imagetypes[i].dev)
			typname = "Compressed images";
		else
			typname = imagetypes[i].dlgname;

        strcpy(s, typname);
        s += strlen(s);
        strcpy(s, " (*.");
        s += strlen(s);
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = ')';
        *(s++) = '|';
        *(s++) = '*';
        *(s++) = '.';
        strcpy(s, imagetypes[i].ext);
        s += strlen(s);
        *(s++) = '|';
    }
    *(s++) = '|';

    t_buffer = tstring_from_utf8(szFilter);
    if( !t_buffer )
	    return FALSE;

	// convert a pipe-char delimited string into a NUL delimited string
	t_filter = (LPTSTR) alloca((_tcslen(t_buffer) + 2) * sizeof(*t_filter));
	for (i = 0; t_buffer[i] != '\0'; i++)
		t_filter[i] = (t_buffer[i] != '|') ? t_buffer[i] : '\0';
	t_filter[i++] = '\0';
	t_filter[i++] = '\0';
	global_free(t_buffer);

    of.lStructSize = sizeof(of);
    of.hwndOwner = GetMainWindow();
    of.hInstance = NULL;
    of.lpstrFilter = t_filter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 1;
    of.lpstrFile = filename;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = NULL;
    of.nMaxFileTitle = 0;
    of.lpstrInitialDir = the_last_directory;
    of.lpstrTitle = NULL;
    of.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    of.nFileOffset = 0;
    of.nFileExtension = 0;
    of.lpstrDefExt = TEXT("rom");
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
static void SetupImageTypes(const machine_config *config, mess_image_type *types, int count, BOOL bZip, const device_config *dev)
{
    int num_extensions = 0;

	memset(types, 0, sizeof(*types) * count);
    count--;

    if (bZip)
	{
		/* add the ZIP extension */
		types[num_extensions].ext = mame_strdup("zip");
		types[num_extensions].dev = NULL;
		types[num_extensions].dlgname = NULL;
		num_extensions++;
    }

	if (dev == NULL)
	{
		/* special case; all non-printer devices */
		for (dev = config->devicelist.first(); dev != NULL;dev = dev->next)
		{
			if (is_image_device(dev))
			{
				image_device_info info = image_device_getinfo(config, dev);

				if (info.type != IO_PRINTER)
					SetupImageTypes(config, &types[num_extensions], count - num_extensions, FALSE, dev);
			}
		}

	}
	else
	{
		image_device_info info = image_device_getinfo(config, dev);
		const char *ext = info.file_extensions;

		if (ext != NULL)
		{
			while(*ext != '\0')
			{
				if (num_extensions < count)
				{
					types[num_extensions].dev = dev;
					types[num_extensions].ext = mame_strdup(ext);
					types[num_extensions].dlgname = lookupdevice(info.type)->dlgname;
					num_extensions++;
				}
				ext += strlen(ext) + 1;
			}
		}
	}
}


static void CleanupImageTypes(mess_image_type *types, int count)
{
	int i;
	for (i = 0; i < count; ++i)
	{
		if (types[i].ext)
			osd_free(types[i].ext);
	}
}


static void MessSetupDevice(common_file_dialog_proc cfd, const device_config *dev)
{
	TCHAR filename[MAX_PATH];
	mess_image_type imagetypes[64];
	int drvindex;
	HWND hwndList;
	char* utf8_filename;
	machine_config *config;
	BOOL cfd_res;

//  begin_resource_tracking();

	hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);
	drvindex = Picker_GetSelectedItem(hwndList);

	// allocate the machine config
	config = machine_config_alloc(drivers[drvindex]->machine_config);

	SetupImageTypes(config, imagetypes, ARRAY_LENGTH(imagetypes), TRUE, dev);
	cfd_res = CommonFileImageDialog(last_directory, cfd, filename, config, imagetypes);
	CleanupImageTypes(imagetypes, ARRAY_LENGTH(imagetypes));

	if (cfd_res)
	{
		utf8_filename = utf8_from_tstring(filename);
		if( !utf8_filename )
			return;
		// TODO - this should go against InternalSetSelectedSoftware()
		SoftwarePicker_AddFile(GetDlgItem(GetMainWindow(), IDC_SWLIST), utf8_filename);
		global_free(utf8_filename);
	}
	
	machine_config_free(config);
}



static void MessOpenOtherSoftware(const device_config *dev)
{
	MessSetupDevice(GetOpenFileName, dev);
}



static void MessCreateDevice(const device_config *dev)
{
	MessSetupDevice(GetSaveFileName, dev);
}


/* This is used to Mount an image in the device view of MESSUI. The directory in the dialog box is not
    set, and is thus random.
    2009-10-18 Robbbert:
    I've attempted to set the directory properly. Since the emulation is not running at this time,
    we cannot do the same as the NEWUI does, that is, "initial_dir = image_working_directory(dev);"
    because a crash occurs (the image system isn't set up yet).

    Order of priority:
    1. Directory where existing image is already loaded from
    2. First directory specified in game-specific software tab
    3. First directory specified in the system-wide software directories
    4. mess-folder\software
    5. mess-folder */

static BOOL DevView_GetOpenFileName(HWND hwndDevView, const machine_config *config, const device_config *dev, LPTSTR pszFilename, UINT nFilenameLength)
{
	BOOL bResult;
	TCHAR *t_s;
	int i;
	mess_image_type imagetypes[64];
	HWND hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);
	int drvindex = Picker_GetSelectedItem(hwndList);
	astring as;

	/* Get the path to the currently mounted image */
	zippath_parent(&as, GetSelectedSoftware(drvindex, config, dev));
	t_s = tstring_from_utf8(astring_c(&as));

	/* See if an image was loaded, and that the path still exists */
	if ((!osd_opendir(astring_c(&as))) || (astring_chr(&as, 0, ':') == -1))
	{
		/* Get the path from the software tab */
		astring_cpyc(&as, GetExtraSoftwarePaths(drvindex));

		/* We only want the first path; throw out the rest */
		i = astring_chr(&as, 0, ';');
		if (i > 0) astring_substr(&as, 0, i);
		global_free(t_s);
		t_s = tstring_from_utf8(astring_c(&as));

		/* Make sure a folder was specified in the tab, and that it exists */
		if ((!osd_opendir(astring_c(&as))) || (astring_chr(&as, 0, ':') == -1))
		{
			/* Get the path from the system-wide software setting in the drop-down directory setup */
			astring_cpyc(&as, GetSoftwareDirs());

			/* We only want the first path; throw out the rest */
			i = astring_chr(&as, 0, ';');
			if (i > 0) astring_substr(&as, 0, i);
			global_free(t_s);
			t_s = tstring_from_utf8(astring_c(&as));

			/* Make sure a folder was specified in the tab, and that it exists */
			if ((!osd_opendir(astring_c(&as))) || (astring_chr(&as, 0, ':') == -1))
			{
				char *dst = NULL;

				osd_get_full_path(&dst,".");
				/* Default to emu directory */
				global_free(t_s);
				t_s = tstring_from_utf8(dst);

				/* If software folder exists, use it instead */
				zippath_combine(&as, dst, "software");
				if (osd_opendir(astring_c(&as))) 
				{
					global_free(t_s);
					t_s = tstring_from_utf8(astring_c(&as));
				}
				global_free(dst);
			}
		}
	}

	SetupImageTypes(config, imagetypes, ARRAY_LENGTH(imagetypes), TRUE, dev);
	bResult = CommonFileImageDialog(t_s, GetOpenFileName, pszFilename, config, imagetypes);
	CleanupImageTypes(imagetypes, ARRAY_LENGTH(imagetypes));
	
	global_free(t_s);

	return bResult;
}


/* This is used to Create an image in the device view of MESSUI. The directory in the dialog box is not
    set, and is thus random.

    Order of priority:
    1. First directory specified in game-specific software tab
    2. First directory specified in the system-wide software directories
    3. mess-folder\software
    4. mess-folder */

static BOOL DevView_GetCreateFileName(HWND hwndDevView, const machine_config *config, const device_config *dev, LPTSTR pszFilename, UINT nFilenameLength)
{
	BOOL bResult;
	TCHAR *t_s;
	int i;
	mess_image_type imagetypes[64];
	HWND hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);
	int drvindex = Picker_GetSelectedItem(hwndList);
	astring as;

	/* Get the path from the software tab */
	astring_cpyc(&as, GetExtraSoftwarePaths(drvindex));

	/* We only want the first path; throw out the rest */
	i = astring_chr(&as, 0, ';');
	if (i > 0) astring_substr(&as, 0, i);
	t_s = tstring_from_utf8(astring_c(&as));

	/* Make sure a folder was specified in the tab, and that it exists */
	if ((!osd_opendir(astring_c(&as))) || (astring_chr(&as, 0, ':') == -1))
	{
		/* Get the path from the system-wide software setting in the drop-down directory setup */
		astring_cpyc(&as, GetSoftwareDirs());

		/* We only want the first path; throw out the rest */
		i = astring_chr(&as, 0, ';');
		if (i > 0) astring_substr(&as, 0, i);
		global_free(t_s);
		t_s = tstring_from_utf8(astring_c(&as));

		/* Make sure a folder was specified in the tab, and that it exists */
		if ((!osd_opendir(astring_c(&as))) || (astring_chr(&as, 0, ':') == -1))
		{
			char *dst = NULL;

			osd_get_full_path(&dst,".");
			/* Default to emu directory */
			global_free(t_s);
			t_s = tstring_from_utf8(dst);

			/* If software folder exists, use it instead */
			zippath_combine(&as, dst, "software");
			if (osd_opendir(astring_c(&as))) 
			{
				global_free(t_s);
				t_s = tstring_from_utf8(astring_c(&as));
			}
			global_free(dst);
		}
	}

	SetupImageTypes(config, imagetypes, ARRAY_LENGTH(imagetypes), TRUE, dev);
	bResult = CommonFileImageDialog(t_s, GetSaveFileName, pszFilename, config, imagetypes);
	CleanupImageTypes(imagetypes, ARRAY_LENGTH(imagetypes));
	
	global_free(t_s);

	return bResult;
}



static void DevView_SetSelectedSoftware(HWND hwndDevView, int drvindex,
	const machine_config *config, const device_config *dev, LPCTSTR pszFilename)
{
	char* utf8_filename = utf8_from_tstring(pszFilename);
	if( !utf8_filename )
		return;
	MessSpecifyImage(drvindex, dev, utf8_filename);
	MessRefreshPicker();
	global_free(utf8_filename);
}



static LPCTSTR DevView_GetSelectedSoftware(HWND hwndDevView, int nDriverIndex,
	const machine_config *config, const device_config *dev, LPTSTR pszBuffer, UINT nBufferLength)
{
	LPCTSTR t_buffer = NULL;
	TCHAR* t_s;
	LPCSTR s = GetSelectedSoftware(nDriverIndex, config, dev);

	t_s = tstring_from_utf8(s);
	if( !t_s )
		return t_buffer;

	_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_s);
	global_free(t_s);
	t_buffer = pszBuffer;

	return t_buffer;
}



// ------------------------------------------------------------------------
// Software List Class
// ------------------------------------------------------------------------

static int SoftwarePicker_GetItemImage(HWND hwndPicker, int nItem)
{
	iodevice_t nType;
	int nIcon;
	int drvindex;
	const char *icon_name;
	HWND hwndGamePicker;
	HWND hwndSoftwarePicker;

	hwndGamePicker = GetDlgItem(GetMainWindow(), IDC_LIST);
	hwndSoftwarePicker = GetDlgItem(GetMainWindow(), IDC_SWLIST);
	drvindex = Picker_GetSelectedItem(hwndGamePicker);
	nType = SoftwarePicker_GetImageType(hwndSoftwarePicker, nItem);
	nIcon = GetMessIcon(drvindex, nType);
	if (!nIcon)
	{
		if (nType == 0)
			nType = (iodevice_t)IO_BAD;
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
				nIcon = FindIconIndexByName(icon_name);
				if (nIcon < 0)
					nIcon = FindIconIndexByName("unknown");
				break;
		}
    }
    return nIcon;
}



static void SoftwarePicker_LeavingItem(HWND hwndSoftwarePicker, int nItem)
{
	int drvindex;
	const char *pszFullName;
	HWND hwndList;

	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);
		drvindex = Picker_GetSelectedItem(hwndList);
		pszFullName = SoftwarePicker_LookupFilename(hwndSoftwarePicker, nItem);

		MessRemoveImage(drvindex, pszFullName);
	}
}



static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem)
{
	LPCSTR pszFullName;
	LPCSTR pszName;
	const char* tmp;
	LPSTR s;
	int drvindex;
	HWND hwndList;

	hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);

	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		drvindex = Picker_GetSelectedItem(hwndList);

		// Get the fullname and partialname for this file
		pszFullName = SoftwarePicker_LookupFilename(hwndSoftwarePicker, nItem);
		tmp = strchr(pszFullName, '\\');
		pszName = tmp ? tmp + 1 : pszFullName;

		// Do the dirty work
		MessSpecifyImage(drvindex, NULL, pszFullName);

		// Set up s_szSelecteItem, for the benefit of UpdateScreenShot()
		strncpyz(g_szSelectedItem, pszName, ARRAY_LENGTH(g_szSelectedItem));
		s = strrchr(g_szSelectedItem, '.');
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
	INT_PTR result = 0;
	int nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);
	const LPCTSTR *ppszColumnNames = Picker_GetColumnNames(MyColumnDialogProc_hwndPicker);

	result = InternalColumnDialogProc(hDlg, Msg, wParam, lParam, nColumnCount,
		MyColumnDialogProc_shown, MyColumnDialogProc_order, ppszColumnNames,
		MyGetRealColumnOrder, MyGetColumnInfo, MySetColumnInfo);

	return result;
}



static BOOL RunColumnDialog(HWND hwndPicker)
{
	BOOL bResult;
	int nColumnCount;

	MyColumnDialogProc_hwndPicker = hwndPicker;
	nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);

	MyColumnDialogProc_order = (int*)alloca(nColumnCount * sizeof(int));
	MyColumnDialogProc_shown = (int*)alloca(nColumnCount * sizeof(int));
	bResult = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLUMNS), GetMainWindow(), MyColumnDialogProc);
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
		GetMainWindow(),
		NULL);

	DestroyMenu(hMenuLoad);

	hwndPicker = GetDlgItem(GetMainWindow(), IDC_SWLIST);

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

BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		case ID_MESS_OPEN_SOFTWARE:
			MessOpenOtherSoftware(NULL);
			break;

		case ID_MESS_CREATE_SOFTWARE:
			MessCreateDevice(NULL);
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

static LPCSTR s_tabs[] =
{
	"picker\0Picker",
	"devview\0Device View"
};



static LPCSTR SoftwareTabView_GetTabShortName(int tab)
{
	return s_tabs[tab];
}



static LPCSTR SoftwareTabView_GetTabLongName(int tab)
{
	return s_tabs[tab] + strlen(s_tabs[tab]) + 1;
}



static void SoftwareTabView_OnSelectionChanged(void)
{
	int nTab;
	HWND hwndSoftwarePicker;
	HWND hwndSoftwareDevView;

	hwndSoftwarePicker = GetDlgItem(GetMainWindow(), IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(GetMainWindow(), IDC_SWDEVVIEW);

	nTab = TabView_GetCurrentTab(GetDlgItem(GetMainWindow(), IDC_SWTAB));

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
	BOOL res;

	hwndSoftwareTabView = GetDlgItem(GetMainWindow(), IDC_SWTAB);
	hwndSoftwarePicker = GetDlgItem(GetMainWindow(), IDC_SWLIST);
	hwndSoftwareDevView = GetDlgItem(GetMainWindow(), IDC_SWDEVVIEW);

	GetWindowRect(hwndSoftwareTabView, &rSoftwareTabView);
	GetClientRect(GetMainWindow(), &rMain);
	ClientToScreen(GetMainWindow(), &((POINT *) &rMain)[0]);
	ClientToScreen(GetMainWindow(), &((POINT *) &rMain)[1]);

	// Calculate rClient from rSoftwareTabView in terms of rMain coordinates
	rClient.left = rSoftwareTabView.left - rMain.left;
	rClient.top = rSoftwareTabView.top - rMain.top;
	rClient.right = rSoftwareTabView.right - rMain.left;
	rClient.bottom = rSoftwareTabView.bottom - rMain.top;

	// If the tabs are visible, then make sure that the tab view's tabs are
	// not being covered up
	if (GetWindowLong(hwndSoftwareTabView, GWL_STYLE) & WS_VISIBLE)
	{
		res = TabCtrl_GetItemRect(hwndSoftwareTabView, 0, &rTab);
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
	opts.nTabCount = ARRAY_LENGTH(s_tabs);

	SetupTabView(GetDlgItem(GetMainWindow(), IDC_SWTAB), &opts);
}


// ------------------------------------------------------------------------
// MessUI Diagnostics
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
		Picker_ResetColumnDisplay(GetDlgItem(GetMainWindow(), IDC_SWLIST));
	}

	SetMessColumnShown(oldshown);
	Picker_ResetColumnDisplay(GetDlgItem(GetMainWindow(), IDC_SWLIST));
}




static void CALLBACK MessTestsTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	int nNewGame;
	HWND hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);

	// if either of the pickers have further idle work to do, or we are
	// already in this timerproc, do not do anything
	if (s_bInTimerProc || Picker_IsIdling(hwndList)
		|| Picker_IsIdling(GetDlgItem(GetMainWindow(), IDC_SWLIST)))
		return;
	s_bInTimerProc = TRUE;

	nNewGame = GetSelectedPick() + 1;

	if (nNewGame >= driver_list_get_count(drivers))
	{
		/* We are done */
		Picker_SetSelectedPick(hwndList, s_nOriginalPick);

		KillTimer(NULL, s_nTestingTimer);
		s_nTestingTimer = 0;

		win_message_box_utf8(GetMainWindow(), "Tests successfully completed!", MAMEUINAME, MB_OK);
	}
	else
	{
//      MessTestsFlex(GetDlgItem(GetMainWindow(), IDC_SWLIST), drivers[Picker_GetSelectedItem(hwndList)]);
		Picker_SetSelectedPick(hwndList, nNewGame);
	}
	s_bInTimerProc = FALSE;
}



static void MessTestsBegin(void)
{
	int nOriginalPick;
	HWND hwndList = GetDlgItem(GetMainWindow(), IDC_LIST);

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
