/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Directories.c

***************************************************************************/

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <sys/stat.h>
#include <assert.h>

#include "screenshot.h"
#include "MAME32.h"
#include "Directories.h"
#include "resource.h"

#define MAX_DIRS 20

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	char    m_Directories[MAX_DIRS][MAX_PATH];
	int     m_NumDirectories;
	BOOL    m_bModified;
} tPath;

typedef union
{
	tPath	*m_Path;
	char	*m_Directory;
	void    *m_Ptr;
} tDirInfo;

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
#ifdef MESS
BOOL BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult);
#else
static BOOL         BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult);
#endif

static void     DirInfo_SetDir(tDirInfo *pInfo, int nType, int nItem, const char* pText);
static char*    DirInfo_Dir(tDirInfo *pInfo, int nType);
static char*    DirInfo_Path(tDirInfo *pInfo, int nType, int nItem);
static void     DirInfo_SetModified(tDirInfo *pInfo, int nType, BOOL bModified);
static BOOL     DirInfo_Modified(tDirInfo *pInfo, int nType);
static char*    FixSlash(char *s);

static void     UpdateDirectoryList(HWND hDlg);

static void     Directories_OnSelChange(HWND hDlg);
static BOOL     Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void     Directories_OnDestroy(HWND hDlg);
static void     Directories_OnClose(HWND hDlg);
static void     Directories_OnOk(HWND hDlg);
static void     Directories_OnCancel(HWND hDlg);
static void     Directories_OnInsert(HWND hDlg);
static void     Directories_OnBrowse(HWND hDlg);
static void     Directories_OnDelete(HWND hDlg);
static BOOL     Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static BOOL     Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static void     Directories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
static BOOL     Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static tDirInfo *g_pDirInfo;

/***************************************************************************
    External function definitions
 ***************************************************************************/

INT_PTR CALLBACK DirectoriesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL bReturn = FALSE;

	switch (Msg)
	{
	case WM_INITDIALOG:
		return (BOOL)HANDLE_WM_INITDIALOG(hDlg, wParam, lParam, Directories_OnInitDialog);

	case WM_COMMAND:
		HANDLE_WM_COMMAND(hDlg, wParam, lParam, Directories_OnCommand);
		bReturn = TRUE;
		break;

	case WM_NOTIFY:
		return (BOOL)HANDLE_WM_NOTIFY(hDlg, wParam, lParam, Directories_OnNotify);
		break;

	case WM_CLOSE:
		HANDLE_WM_CLOSE(hDlg, wParam, lParam, Directories_OnClose);
		break;

	case WM_DESTROY:
		HANDLE_WM_DESTROY(hDlg, wParam, lParam, Directories_OnDestroy);
		break;

	default:
		bReturn = FALSE;
	}
	return bReturn;
}

/***************************************************************************
	Internal function definitions
 ***************************************************************************/

static BOOL IsMultiDir(int nType)
{
	return g_directoryInfo[nType].bMulti;
}

static void DirInfo_SetDir(tDirInfo *pInfo, int nType, int nItem, const char* pText)
{
	char *s;
	char *pOldText;

	if (IsMultiDir(nType))
	{
		assert(nItem >= 0);
		strcpy(DirInfo_Path(pInfo, nType, nItem), pText);
		DirInfo_SetModified(pInfo, nType, TRUE);
	}
	else
	{
		s = mame_strdup(pText);
		if (!s)
			return;
		pOldText = pInfo[nType].m_Directory;
		if (pOldText)
			free(pOldText);
		pInfo[nType].m_Directory = s;
	}
}

static char* DirInfo_Dir(tDirInfo *pInfo, int nType)
{
	assert(!IsMultiDir(nType));
	return pInfo[nType].m_Directory;
}

static char* DirInfo_Path(tDirInfo *pInfo, int nType, int nItem)
{
	return pInfo[nType].m_Path->m_Directories[nItem];
}

static void DirInfo_SetModified(tDirInfo *pInfo, int nType, BOOL bModified)
{
	assert(IsMultiDir(nType));
	pInfo[nType].m_Path->m_bModified = bModified;
}

