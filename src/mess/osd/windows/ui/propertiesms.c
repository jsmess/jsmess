#define WIN32_LEAN_AND_MEAN

#ifndef _MSC_VER
#define NONAMELESSUNION 1
#endif

#include <windows.h>
#include <string.h>
#include <assert.h>
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <sys/stat.h>
#include <stdio.h>
#include <tchar.h>

#include "driver.h"
#include "device.h"
#include "ui/screenshot.h"
#include "ui/datamap.h"
#include "ui/bitmask.h"
#include "ui/mame32.h"
#include "ui/directories.h"
#include "ui/m32util.h"
#include "ui/m32opts.h"
#include "resourcems.h"
#include "mess.h"
#include "utils.h"
#include "propertiesms.h"
#include "optionsms.h"
#include "ms32util.h"
#include "messopts.h"
#include "strconv.h"

static BOOL SoftwareDirectories_OnInsertBrowse(HWND hDlg, BOOL bBrowse, LPCSTR lpItem);
static BOOL SoftwareDirectories_OnDelete(HWND hDlg);
static BOOL SoftwareDirectories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static BOOL SoftwareDirectories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);

extern BOOL BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult);
BOOL g_bModifiedSoftwarePaths = FALSE;

static void MarkChanged(HWND hDlg)
{
	HWND hCtrl;

	/* fake a CBN_SELCHANGE event from IDC_SIZES to force it to be changed */
	hCtrl = GetDlgItem(hDlg, IDC_SIZES);
	PostMessage(hDlg, WM_COMMAND, (CBN_SELCHANGE << 16) | IDC_SIZES, (LPARAM) hCtrl);
}

static void AppendList(HWND hList, LPCSTR lpItem, int nItem)
{
    LV_ITEM Item;
	memset(&Item, 0, sizeof(LV_ITEM));
	Item.mask = LVIF_TEXT;
	Item.pszText = (LPSTR) lpItem;
	Item.iItem = nItem;
	ListView_InsertItem(hList, &Item);
}

static BOOL SoftwareDirectories_OnInsertBrowse(HWND hDlg, BOOL bBrowse, LPCSTR lpItem)
{
    int nItem;
    char inbuf[MAX_PATH];
    char outbuf[MAX_PATH];
    HWND hList;
	LPSTR lpIn;

	g_bModifiedSoftwarePaths = TRUE;

    hList = GetDlgItem(hDlg, IDC_DIR_LIST);
    nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    if (nItem == -1)
        return FALSE;

    /* Last item is placeholder for append */
    if (nItem == ListView_GetItemCount(hList) - 1)
    {
        bBrowse = FALSE;
    }

	if (!lpItem) {
		if (bBrowse) {
			ListView_GetItemText(hList, nItem, 0, inbuf, sizeof(inbuf) / sizeof(inbuf[0]));
			lpIn = inbuf;
		}
		else {
			lpIn = NULL;
		}

		if (!BrowseForDirectory(hDlg, lpIn, outbuf))
	        return FALSE;
		lpItem = outbuf;
	}

	AppendList(hList, lpItem, nItem);
	if (bBrowse)
		ListView_DeleteItem(hList, nItem+1);
	MarkChanged(hDlg);
	return TRUE;
}

static BOOL SoftwareDirectories_OnDelete(HWND hDlg)
{
    int     nCount;
    int     nSelect;
    int     nItem;
    HWND    hList = GetDlgItem(hDlg, IDC_DIR_LIST);

	g_bModifiedSoftwarePaths = TRUE;

    nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED | LVNI_ALL);

    if (nItem == -1)
        return FALSE;

    /* Don't delete "Append" placeholder. */
    if (nItem == ListView_GetItemCount(hList) - 1)
        return FALSE;

	ListView_DeleteItem(hList, nItem);

    nCount = ListView_GetItemCount(hList);
    if (nCount <= 1)
        return FALSE;

    /* If the last item was removed, select the item above. */
    if (nItem == nCount - 1)
        nSelect = nCount - 2;
    else
        nSelect = nItem;

    ListView_SetItemState(hList, nSelect, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	MarkChanged(hDlg);
	return TRUE;
}

