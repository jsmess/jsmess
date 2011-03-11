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

#include "emu.h"
#include "emuopts.h"
#include "mui_opts.h"
#include "image.h"
#include "screenshot.h"
#include "datamap.h"
#include "bitmask.h"
#include "winui.h"
#include "directories.h"
#include "mui_util.h"
#include "resourcems.h"
#include "propertiesms.h"
#include "optionsms.h"
#include "msuiutil.h"
#include "strconv.h"
#include "winutf8.h"
#include "machine/ram.h"

static BOOL SoftwareDirectories_OnInsertBrowse(HWND hDlg, BOOL bBrowse, LPCTSTR lpItem);
static BOOL SoftwareDirectories_OnDelete(HWND hDlg);
static BOOL SoftwareDirectories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static BOOL SoftwareDirectories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);

extern BOOL BrowseForDirectory(HWND hwnd, LPCTSTR pStartDir, TCHAR* pResult);
BOOL g_bModifiedSoftwarePaths = FALSE;



static void MarkChanged(HWND hDlg)
{
	HWND hCtrl;

	/* fake a CBN_SELCHANGE event from IDC_SIZES to force it to be changed */
	hCtrl = GetDlgItem(hDlg, IDC_SIZES);
	PostMessage(hDlg, WM_COMMAND, (CBN_SELCHANGE << 16) | IDC_SIZES, (LPARAM) hCtrl);
}



static void AppendList(HWND hList, LPCTSTR lpItem, int nItem)
{
    LV_ITEM Item;
	HRESULT res;
	memset(&Item, 0, sizeof(LV_ITEM));
	Item.mask = LVIF_TEXT;
	Item.pszText = (LPTSTR) lpItem;
	Item.iItem = nItem;
	res = ListView_InsertItem(hList, &Item);
}



static BOOL SoftwareDirectories_OnInsertBrowse(HWND hDlg, BOOL bBrowse, LPCTSTR lpItem)
{
    int nItem;
    TCHAR inbuf[MAX_PATH];
    TCHAR outbuf[MAX_PATH];
    HWND hList;
	LPTSTR lpIn;
	BOOL res;

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
			ListView_GetItemText(hList, nItem, 0, inbuf, ARRAY_LENGTH(inbuf));
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
		res = ListView_DeleteItem(hList, nItem+1);
	MarkChanged(hDlg);
	return TRUE;
}



static BOOL SoftwareDirectories_OnDelete(HWND hDlg)
{
    int     nCount;
    int     nSelect;
    int     nItem;
    HWND    hList = GetDlgItem(hDlg, IDC_DIR_LIST);
	BOOL res;

	g_bModifiedSoftwarePaths = TRUE;

    nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED | LVNI_ALL);

    if (nItem == -1)
        return FALSE;

    /* Don't delete "Append" placeholder. */
    if (nItem == ListView_GetItemCount(hList) - 1)
        return FALSE;

	res = ListView_DeleteItem(hList, nItem);

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
		HWND hEdit = (HWND) (FPTR) SendMessage(hList, LVM_GETEDITCONTROL, 0, 0);
		win_set_window_text_utf8(hEdit, "");
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
        struct _stat file_stat;

        /* Don't allow empty entries. */
        if (!_tcscmp(pItem->pszText, TEXT("")))
        {
            return FALSE;
        }

        /* Check validity of edited directory. */
        if (_tstat(pItem->pszText, &file_stat) == 0
        &&  (file_stat.st_mode & S_IFDIR))
        {
            bResult = TRUE;
        }
        else
        {
            if (win_message_box_utf8(NULL, "Directory does not exist, continue anyway?", MAMEUINAME, MB_OKCANCEL) == IDOK)
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
	return (drv->m_devicelist.first(RAM)!=NULL) || DriverHasDevice(gamedrv, IO_PRINTER);
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



BOOL MessPropertiesCommand(HWND hWnd, WORD wNotifyCode, WORD wID, BOOL *changed)
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

static BOOL DirListReadControl(datamap *map, HWND dialog, HWND control, windows_options *opts, const char *option_name)
{
	int directory_count;
    LV_ITEM lvi;
	TCHAR buffer[2048];
	char *utf8_dir_list;
	int i, pos, driver_index;
	BOOL res;

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
			pos += _sntprintf(&buffer[pos], ARRAY_LENGTH(buffer) - pos, TEXT(";"));

		// retrieve the next entry
		memset(&lvi, '\0', sizeof(lvi));
		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.pszText = &buffer[pos];
		lvi.cchTextMax = ARRAY_LENGTH(buffer) - pos;
		res = ListView_GetItem(control, &lvi);

		// advance the position
		pos += _tcslen(&buffer[pos]);
	}

	utf8_dir_list = utf8_from_tstring(buffer);
	if (utf8_dir_list != NULL)
	{
		driver_index = PropertiesCurrentGame(dialog);
		SetExtraSoftwarePaths(driver_index, utf8_dir_list);
		osd_free(utf8_dir_list);
	}
	return TRUE;
}



