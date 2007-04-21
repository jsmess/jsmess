/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse
    
  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef WIN32UI_H
#define WIN32UI_H

#define WIN32_LEAN_AND_MEAN
#include <commdlg.h>
#include <driver.h>
#include "screenshot.h"

enum
{
	TAB_PICKER = 0,
	TAB_DISPLAY,
	TAB_MISC,
	NUM_TABS
};

enum
{
	FILETYPE_INPUT_FILES = 1,
	FILETYPE_SAVESTATE_FILES = 2,
	FILETYPE_WAVE_FILES = 3,
	FILETYPE_MNG_FILES = 5,
	FILETYPE_EFFECT_FILES = 6
};


typedef struct
{
	INT resource;
	const char *icon_name;
} ICONDATA;

extern char last_directory[MAX_PATH];

typedef BOOL (WINAPI *common_file_dialog_proc)(LPOPENFILENAME lpofn);
BOOL CommonFileDialog(common_file_dialog_proc cfd,char *filename, int filetype);

HWND GetMainWindow(void);
HWND GetTreeView(void);
HIMAGELIST GetLargeImageList(void);
HIMAGELIST GetSmallImageList(void);
int GetNumOptionFolders(void);
void SetNumOptionFolders(int count);
void GetRealColumnOrder(int order[]);
HICON LoadIconFromFile(const char *iconname);
void UpdateScreenShot(void);
void ResizePickerControls(HWND hWnd);
void MamePlayGame(void);
int FindIconIndex(int nIconResource);
int FindIconIndexByName(const char *icon_name);
int GetSelectedPick(void);

void UpdateListView(void);

// Move The in "The Title (notes)" to "Title, The (notes)"
char * ModifyThe(const char *str);

// Convert Ampersand so it can display in a static control
char * ConvertAmpersandString(const char *s);

// globalized for painting tree control
HBITMAP GetBackgroundBitmap(void);
HPALETTE GetBackgroundPalette(void);
MYBITMAPINFO* GetBackgroundInfo(void);
BOOL GetUseOldControl(void);
BOOL GetUseXPControl(void);

int GetMinimumScreenShotWindowWidth(void);

// we maintain an array of drivers sorted by name, useful all around
int GetDriverIndex(const game_driver *driver);
int GetParentIndex(const game_driver *driver);
int GetParentRomSetIndex(const game_driver *driver);
int GetGameNameIndex(const char *name);
int GetIndexFromSortedIndex(int sorted_index);

// sets text in part of the status bar on the main window
void SetStatusBarText(int part_index, const char *message);
void SetStatusBarTextF(int part_index, const char *fmt, ...);

int Mame32Main(HINSTANCE    hInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow);

BOOL MouseHasBeenMoved(void);

#endif
