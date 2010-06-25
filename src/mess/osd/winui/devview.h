#ifndef DEVVIEW_H
#define DEVVIEW_H

#include "swconfig.h"


struct DevViewCallbacks
{
	BOOL (*pfnGetOpenFileName)(HWND hwndDevView, const machine_config *config, const device_config_image_interface *dev, LPTSTR pszFilename, UINT nFilenameLength);
	BOOL (*pfnGetCreateFileName)(HWND hwndDevView, const machine_config *config, const device_config_image_interface *dev, LPTSTR pszFilename, UINT nFilenameLength);
	void (*pfnSetSelectedSoftware)(HWND hwndDevView, int nGame, const machine_config *config, const device_config_image_interface *dev, LPCTSTR pszFilename);
	LPCTSTR (*pfnGetSelectedSoftware)(HWND hwndDevView, int nGame, const machine_config *config, const device_config_image_interface *dev, LPTSTR pszBuffer, UINT nBufferLength);
};

void DevView_SetCallbacks(HWND hwndDevView, const struct DevViewCallbacks *pCallbacks);
BOOL DevView_SetDriver(HWND hwndDevView, const software_config *config);
void DevView_RegisterClass(void);
void DevView_Refresh(HWND hwndDevView);

#endif // DEVVIEW_H