static BOOL DirListPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *opts, const char *option_name)
{
	int driver_index, pos, new_pos, current_item;
	const char *dir_list;
	TCHAR *t_dir_list;
	TCHAR *s;
	LV_COLUMN lvc;
	RECT r;
	HRESULT res;
	BOOL b_res;

	// access the directory list, and convert to TCHARs
	driver_index = PropertiesCurrentGame(dialog);
	dir_list = GetExtraSoftwarePaths(driver_index);
	t_dir_list = tstring_from_utf8(dir_list);
	if (!t_dir_list)
		return FALSE;

	// delete all items in the list control
	b_res = ListView_DeleteAllItems(control);

	// add the column
	GetClientRect(control, &r);
	memset(&lvc, 0, sizeof(LVCOLUMN));
	lvc.mask = LVCF_WIDTH;
	lvc.cx = r.right - r.left - GetSystemMetrics(SM_CXHSCROLL);
	res = ListView_InsertColumn(control, 0, &lvc);

	// add each of the directories
	pos = 0;
	current_item = 0;
	while(t_dir_list[pos] != '\0')
	{
		// parse off this item
		s = _tcschr(&t_dir_list[pos], ';');
		if (s != NULL)
		{
			*s = '\0';
			new_pos = s - t_dir_list + 1;
		}
		else
		{
			new_pos = pos + _tcslen(&t_dir_list[pos]);
		}

		// append this item
		AppendList(control, &t_dir_list[pos], current_item);

		// advance to next item
		pos = new_pos;
		current_item++;
	}

	// finish up
	AppendList(control, TEXT(DIRLIST_NEWENTRYTEXT), current_item);
	ListView_SetItemState(control, 0, LVIS_SELECTED, LVIS_SELECTED);
	osd_free(t_dir_list);
	return TRUE;
}



static BOOL RamPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *opts, const char *option_name)
{
	int i, current_index, driver_index;
	const game_driver *gamedrv;
	UINT32 ram, current_ram;
	const char *this_ram_string;
	TCHAR* t_ramstring;
	const device_config *device;

	// identify the driver
	driver_index = PropertiesCurrentGame(dialog);
	gamedrv = drivers[driver_index];

	// clear out the combo box
	(void)ComboBox_ResetContent(control);

	// allocate the machine config
	machine_config cfg(*gamedrv,*opts);

	// identify how many options that we have
	device = cfg.m_devicelist.first(RAM);

	EnableWindow(control, (device != NULL));
	i = 0;
	// we can only do something meaningful if there is more than one option
	if (device != NULL)
	{
		ram_config *config = (ram_config *)downcast<const legacy_device_config_base *>(device)->inline_config();

		// identify the current amount of RAM
		this_ram_string = opts->value(OPTION_RAMSIZE);
		current_ram = (this_ram_string != NULL) ? ram_parse_string(this_ram_string) : 0;
		if (current_ram == 0)
			current_ram = ram_parse_string(config->default_size);

		// by default, assume index 0
		current_index = 0;

		{
			ram = ram_parse_string(config->default_size);
			t_ramstring = tstring_from_utf8(config->default_size);
			if( !t_ramstring )
				return FALSE;

			(void)ComboBox_InsertString(control, i, win_tstring_strdup(t_ramstring));
			(void)ComboBox_SetItemData(control, i, ram);

			osd_free(t_ramstring);
		}
		if (config->extra_options != NULL)
		{
			int j;
			int size = strlen(config->extra_options);
			char * const s = mame_strdup(config->extra_options);
			char * const e = s + size;
			char *p = s;
			for (j=0;j<size;j++) {
				if (p[j]==',') p[j]=0;
			}

			/* try to parse each option */
			while(p <= e)
			{
				i++;
				// identify this option
				ram = ram_parse_string(p);

				this_ram_string = p;

				t_ramstring = tstring_from_utf8(this_ram_string);
				if( !t_ramstring )
					return FALSE;

				// add this option to the combo box
				(void)ComboBox_InsertString(control, i, win_tstring_strdup(t_ramstring));
				(void)ComboBox_SetItemData(control, i, ram);

				osd_free(t_ramstring);

				// is this the current option?  record the index if so
				if (ram == current_ram)
					current_index = i;

				p+= strlen(p);
				if (p == e)
					break;

				p += 1;
			}

			osd_free(s);
		}

		// set the combo box
		(void)ComboBox_SetCurSel(control, current_index);
	}
	return TRUE;
}



//============================================================
//  DATAMAP SETUP
//============================================================

void MessBuildDataMap(datamap *properties_datamap)
{
	// MESS specific stuff
	datamap_add(properties_datamap, IDC_DIR_LIST,				DM_STRING,	NULL);
	datamap_add(properties_datamap, IDC_RAM_COMBOBOX,			DM_INT,		OPTION_RAMSIZE);

	// set up callbacks
	datamap_set_callback(properties_datamap, IDC_DIR_LIST,		DCT_READ_CONTROL,		DirListReadControl);
	datamap_set_callback(properties_datamap, IDC_DIR_LIST,		DCT_POPULATE_CONTROL,	DirListPopulateControl);
	datamap_set_callback(properties_datamap, IDC_RAM_COMBOBOX,	DCT_POPULATE_CONTROL,	RamPopulateControl);
}
