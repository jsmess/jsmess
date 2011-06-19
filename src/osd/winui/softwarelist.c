//============================================================
//
//  softwarelist.c - MESS's software picker
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
#include <ctype.h>

#include "emu.h"
#include "unzip.h"
#include "strconv.h"
#include "picker.h"
#include "screenshot.h"
#include "bitmask.h"
#include "winui.h"
#include "resourcems.h"
#include "mui_opts.h"
#include "softwarelist.h"
#include "mui_util.h"



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _file_info file_info;
struct _file_info
{
	char list_name[MAX_PATH];
	char description[MAX_PATH];
	char file_name[MAX_PATH];
	char publisher[MAX_PATH];
	char year[10];
	char full_name[MAX_PATH*2];

	char device[MAX_PATH];
};

typedef struct _software_list_info software_list_info;
struct _software_list_info
{
	WNDPROC old_window_proc;
	file_info **file_index;
	int file_index_length;
	const software_config *config;
};



//============================================================
//  CONSTANTS
//============================================================

static const TCHAR software_list_property_name[] = TEXT("SWLIST");




static software_list_info *GetSoftwareListInfo(HWND hwndPicker)
{
	HANDLE h;
	h = GetProp(hwndPicker, software_list_property_name);
	assert(h);
	return (software_list_info *) h;
}



LPCSTR SoftwareList_LookupFilename(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return NULL;
	return pPickerInfo->file_index[nIndex]->full_name;
}

LPCSTR SoftwareList_LookupDevice(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return NULL;
	return pPickerInfo->file_index[nIndex]->device;
}


int SoftwareList_LookupIndex(HWND hwndPicker, LPCSTR pszFilename)
{
	software_list_info *pPickerInfo;
	int i;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	for (i = 0; i < pPickerInfo->file_index_length; i++)
	{
		if (!mame_stricmp(pPickerInfo->file_index[i]->file_name, pszFilename))
			return i;
	}
	return -1;
}



iodevice_t SoftwareList_GetImageType(HWND hwndPicker, int nIndex)
{
	return IO_UNKNOWN;
}



void SoftwareList_SetDriver(HWND hwndPicker, const software_config *config)
{
	software_list_info *pPickerInfo;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	pPickerInfo->config = config;
}


BOOL SoftwareList_AddFile(HWND hwndPicker,LPCSTR pszName, LPCSTR pszListname, LPCSTR pszDescription, LPCSTR pszPublisher, LPCSTR pszYear, LPCSTR pszDevice)
{
	Picker_ResetIdle(hwndPicker);

	software_list_info *pPickerInfo;
	file_info **ppNewIndex;
	file_info *pInfo;
	int nIndex, nSize;

	// first check to see if it is already here
	if (SoftwareList_LookupIndex(hwndPicker, pszName) >= 0)
		return TRUE;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);

	// create the FileInfo structure
	nSize = sizeof(file_info);
	pInfo = (file_info *) malloc(nSize);
	if (!pInfo)
		goto error;
	memset(pInfo, 0, nSize);

	// copy the filename
	strcpy(pInfo->file_name, pszName);
	strcpy(pInfo->list_name, pszListname);
	strcpy(pInfo->description, pszDescription);
	strcpy(pInfo->publisher, pszPublisher);
	strcpy(pInfo->year, pszYear);
	strcpy(pInfo->device, pszDevice);
	sprintf(pInfo->full_name,"%s:%s", pszListname,pszName);


	ppNewIndex = (file_info**)malloc((pPickerInfo->file_index_length + 1) * sizeof(*pPickerInfo->file_index));
	memcpy(ppNewIndex,pPickerInfo->file_index,pPickerInfo->file_index_length * sizeof(*pPickerInfo->file_index));
	if (pPickerInfo->file_index) free(pPickerInfo->file_index);
	if (!ppNewIndex)
		goto error;

	nIndex = pPickerInfo->file_index_length++;
	pPickerInfo->file_index = ppNewIndex;
	pPickerInfo->file_index[nIndex] = pInfo;

	// Actually insert the item into the picker
	Picker_InsertItemSorted(hwndPicker, nIndex);
	return TRUE;

error:
	if (pInfo)
		free(pInfo);
	return FALSE;
}


static void SoftwareList_InternalClear(software_list_info *pPickerInfo)
{
	int i;

	for (i = 0; i < pPickerInfo->file_index_length; i++)
		free(pPickerInfo->file_index[i]);

	pPickerInfo->file_index = NULL;
	pPickerInfo->file_index_length = 0;
}



void SoftwareList_Clear(HWND hwndPicker)
{
	software_list_info *pPickerInfo;
	BOOL res;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	SoftwareList_InternalClear(pPickerInfo);
	res = ListView_DeleteAllItems(hwndPicker);
}


BOOL SoftwareList_Idle(HWND hwndPicker)
{
	return FALSE;
}



LPCTSTR SoftwareList_GetItemString(HWND hwndPicker, int nRow, int nColumn,
	TCHAR *pszBuffer, UINT nBufferLength)
{
	software_list_info *pPickerInfo;
	const file_info *pFileInfo;
	LPCTSTR s = NULL;
	TCHAR* t_buf;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nRow < 0) || (nRow >= pPickerInfo->file_index_length))
		return NULL;

	pFileInfo = pPickerInfo->file_index[nRow];

	switch(nColumn)
	{
		case 0:
			t_buf = tstring_from_utf8(pFileInfo->file_name);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 1:
			t_buf = tstring_from_utf8(pFileInfo->list_name);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 2:
			t_buf = tstring_from_utf8(pFileInfo->description);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 3:
			t_buf = tstring_from_utf8(pFileInfo->year);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;
		case 4:
			t_buf = tstring_from_utf8(pFileInfo->publisher);
			if( !t_buf )
				return s;
			_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_buf);
			s = pszBuffer;
			osd_free(t_buf);
			break;

	}
	return s;
}



static LRESULT CALLBACK SoftwareList_WndProc(HWND hwndPicker, UINT nMessage,
	WPARAM wParam, LPARAM lParam)
{
	software_list_info *pPickerInfo;
	LRESULT rc;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	rc = CallWindowProc(pPickerInfo->old_window_proc, hwndPicker, nMessage, wParam, lParam);

	if (nMessage == WM_DESTROY)
	{
		SoftwareList_InternalClear(pPickerInfo);
		SoftwareList_SetDriver(hwndPicker, NULL);
		free(pPickerInfo);
	}

	return rc;
}



BOOL SetupSoftwareList(HWND hwndPicker, const struct PickerOptions *pOptions)
{
	software_list_info *pPickerInfo = NULL;
	LONG_PTR l;

	if (!SetupPicker(hwndPicker, pOptions))
		goto error;

	pPickerInfo = (software_list_info *)malloc(sizeof(*pPickerInfo));
	if (!pPickerInfo)
		goto error;

	memset(pPickerInfo, 0, sizeof(*pPickerInfo));
	if (!SetProp(hwndPicker, software_list_property_name, (HANDLE) pPickerInfo))
		goto error;

	l = (LONG_PTR) SoftwareList_WndProc;
	l = SetWindowLongPtr(hwndPicker, GWLP_WNDPROC, l);
	pPickerInfo->old_window_proc = (WNDPROC) l;
	return TRUE;

error:
	if (pPickerInfo)
		free(pPickerInfo);
	return FALSE;
}