static BOOL SoftwareDirectories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	BOOL          bResult = FALSE;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEM*       pItem = &pDispInfo->item;
	HWND    hList = GetDlgItem(hDlg, IDC_DIR_LIST);

	/* Last item is placeholder for append */
	if (pItem->iItem == ListView_GetItemCount(hList) - 1)
	{
		HWND hEdit = (HWND) (int) SendMessage(hList, LVM_GETEDITCONTROL, 0, 0);
		Edit_SetText(hEdit, "");
	}

	return bResult;
}

static BOOL SoftwareDirectories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
    BOOL          bResult = FALSE;
    NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
    LVITEM*       pItem = &pDispInfo->item;

    if (pItem->pszText != NULL)
    {
        struct stat file_stat;

        /* Don't allow empty entries. */
        if (!strcmp(pItem->pszText, ""))
        {
            return FALSE;
        }

        /* Check validity of edited directory. */
        if (stat(pItem->pszText, &file_stat) == 0
        &&  (file_stat.st_mode & S_IFDIR))
        {
            bResult = TRUE;
        }
        else
        {
            if (MessageBox(NULL, "Directory does not exist, continue anyway?", MAME32NAME, MB_OKCANCEL) == IDOK)
                bResult = TRUE;
        }
    }

    if (bResult == TRUE)
    {
		SoftwareDirectories_OnInsertBrowse(hDlg, TRUE, pItem->pszText);
    }

    return bResult;
}

BOOL PropSheetFilter_Config(const machine_config *drv, const game_driver *gamedrv)
{
	return (ram_option_count(gamedrv) > 0) || DriverHasDevice(gamedrv, IO_PRINTER);
}

INT_PTR CALLBACK GameMessOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR rc = 0;
	BOOL bHandled = FALSE;

	switch (Msg) {
	case WM_NOTIFY:
		switch (((NMHDR *) lParam)->code) {
		case LVN_ENDLABELEDIT:
			rc = SoftwareDirectories_OnEndLabelEdit(hDlg, (NMHDR *) lParam);
			bHandled = TRUE;
			break;

		case LVN_BEGINLABELEDIT:
			rc = SoftwareDirectories_OnBeginLabelEdit(hDlg, (NMHDR *) lParam);
			bHandled = TRUE;
			break;
		}
	}

	if (!bHandled)
		rc = GameOptionsProc(hDlg, Msg, wParam, lParam);
	return rc;
}

/* ------------------------------------------------------------------------ */

struct component_param_block
{
	core_options *o;
	int nGame;
	WORD wID;
	WORD wNotifyCode;
	BOOL *changed;
};

enum component_msg
{
	CMSG_OPTIONSTOPROP,
	CMSG_PROPTOOPTIONS,
	CMSG_COMMAND
};

