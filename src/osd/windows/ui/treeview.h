/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef TREEVIEW_H
#define TREEVIEW_H

/* corrections for commctrl.h */

#if defined(__GNUC__)
/* fix warning: cast does not match function type */
#undef  TreeView_InsertItem
#define TreeView_InsertItem(w,i) (HTREEITEM)(LRESULT)(int)SendMessage((w),TVM_INSERTITEM,0,(LPARAM)(LPTV_INSERTSTRUCT)(i))

#undef  TreeView_SetImageList
#define TreeView_SetImageList(w,h,i) (HIMAGELIST)(LRESULT)(int)SendMessage((w),TVM_SETIMAGELIST,i,(LPARAM)(HIMAGELIST)(h))

#undef  TreeView_GetNextItem
#define TreeView_GetNextItem(w,i,c) (HTREEITEM)(LRESULT)(int)SendMessage((w),TVM_GETNEXTITEM,c,(LPARAM)(HTREEITEM)(i))

#undef TreeView_HitTest
#define TreeView_HitTest(hwnd, lpht) \
    (HTREEITEM)(LRESULT)(int)SNDMSG((hwnd), TVM_HITTEST, 0, (LPARAM)(LPTV_HITTESTINFO)(lpht))

/* fix wrong return type */
#undef  TreeView_Select
#define TreeView_Select(w,i,c) (BOOL)(int)SendMessage((w),TVM_SELECTITEM,c,(LPARAM)(HTREEITEM)(i))

#undef TreeView_EditLabel
#define TreeView_EditLabel(w, i) \
    SNDMSG(w,TVM_EDITLABEL,0,(LPARAM)(i))

#endif /* defined(__GNUC__) */

/***************************************************************************
    Folder And Filter Definitions
 ***************************************************************************/

typedef struct
{
	const char *m_lpTitle; // Folder Title
	const char *short_name;  // for saving in the .ini
	UINT        m_nFolderId; // ID
	UINT        m_nIconId; // if >= 0, resource id of icon (IDI_xxx), otherwise index in image list
	DWORD       m_dwUnset; // Excluded filters
	DWORD       m_dwSet;   // Implied filters
	void        (*m_pfnCreateFolders)(int parent_index); // Constructor for special folders
	BOOL        (*m_pfnQuery)(int nDriver);              // Query function
	BOOL        m_bExpectedResult;                       // Expected query result
} FOLDERDATA, *LPFOLDERDATA;

typedef struct
{
	DWORD m_dwFilterType;				/* Filter value */
	DWORD m_dwCtrlID;					/* Control ID that represents it */
	BOOL (*m_pfnQuery)(int nDriver);	/* Query function */
	BOOL m_bExpectedResult;				/* Expected query result */
} FILTER_ITEM, *LPFILTER_ITEM;

/***************************************************************************
    Functions to build builtin folder lists
 ***************************************************************************/

void CreateManufacturerFolders(int parent_index);
void CreateYearFolders(int parent_index);
void CreateSourceFolders(int parent_index);
void CreateCPUFolders(int parent_index);
void CreateSoundFolders(int parent_index);
void CreateOrientationFolders(int parent_index);
void CreateDeficiencyFolders(int parent_index);
void CreateDumpingFolders(int parent_index);

/***************************************************************************/

#define MAX_EXTRA_FOLDERS 256
#define MAX_EXTRA_SUBFOLDERS 256

/* TreeView structures */
enum
{
	FOLDER_NONE = 0,
	FOLDER_ALLGAMES,
	FOLDER_AVAILABLE,
#ifdef SHOW_UNAVAILABLE_FOLDER
	FOLDER_UNAVAILABLE,
#endif
	FOLDER_MANUFACTURER,
	FOLDER_YEAR,
	FOLDER_SOURCE,
	FOLDER_CPU,
	FOLDER_SND,
	FOLDER_ORIENTATION,
	FOLDER_DEFICIENCY,
	FOLDER_WORKING,
	FOLDER_NONWORKING,
	FOLDER_ORIGINAL,
	FOLDER_CLONES,
	FOLDER_RASTER,
	FOLDER_VECTOR,
	FOLDER_TRACKBALL,
	FOLDER_LIGHTGUN,
	FOLDER_STEREO,
 	//FOLDER_MULTIMON,
	FOLDER_HARDDISK,
	FOLDER_SAMPLES,
	FOLDER_DUMPING,
	FOLDER_SAVESTATE,
	FOLDER_BIOS,
#ifdef MESS
	FOLDER_CONSOLE,
	FOLDER_COMPUTER,
	FOLDER_MODIFIED,
	FOLDER_MOUSE,
#endif
	MAX_FOLDERS,
};

