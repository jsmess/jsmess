//============================================================
//
//  softwarelist.h - MESS's software
//
//============================================================

#include "swconfig.h"

LPCSTR SoftwareList_LookupFilename(HWND hwndPicker, int nIndex);
LPCSTR SoftwareList_LookupDevice(HWND hwndPicker, int nIndex);
int SoftwareList_LookupIndex(HWND hwndPicker, LPCSTR pszFilename);
iodevice_t SoftwareList_GetImageType(HWND hwndPicker, int nIndex);
BOOL SoftwareList_AddFile(HWND hwndPicker, LPCSTR pszName, LPCSTR pszListname, LPCSTR pszDescription, LPCSTR pszPublisher, LPCSTR pszYear, LPCSTR pszDevice);
void SoftwareList_Clear(HWND hwndPicker);
void SoftwareList_SetDriver(HWND hwndPicker, const software_config *config);

// PickerOptions callbacks
LPCTSTR SoftwareList_GetItemString(HWND hwndPicker, int nRow, int nColumn,
	TCHAR *pszBuffer, UINT nBufferLength);
BOOL SoftwareList_Idle(HWND hwndPicker);

BOOL SetupSoftwareList(HWND hwndPicker, const struct PickerOptions *pOptions);
