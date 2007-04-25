#ifndef __MESS32UI_H__
#define __MESS32UI_H__

#include "driver.h"
#include "resourcems.h"
#include "devview.h"
#include "uitext.h"
#include "options.h"

extern TCHAR g_szSelectedItem[MAX_PATH];

void InitMessPicker(void);
void MessUpdateSoftwareList(void);
void MyFillSoftwareList(int nGame, BOOL bForce);
BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
void MessReadMountedSoftware(int nGame);
BOOL CreateMessIcons(void);
BOOL MessApproveImageList(HWND hParent, int nDriver);

void MessCopyDeviceOption(core_options *opts, const game_driver *gamedrv, const device_class *devclass, int device_index, int global_index);

#endif // __MESS32UI_H__
