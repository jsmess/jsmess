#ifndef DEVVIEW_H
#define DEVVIEW_H

struct DevViewCallbacks
{
	BOOL (*pfnGetOpenFileName)(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength);
	BOOL (*pfnGetCreateFileName)(HWND hwndDevView, const struct IODevice *dev, LPTSTR pszFilename, UINT nFilenameLength);
	void (*pfnSetSelectedSoftware)(HWND hwndDevView, int nGame, const struct IODevice *dev, int nID, LPCTSTR pszFilename);
	LPCTSTR (*pfnGetSelectedSoftware)(HWND hwndDevView, int nGame, const struct IODevice *dev, int nID, LPTSTR pszBuffer, UINT nBufferLength);
};

void DevView_SetCallbacks(HWND hwndDevView, const struct DevViewCallbacks *pCallbacks);
BOOL DevView_SetDriver(HWND hwndDevView, int nGame);
void DevView_RegisterClass(void);
void DevView_Refresh(HWND hwndDevView);

#endif // DEVVIEW_H