typedef enum
{
	F_CLONES        = 0x00000001,
	F_NONWORKING    = 0x00000002,
	F_UNAVAILABLE   = 0x00000004,
	F_VECTOR        = 0x00000008,
	F_RASTER        = 0x00000010,
	F_ORIGINALS     = 0x00000020,
	F_WORKING       = 0x00000040,
	F_AVAILABLE     = 0x00000080,
#ifdef MESS
	F_COMPUTER      = 0x00000200,
	F_CONSOLE       = 0x00000400,
	F_MODIFIED      = 0x00000800,
#endif
	F_MASK          = 0x00000FFF,
	F_CUSTOM        = 0x01000000  // for current .ini custom folders
} FOLDERFLAG;

typedef struct
{
    LPSTR m_lpTitle;        // String contains the folder name
    UINT        m_nFolderId;      // Index / Folder ID number
    int         m_nParent;        // Parent folder index in treeFolders[]
    int         m_nIconId;        // negative icon index into the ImageList, or IDI_xxx resource id
    DWORD       m_dwFlags;        // Misc flags
    LPBITS      m_lpGameBits;     // Game bits, represent game indices
} TREEFOLDER, *LPTREEFOLDER;

typedef struct
{
    char        m_szTitle[64];  // Folder Title
    UINT        m_nFolderId;    // ID
    int         m_nParent;      // Parent Folder index in treeFolders[]
    DWORD       m_dwFlags;      // Flags - Customizable and Filters
    int         m_nIconId;      // negative icon index into the ImageList, or IDI_xxx resource id
    int         m_nSubIconId;   // negative icon index into the ImageList, or IDI_xxx resource id
} EXFOLDERDATA, *LPEXFOLDERDATA;

void FreeFolders(void);
void ResetFilters(void);
void InitTree(LPFOLDERDATA lpFolderData, LPFILTER_ITEM lpFilterList);
void SetCurrentFolder(LPTREEFOLDER lpFolder);
UINT GetCurrentFolderID(void);

LPTREEFOLDER GetCurrentFolder(void);
int GetNumFolders(void);
LPTREEFOLDER GetFolder(UINT nFolder);
LPTREEFOLDER GetFolderByID(UINT nID);
LPTREEFOLDER GetFolderByName(int nParentId, const char *pszFolderName);

void AddGame(LPTREEFOLDER lpFolder, UINT nGame);
void RemoveGame(LPTREEFOLDER lpFolder, UINT nGame);
int  FindGame(LPTREEFOLDER lpFolder, int nGame);

void ResetWhichGamesInFolders(void);

LPFOLDERDATA FindFilter(DWORD folderID);

BOOL GameFiltered(int nGame, DWORD dwFlags);
BOOL GetParentFound(int nGame);

LPFILTER_ITEM GetFilterList(void);

void SetTreeIconSize(HWND hWnd, BOOL bLarge);
BOOL GetTreeIconSize(void);

void GetFolders(TREEFOLDER ***folders,int *num_folders);
BOOL TryRenameCustomFolder(LPTREEFOLDER lpFolder,const char *new_name);
void AddToCustomFolder(LPTREEFOLDER lpFolder,int driver_index);
void RemoveFromCustomFolder(LPTREEFOLDER lpFolder,int driver_index);

HIMAGELIST GetTreeViewIconList(void);
int GetTreeViewIconIndex(int icon_id);

void ResetTreeViewFolders(void);
void SelectTreeViewFolder(int folder_id);

#endif /* TREEVIEW_H */