static BOOL SoftwareDirectories_ComponentProc(enum component_msg msg, HWND hWnd, const game_driver *gamedrv, struct component_param_block *params)
{
	HWND hList;
	RECT        rectClient;
	LVCOLUMN    LVCol;
	int nItem;
	int nLen;
	char buf[2048];
	LPCSTR s;
	LPCSTR lpList;
	LPSTR lpBuf;
    LV_ITEM Item;
	int iCount, i;
	int iBufLen;

	hList = GetDlgItem(hWnd, IDC_DIR_LIST);
	if (!hList)
		return FALSE;

	switch(msg) {
	case CMSG_OPTIONSTOPROP:
		ListView_DeleteAllItems(hList);

		GetClientRect(hList, &rectClient);
		memset(&LVCol, 0, sizeof(LVCOLUMN));
		LVCol.mask    = LVCF_WIDTH;
		LVCol.cx      = rectClient.right - rectClient.left - GetSystemMetrics(SM_CXHSCROLL);
		ListView_InsertColumn(hList, 0, &LVCol);

		lpList = (params->nGame >= 0) ? GetExtraSoftwarePaths(params->nGame) : NULL;

		nItem = 0;
		while(lpList && *lpList)
		{
			s = strchr(lpList, ';');
			nLen = (s) ? (s - lpList) : (strlen(lpList));
			if (nLen >= sizeof(buf) / sizeof(buf[0]))
				nLen = (sizeof(buf) / sizeof(buf[0])) - 1;
			strncpy(buf, lpList, nLen);
			buf[nLen] = '\0';
			lpList += nLen;
			if (*lpList == ';')
				lpList++;

			AppendList(hList, buf, nItem++);
		}
		AppendList(hList, DIRLIST_NEWENTRYTEXT, nItem);

		ListView_SetItemState(hList, 0, LVIS_SELECTED, LVIS_SELECTED);
		break;

	case CMSG_PROPTOOPTIONS:
		lpBuf = buf;
		iBufLen = sizeof(buf) / sizeof(buf[0]);
		memset(lpBuf, '\0', iBufLen);

		iCount = ListView_GetItemCount(hList);
		if (iCount)
			iCount--;

		memset(&Item, '\0', sizeof(Item));
		Item.mask = LVIF_TEXT;

		*lpBuf = '\0';

		for (i = 0; i < iCount; i++)
		{
			if (i > 0)
				strncatz(lpBuf, ";", iBufLen);

			Item.iItem = i;
			Item.pszText = lpBuf + strlen(lpBuf);
			Item.cchTextMax = iBufLen - strlen(lpBuf);
			ListView_GetItem(hList, &Item);
		}

		if (params->nGame >= 0)
			SetExtraSoftwarePaths(params->nGame, buf);
		break;

	case CMSG_COMMAND:
		switch(params->wID) {
		case IDC_DIR_BROWSE:
			if (params->wNotifyCode == BN_CLICKED)
				*(params->changed) = SoftwareDirectories_OnInsertBrowse(hWnd, TRUE, NULL);
			break;

		case IDC_DIR_INSERT:
			if (params->wNotifyCode == BN_CLICKED)
				*(params->changed) = SoftwareDirectories_OnInsertBrowse(hWnd, FALSE, NULL);
			break;

		case IDC_DIR_DELETE:
			if (params->wNotifyCode == BN_CLICKED)
				*(params->changed) = SoftwareDirectories_OnDelete(hWnd);
			break;

		default:
			return FALSE;
		}
		break;

	default:
		assert(0);
		break;
	}
	return TRUE;
}

/* ------------------------------------------------------------------------ */

typedef BOOL (*component_proc)(enum component_msg msg, HWND hWnd, const game_driver *gamedrv, struct component_param_block *params);

static const component_proc s_ComponentProcs[] =
{
	SoftwareDirectories_ComponentProc
};

static BOOL InvokeComponentProcs(int nGame, enum component_msg msg, HWND hWnd, struct component_param_block *params)
{
	int i;
	const game_driver *gamedrv;
	BOOL handled = FALSE;

	/* figure out which GameDriver we're using.  NULL indicated default options */
	gamedrv = (nGame >= 0) ? drivers[nGame] : NULL;

	for (i = 0; i < sizeof(s_ComponentProcs) / sizeof(s_ComponentProcs[0]); i++)
	{
		if (s_ComponentProcs[i](msg, hWnd, gamedrv, params))
			handled = TRUE;
	}
	return handled;
}

BOOL MessPropertiesCommand(int nGame, HWND hWnd, WORD wNotifyCode, WORD wID, BOOL *changed)
{
	BOOL handled = TRUE;

	switch(wID)
	{
		case IDC_DIR_BROWSE:
			if (wNotifyCode == BN_CLICKED)
				*changed = SoftwareDirectories_OnInsertBrowse(hWnd, TRUE, NULL);
			break;

		case IDC_DIR_INSERT:
			if (wNotifyCode == BN_CLICKED)
				*changed = SoftwareDirectories_OnInsertBrowse(hWnd, FALSE, NULL);
			break;

		case IDC_DIR_DELETE:
			if (wNotifyCode == BN_CLICKED)
				*changed = SoftwareDirectories_OnDelete(hWnd);
			break;

		default:
			handled = FALSE;
			break;
	}
	return handled;
}

//============================================================
//  DATAMAP HANDLERS
//============================================================

