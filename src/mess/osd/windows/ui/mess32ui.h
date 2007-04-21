#include "resourcems.h"
#include "devview.h"
#include "uitext.h"

extern TCHAR g_szSelectedItem[MAX_PATH];

void InitMessPicker(void);
void MessUpdateSoftwareList(void);
void MyFillSoftwareList(int nGame, BOOL bForce);
BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
void MessReadMountedSoftware(int nGame);
BOOL CreateMessIcons(void);
BOOL MessApproveImageList(HWND hParent, int nDriver);
void MessCreateCommandLine(char *pCmdLine, core_options *pOpts, const game_driver *gamedrv);