static BOOL DirInfo_Modified(tDirInfo *pInfo, int nType)
{
	assert(IsMultiDir(nType));
	return pInfo[nType].m_Path->m_bModified;
}

#define DirInfo_NumDir(pInfo, path) \
	((pInfo)[(path)].m_Path->m_NumDirectories)

/* lop off trailing backslash if it exists */
static char * FixSlash(char *s)
{
	int len = 0;
	
	if (s)
		len = strlen(s);

	if (len && s[len - 1] == '\\')
		s[len - 1] = '\0';

	return s;
}

static void UpdateDirectoryList(HWND hDlg)
{
	int 	i;
	int 	nType;
	LV_ITEM Item;
	HWND	hList  = GetDlgItem(hDlg, IDC_DIR_LIST);
	HWND	hCombo = GetDlgItem(hDlg, IDC_DIR_COMBO);

	/* Remove previous */

	ListView_DeleteAllItems(hList);

	/* Update list */

	memset(&Item, 0, sizeof(LV_ITEM));
	Item.mask = LVIF_TEXT;

	nType = ComboBox_GetCurSel(hCombo);
	if (IsMultiDir(nType))
	{
		Item.pszText = (char *) DIRLIST_NEWENTRYTEXT;
		ListView_InsertItem(hList, &Item);
		for (i = DirInfo_NumDir(g_pDirInfo, nType) - 1; 0 <= i; i--)
		{
			Item.pszText = DirInfo_Path(g_pDirInfo, nType, i);
			ListView_InsertItem(hList, &Item);
		}
	}
	else
	{
		Item.pszText = (char *) DirInfo_Dir(g_pDirInfo, nType);
		ListView_InsertItem(hList, &Item);
	}

	/* select first one */

	ListView_SetItemState(hList, 0, LVIS_SELECTED, LVIS_SELECTED);
}

static void Directories_OnSelChange(HWND hDlg)
{
	UpdateDirectoryList(hDlg);
	
	if (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO)) == 0
	||	ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO)) == 1)
	{
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_DELETE), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_INSERT), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_DELETE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_INSERT), FALSE);
	}
}

static BOOL Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	RECT		rectClient;
	LVCOLUMN	LVCol;
	int 		i;
	int			nDirInfoCount;
	LPCSTR		s;
	char *token;
	char buf[MAX_PATH * MAX_DIRS];

	/* count how many dirinfos there are */
	nDirInfoCount = 0;
	while(g_directoryInfo[nDirInfoCount].lpName)
		nDirInfoCount++;

	g_pDirInfo = (tDirInfo *) malloc(sizeof(tDirInfo) * nDirInfoCount);
	if (!g_pDirInfo) /* bummer */
		goto error;
	memset(g_pDirInfo, 0, sizeof(tDirInfo) * nDirInfoCount);

	for (i = nDirInfoCount - 1; i >= 0; i--)
	{
		ComboBox_InsertString(GetDlgItem(hDlg, IDC_DIR_COMBO), 0, g_directoryInfo[i].lpName);
	}

	ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO), 0);

	GetClientRect(GetDlgItem(hDlg, IDC_DIR_LIST), &rectClient);

	memset(&LVCol, 0, sizeof(LVCOLUMN));
	LVCol.mask	  = LVCF_WIDTH;
	LVCol.cx	  = rectClient.right - rectClient.left - GetSystemMetrics(SM_CXHSCROLL);
	
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_DIR_LIST), 0, &LVCol);

	/* Keep a temporary copy of the directory strings in g_pDirInfo. */
	for (i = 0; i < nDirInfoCount; i++)
	{
		s = g_directoryInfo[i].pfnGetTheseDirs();
		if (g_directoryInfo[i].bMulti)
		{
			/* Copy the string to our own buffer so that we can mutilate it */
			strcpy(buf, s);

			g_pDirInfo[i].m_Path = malloc(sizeof(tPath));
			if (!g_pDirInfo[i].m_Path)
				goto error;

			g_pDirInfo[i].m_Path->m_NumDirectories = 0;
			token = strtok(buf, ";");
			while ((DirInfo_NumDir(g_pDirInfo, i) < MAX_DIRS) && token)
			{
				strcpy(DirInfo_Path(g_pDirInfo, i, DirInfo_NumDir(g_pDirInfo, i)), token);
				DirInfo_NumDir(g_pDirInfo, i)++;
				token = strtok(NULL, ";");
			}
			DirInfo_SetModified(g_pDirInfo, i, FALSE);
		}
		else
		{
			DirInfo_SetDir(g_pDirInfo, i, -1, s);
		}
	}

	UpdateDirectoryList(hDlg);
	return TRUE;

