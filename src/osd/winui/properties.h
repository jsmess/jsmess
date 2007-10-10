/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef PROPERTIES_H
#define PROPERTIES_H

/* Get title string to display in the top of the property page,
 * Called also in Audit32.c
 */
char * GameInfoTitle(OPTIONS_TYPE opt_type, UINT nIndex);

/* Called in win32ui.c to create the property page */
void	InitPropertyPage(HINSTANCE hInst, HWND hWnd, HICON hIcon, OPTIONS_TYPE opt_type, int folder_id, int game_num);

#define PROPERTIES_PAGE 0
#define AUDIT_PAGE      1   

void	InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, HICON hIcon, OPTIONS_TYPE opt_type, int folder_id, int game_num, int start_page);
void    InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd);

/* Get Help ID array for WM_HELP and WM_CONTEXTMENU */
DWORD_PTR   GetHelpIDs(void);

/* Get Game status text string */
const char *GameInfoStatus(int driver_index, BOOL bRomStatus);

/* Property sheet info for layout.c */
typedef struct
{
	BOOL bOnDefaultPage;
	BOOL (*pfnFilterProc)(const machine_config *drv, const game_driver *gamedrv);
	DWORD dwDlgID;
	DLGPROC pfnDlgProc;
} PROPERTYSHEETINFO;

extern const PROPERTYSHEETINFO g_propSheets[];

BOOL PropSheetFilter_Vector(const machine_config *drv, const game_driver *gamedrv);

INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

int PropertiesCurrentGame(HWND hDlg);

#endif
