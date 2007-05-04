#ifndef PROPERTIESMS_H
#define PROPERTIESMS_H

#include "ui/properties.h"
#include "ui/datamap.h"

BOOL MessPropertiesCommand(int nGame, HWND hWnd, WORD wNotifyCode, WORD wID, BOOL *changed);

INT_PTR CALLBACK GameMessOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL PropSheetFilter_Config(const machine_config *drv, const game_driver *gamedrv);

void MessBuildDataMap(datamap *properties_datamap);

#endif /* PROPERTIESMS_H */
