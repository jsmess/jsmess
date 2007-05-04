#ifndef PROPERTIESMS_H
#define PROPERTIESMS_H

#include "ui/properties.h"
#include "ui/datamap.h"

void MessOptionsToProp(int nGame, HWND hWnd, core_options *o);
BOOL MessPropertiesCommand(int nGame, HWND hWnd, WORD wNotifyCode, WORD wID, BOOL *changed);
void MessPropToOptions(int nGame, HWND hWnd, core_options *o);
void MessSetPropEnabledControls(HWND hWnd, core_options *o);

INT_PTR CALLBACK GameMessOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL PropSheetFilter_Config(const machine_config *drv, const game_driver *gamedrv);

void MessBuildDataMap(datamap *properties_datamap, int nGame);

#endif /* PROPERTIESMS_H */