error:
	Directories_OnDestroy(hDlg);
	EndDialog(hDlg, -1);
	return FALSE;
}

static void Directories_OnDestroy(HWND hDlg)
{
	int nDirInfoCount, i;

	if (g_pDirInfo)
	{
		/* count how many dirinfos there are */
		nDirInfoCount = 0;
		while(g_directoryInfo[nDirInfoCount].lpName)
			nDirInfoCount++;

		for (i = 0; i < nDirInfoCount; i++)
		{
			if (g_pDirInfo[i].m_Ptr)
				free(g_pDirInfo[i].m_Ptr);
		}
		free(g_pDirInfo);
		g_pDirInfo = NULL;
	}
}

static void Directories_OnClose(HWND hDlg)
{
	EndDialog(hDlg, IDCANCEL);
}

static int RetrieveDirList(int nDir, int nFlagResult, void (*SetTheseDirs)(const char *s))
{
	int i;
	int nResult = 0;
	int nPaths;
	char buf[MAX_PATH * MAX_DIRS];

	if (DirInfo_Modified(g_pDirInfo, nDir))
	{
		memset(buf, 0, sizeof(buf));
		nPaths = DirInfo_NumDir(g_pDirInfo, nDir);
		for (i = 0; i < nPaths; i++)
		{

			strcat(buf, FixSlash(DirInfo_Path(g_pDirInfo, nDir, i)));

			if (i < nPaths - 1)
				strcat(buf, ";");
		}
		SetTheseDirs(buf);

		nResult |= nFlagResult;
    }
	return nResult;
}

static void Directories_OnOk(HWND hDlg)
{
	int i;
	int nResult = 0;
	LPSTR s;

	for (i = 0; g_directoryInfo[i].lpName; i++)
	{
		if (g_directoryInfo[i].bMulti)
		{
			nResult |= RetrieveDirList(i, g_directoryInfo[i].nDirDlgFlags, g_directoryInfo[i].pfnSetTheseDirs);
		}
		else
		{
			s = FixSlash(DirInfo_Dir(g_pDirInfo, i));
			g_directoryInfo[i].pfnSetTheseDirs(s);
		}
	}
	EndDialog(hDlg, nResult);
}

static void Directories_OnCancel(HWND hDlg)
{
	EndDialog(hDlg, IDCANCEL);
}

