/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef TABVIEW_H
#define TABVIEW_H

struct TabViewCallbacks
{
	// Options retrieval
	BOOL (*pfnGetShowTabCtrl)(void);
	void (*pfnSetCurrentTab)(LPCTSTR pszShortName);
	LPCTSTR (*pfnGetCurrentTab)(void);
	void (*pfnSetShowTab)(int nTab, BOOL show);
	int (*pfnGetShowTab)(int nTab);

	// Accessors
	LPCTSTR (*pfnGetTabShortName)(int nTab);
	LPCTSTR (*pfnGetTabLongName)(int nTab);

	// Callbacks
	void (*pfnOnSelectionChanged)(void);
	void (*pfnOnMoveSize)(void);
};

struct TabViewOptions
{
	const struct TabViewCallbacks *pCallbacks;
	int nTabCount;
};


BOOL SetupTabView(HWND hwndTabView, const struct TabViewOptions *pOptions);

void TabView_Reset(HWND hwndTabView);
void TabView_CalculateNextTab(HWND hwndTabView);
int TabView_GetCurrentTab(HWND hwndTabView);
void TabView_SetCurrentTab(HWND hwndTabView, int nTab);
void TabView_UpdateSelection(HWND hwndTabView);

// These are used to handle events received by the parent regarding
// tabview controls
BOOL TabView_HandleNotify(LPNMHDR lpNmHdr);

#endif // TABVIEW_H