static void DirListReadControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	int directory_count;
    LV_ITEM lvi;
	TCHAR buffer[2048];
	char *dir_list;
	int i, pos, driver_index;

	// determine the directory count; note that one item is the "<    >" entry
	directory_count = ListView_GetItemCount(control);
	if (directory_count > 0)
		directory_count--;

	buffer[0] = '\0';
	pos = 0;

	for (i = 0; i < directory_count; i++)
	{
		// append a semicolon, if we're past the first entry
		if (i > 0)
			pos += _sntprintf(&buffer[pos], ARRAY_LENGTH(buffer) - pos, ";");

		// retrieve the next entry
		memset(&lvi, '\0', sizeof(lvi));
		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = &buffer[pos];
		lvi.cchTextMax = ARRAY_LENGTH(buffer) - pos;
		ListView_GetItem(control, &lvi);

		// advance the position
		pos += _tcslen(&buffer[pos]);
	}

	dir_list = utf8_from_tstring(buffer);
	if (dir_list != NULL)
	{
		driver_index = PropertiesCurrentGame(dialog);
		SetExtraSoftwarePaths(driver_index, dir_list);
		free(dir_list);
	}
}



static void DirListPopulateControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	int driver_index;
	int directory_count, pos;
	const char *dir_list;
	TCHAR *t_dir_list;
	TCHAR *s;
	LV_COLUMN lvc;
	RECT r;

	// access the directory list, and convert to TCHARs
	driver_index = PropertiesCurrentGame(dialog);
	dir_list = GetExtraSoftwarePaths(driver_index);
	t_dir_list = tstring_from_utf8(dir_list);
	if (!t_dir_list)
		return;

	// count the number of directories
	directory_count = 0;
	pos = 0;
	while(t_dir_list[pos] != '\0')
	{
		directory_count++;

		s = _tcschr(&t_dir_list[pos], ';');
		if (s != NULL)
		{
			*s = '\0';
			pos = s - &t_dir_list[pos] + 1;
		}
		else
		{
			pos += _tcslen(&t_dir_list[pos]);
		}
	}

	// delete all items in the list control
	ListView_DeleteAllItems(control);

	// add the column
	GetClientRect(control, &r);
	memset(&lvc, 0, sizeof(LVCOLUMN));
	lvc.mask = LVCF_WIDTH;
	lvc.cx = r.right - r.left - GetSystemMetrics(SM_CXHSCROLL);


}



static void RamPopulateControl(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name)
{
	int count, i, default_index, driver_index;
	const game_driver *gamedrv;
	UINT32 ram, default_ram;
	char buffer[64];
	const char *this_ram_string;

	driver_index = PropertiesCurrentGame(dialog);
	gamedrv = drivers[driver_index];

	ComboBox_ResetContent(control);

	count = ram_option_count(gamedrv);
	EnableWindow(control, count > 0);

	if (count > 0)
	{
		default_ram = ram_default(gamedrv);
		default_index = 0;

		for (i = 0; i < count; i++)
		{
			ram = ram_option(gamedrv, i);
			this_ram_string = mame_strdup(ram_string(buffer, ram));

			ComboBox_InsertString(control, i, this_ram_string);
			ComboBox_SetItemData(control, i, this_ram_string);

			if (ram == default_ram)
				default_index = i;
		}
		ComboBox_SetCurSel(control, default_index);
	}
}



//============================================================
//  DATAMAP SETUP
//============================================================

void MessBuildDataMap(datamap *properties_datamap)
{
	// MESS specific stuff
	datamap_add(properties_datamap, IDC_DIR_LIST,				DM_STRING,	NULL);
	datamap_add(properties_datamap, IDC_RAM_COMBOBOX,			DM_INT,		OPTION_RAMSIZE);
	datamap_add(properties_datamap, IDC_SKIP_WARNINGS,			DM_BOOL,	OPTION_SKIP_WARNINGS);
	datamap_add(properties_datamap, IDC_USE_NEW_UI,				DM_BOOL,	"newui");

	// set up callbacks
	datamap_set_callback(properties_datamap, IDC_DIR_LIST,		DCT_READ_CONTROL,		DirListReadControl);
	datamap_set_callback(properties_datamap, IDC_DIR_LIST,		DCT_POPULATE_CONTROL,	DirListPopulateControl);
	datamap_set_callback(properties_datamap, IDC_RAM_COMBOBOX,	DCT_POPULATE_CONTROL,	RamPopulateControl);
}