static void Directories_OnInsert(HWND hDlg)
{
	int 	nItem;
	char	buf[MAX_PATH];
	HWND	hList;

	hList = GetDlgItem(hDlg, IDC_DIR_LIST);
	nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

	if (BrowseForDirectory(hDlg, NULL, buf) == TRUE)
	{
		int 	i;
		int 	nType;

		/* list was empty */
		if (nItem == -1)
			nItem = 0;

		nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
		if (IsMultiDir(nType))
		{
			if (MAX_DIRS <= DirInfo_NumDir(g_pDirInfo, nType))
				return;

			for (i = DirInfo_NumDir(g_pDirInfo, nType); nItem < i; i--)
				strcpy(DirInfo_Path(g_pDirInfo, nType, i),
					   DirInfo_Path(g_pDirInfo, nType, i - 1));

			strcpy(DirInfo_Path(g_pDirInfo, nType, nItem), buf);
			DirInfo_NumDir(g_pDirInfo, nType)++;
			DirInfo_SetModified(g_pDirInfo, nType, TRUE);
		}

		UpdateDirectoryList(hDlg);

		ListView_SetItemState(hList, nItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

static void Directories_OnBrowse(HWND hDlg)
{
	int 	nType;
	int 	nItem;
	char	inbuf[MAX_PATH];
	char	outbuf[MAX_PATH];
	HWND	hList;

	hList = GetDlgItem(hDlg, IDC_DIR_LIST);
	nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

	if (nItem == -1)
		return;

	nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		/* Last item is placeholder for append */
		if (nItem == ListView_GetItemCount(hList) - 1)
		{
			Directories_OnInsert(hDlg); 
			return;
		}
	}

	ListView_GetItemText(hList, nItem, 0, inbuf, MAX_PATH);

	if (BrowseForDirectory(hDlg, inbuf, outbuf) == TRUE)
	{
		nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
		DirInfo_SetDir(g_pDirInfo, nType, nItem, outbuf);
		UpdateDirectoryList(hDlg);
	}
}

static void Directories_OnDelete(HWND hDlg)
{
	int 	nType;
	int 	nCount;
	int 	nSelect;
	int 	i;
	int 	nItem;
	HWND	hList = GetDlgItem(hDlg, IDC_DIR_LIST);

	nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED | LVNI_ALL);

	if (nItem == -1)
		return;

	/* Don't delete "Append" placeholder. */
	if (nItem == ListView_GetItemCount(hList) - 1)
		return;

	nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		for (i = nItem; i < DirInfo_NumDir(g_pDirInfo, nType) - 1; i++)
			strcpy(DirInfo_Path(g_pDirInfo, nType, i),
				   DirInfo_Path(g_pDirInfo, nType, i + 1));

		strcpy(DirInfo_Path(g_pDirInfo, nType, DirInfo_NumDir(g_pDirInfo, nType) - 1), "");
		DirInfo_NumDir(g_pDirInfo, nType)--;

		DirInfo_SetModified(g_pDirInfo, nType, TRUE);
	}

	UpdateDirectoryList(hDlg);
	

	nCount = ListView_GetItemCount(hList);
	if (nCount <= 1)
		return;

	/* If the last item was removed, select the item above. */
	if (nItem == nCount - 1)
		nSelect = nCount - 2;
	else
		nSelect = nItem;

	ListView_SetItemState(hList, nSelect, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

static BOOL Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	int 		  nType;
	BOOL		  bResult = FALSE;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEM* 	  pItem = &pDispInfo->item;

	nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		/* Last item is placeholder for append */
		if (pItem->iItem == ListView_GetItemCount(GetDlgItem(hDlg, IDC_DIR_LIST)) - 1)
		{
			HWND hEdit;

			if (MAX_DIRS <= DirInfo_NumDir(g_pDirInfo, nType))
				return TRUE; /* don't edit */

			hEdit = (HWND)(LRESULT)(int)SendDlgItemMessage(hDlg, IDC_DIR_LIST, LVM_GETEDITCONTROL, 0, 0);
			Edit_SetText(hEdit, "");
		}
	}

	return bResult;
}

