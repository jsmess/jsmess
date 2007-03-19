//============================================================
//
//	softwarepicker.h - MESS's software picker
//
//============================================================

LPCTSTR SoftwarePicker_LookupFilename(HWND hwndPicker, int nIndex);
device_class SoftwarePicker_LookupDevice(HWND hwndPicker, int nIndex);
int SoftwarePicker_LookupIndex(HWND hwndPicker, LPCTSTR pszFilename);
iodevice_t SoftwarePicker_GetImageType(HWND hwndPicker, int nIndex);
BOOL SoftwarePicker_AddFile(HWND hwndPicker, LPCTSTR pszFilename);
BOOL SoftwarePicker_AddDirectory(HWND hwndPicker, LPCTSTR pszDirectory);
void SoftwarePicker_Clear(HWND hwndPicker);
void SoftwarePicker_SetDriver(HWND hwndPicker, const game_driver *pDriver);
void SoftwarePicker_SetErrorProc(HWND hwndPicker, void (*pfnErrorProc)(const char *message));

// PickerOptions callbacks
LPCTSTR SoftwarePicker_GetItemString(HWND hwndPicker, int nRow, int nColumn,
	TCHAR *pszBuffer, UINT nBufferLength);
BOOL SoftwarePicker_Idle(HWND hwndPicker);

BOOL SetupSoftwarePicker(HWND hwndPicker, const struct PickerOptions *pOptions);