static BOOL Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	BOOL		  bResult = FALSE;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEM* 	  pItem = &pDispInfo->item;

	if (pItem->pszText != NULL)
	{
		struct stat file_stat;

		/* Don't allow empty entries. */
		if (!strcmp(pItem->pszText, ""))
		{
			return FALSE;
		}

		/* Check validity of edited directory. */
		if (stat(pItem->pszText, &file_stat) == 0
		&&	(file_stat.st_mode & S_IFDIR))
		{
			bResult = TRUE;
		}
		else
		{
			if (MessageBox(NULL, "Directory does not exist, continue anyway?", MAME32NAME, MB_OKCANCEL) == IDOK)
				bResult = TRUE;
		}
	}

	if (bResult == TRUE)
	{
		int nType;
		int i;

		nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
		if (IsMultiDir(nType))
		{
			/* Last item is placeholder for append */
			if (pItem->iItem == ListView_GetItemCount(GetDlgItem(hDlg, IDC_DIR_LIST)) - 1)
			{
				if (MAX_DIRS <= DirInfo_NumDir(g_pDirInfo, nType))
					return FALSE;

				for (i = DirInfo_NumDir(g_pDirInfo, nType); pItem->iItem < i; i--)
					strcpy(DirInfo_Path(g_pDirInfo, nType, i),
						   DirInfo_Path(g_pDirInfo, nType, i - 1));

				strcpy(DirInfo_Path(g_pDirInfo, nType, pItem->iItem), pItem->pszText);

				DirInfo_SetModified(g_pDirInfo, nType, TRUE);
				DirInfo_NumDir(g_pDirInfo, nType)++;
			}
			else
				DirInfo_SetDir(g_pDirInfo, nType, pItem->iItem, pItem->pszText);
		}
		else
		{
			DirInfo_SetDir(g_pDirInfo, nType, pItem->iItem, pItem->pszText);
		}

		UpdateDirectoryList(hDlg);
		ListView_SetItemState(GetDlgItem(hDlg, IDC_DIR_LIST), pItem->iItem,
							  LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

	}

	return bResult;
}

static void Directories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		if (codeNotify == BN_CLICKED)
			Directories_OnOk(hDlg);
		break;

	case IDCANCEL:
		if (codeNotify == BN_CLICKED)
			Directories_OnCancel(hDlg);
		break;

	case IDC_DIR_BROWSE:
		if (codeNotify == BN_CLICKED)
			Directories_OnBrowse(hDlg);
		break;

	case IDC_DIR_INSERT:
		if (codeNotify == BN_CLICKED)
			Directories_OnInsert(hDlg);
		break;

	case IDC_DIR_DELETE:
		if (codeNotify == BN_CLICKED)
			Directories_OnDelete(hDlg);
		break;

	case IDC_DIR_COMBO:
		switch (codeNotify)
		{
		case CBN_SELCHANGE:
			Directories_OnSelChange(hDlg);
			break;
		}
		break;
	}
}

static BOOL Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR)
{
	switch (id)
	{
	case IDC_DIR_LIST:
		switch (pNMHDR->code)
		{
		case LVN_ENDLABELEDIT:
			return Directories_OnEndLabelEdit(hDlg, pNMHDR);

		case LVN_BEGINLABELEDIT:
			return Directories_OnBeginLabelEdit(hDlg, pNMHDR);
		}
	}
	return FALSE;
}

/**************************************************************************

	Use the shell to select a Directory.

 **************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	/*
		Called just after the dialog is initialized
		Select the dir passed in BROWSEINFO.lParam
	*/
	if (uMsg == BFFM_INITIALIZED)
	{
		if ((const char*)lpData != NULL)
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

BOOL BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult) 
{
	BOOL		bResult = FALSE;
	IMalloc*	piMalloc = 0;
	BROWSEINFO	Info;
	ITEMIDLIST* pItemIDList = NULL;
	char		buf[MAX_PATH];
	
	if (!SUCCEEDED(SHGetMalloc(&piMalloc)))
		return FALSE;

	Info.hwndOwner		= hwnd;
	Info.pidlRoot		= NULL;
	Info.pszDisplayName = buf;
	Info.lpszTitle		= (LPCSTR)"Directory name:";
	Info.ulFlags		= BIF_RETURNONLYFSDIRS;
	Info.lpfn			= BrowseCallbackProc;
	Info.lParam 		= (LPARAM)pStartDir;

	pItemIDList = SHBrowseForFolder(&Info);

	if (pItemIDList != NULL)
	{
		if (SHGetPathFromIDList(pItemIDList, buf) == TRUE)
		{
			strncpy(pResult, buf, MAX_PATH);
			bResult = TRUE;
		}
		IMalloc_Free(piMalloc, pItemIDList);
	}
	else
	{
		bResult = FALSE;
	}

	IMalloc_Release(piMalloc);
	return bResult;
}

/**************************************************************************/


