/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  directories.c

***************************************************************************/

// standard windows headers
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#undef CINTERFACE

// standard C headers
#include <sys/stat.h>
#include <assert.h>
#include <tchar.h>

// MAME/MAMEUI headers
#include "winui.h"
#include "directories.h"
#include "resource.h"
#include "strconv.h"
#include "mui_util.h"
#define CINTERFACE
#include <shlobj.h>
#undef CINTERFACE

#define MAX_DIRS 128

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	TCHAR   m_Directories[MAX_DIRS][MAX_PATH];
	int     m_NumDirectories;
	BOOL    m_bModified;
} tPath;

typedef struct
{
	tPath	*m_Path;
	TCHAR	*m_tDirectory;
} tDirInfo;

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
BOOL            BrowseForDirectory(HWND hwnd, LPCTSTR pStartDir, TCHAR* pResult);

static void     DirInfo_SetDir(tDirInfo *pInfo, int nType, int nItem, LPCTSTR pText);
static TCHAR*   DirInfo_Dir(tDirInfo *pInfo, int nType);
static TCHAR*   DirInfo_Path(tDirInfo *pInfo, int nType, int nItem);
static void     DirInfo_SetModified(tDirInfo *pInfo, int nType, BOOL bModified);
static BOOL     DirInfo_Modified(tDirInfo *pInfo, int nType);
static TCHAR*   FixSlash(TCHAR *s);

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

static void DirInfo_SetDir(tDirInfo *pInfo, int nType, int nItem, LPCTSTR pText)
{
	TCHAR *t_s;
	TCHAR *t_pOldText;

	if (IsMultiDir(nType))
	{
		assert(nItem >= 0);
		_tcscpy(DirInfo_Path(pInfo, nType, nItem), pText);
		DirInfo_SetModified(pInfo, nType, TRUE);
	}
	else
	{
		t_s = win_tstring_strdup(pText);
		if (!t_s)
			return;
		t_pOldText = pInfo[nType].m_tDirectory;
		if (t_pOldText)
			osd_free(t_pOldText);
		pInfo[nType].m_tDirectory = t_s;
	}
}

static TCHAR* DirInfo_Dir(tDirInfo *pInfo, int nType)
{
	assert(!IsMultiDir(nType));
	return pInfo[nType].m_tDirectory;
}

static TCHAR* DirInfo_Path(tDirInfo *pInfo, int nType, int nItem)
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
static TCHAR * FixSlash(TCHAR *s)
{
	int len = 0;
	
	if (s)
		len = _tcslen(s);

	if (len>3 && s[len - 1] == '\\')
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
	BOOL	b_res;

	/* Remove previous */

	b_res = ListView_DeleteAllItems(hList);

	/* Update list */

	memset(&Item, 0, sizeof(LV_ITEM));
	Item.mask = LVIF_TEXT;

	nType = ComboBox_GetCurSel(hCombo);
	if (IsMultiDir(nType))
	{
		Item.pszText = (TCHAR*) TEXT(DIRLIST_NEWENTRYTEXT);
		(void)ListView_InsertItem(hList, &Item);
		for (i = DirInfo_NumDir(g_pDirInfo, nType) - 1; 0 <= i; i--)
		{
			Item.pszText = DirInfo_Path(g_pDirInfo, nType, i);
			(void)ListView_InsertItem(hList, &Item);
		}
	}
	else
	{
		Item.pszText = DirInfo_Dir(g_pDirInfo, nType);
		(void)ListView_InsertItem(hList, &Item);
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
	TCHAR       *token;
	TCHAR       buf[MAX_PATH * MAX_DIRS];
	TCHAR*      t_s = NULL;
	HRESULT 	res;

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
		t_s = tstring_from_utf8(g_directoryInfo[i].lpName);
		if( !t_s )
			return FALSE;
		(void)ComboBox_InsertString(GetDlgItem(hDlg, IDC_DIR_COMBO), 0, win_tstring_strdup(t_s));
		osd_free(t_s);
		t_s = NULL;
	}

	(void)ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO), 0);

	GetClientRect(GetDlgItem(hDlg, IDC_DIR_LIST), &rectClient);

	memset(&LVCol, 0, sizeof(LVCOLUMN));
	LVCol.mask	  = LVCF_WIDTH;
	LVCol.cx	  = rectClient.right - rectClient.left - GetSystemMetrics(SM_CXHSCROLL);
	
	res = ListView_InsertColumn(GetDlgItem(hDlg, IDC_DIR_LIST), 0, &LVCol);

	/* Keep a temporary copy of the directory strings in g_pDirInfo. */
	for (i = 0; i < nDirInfoCount; i++)
	{
		s = g_directoryInfo[i].pfnGetTheseDirs();
		t_s = tstring_from_utf8(s);
		if( !t_s )
			return FALSE;
		if (g_directoryInfo[i].bMulti)
		{
			/* Copy the string to our own buffer so that we can mutilate it */
			_tcscpy(buf, t_s);

			g_pDirInfo[i].m_Path = (tPath*)malloc(sizeof(tPath));
			if (!g_pDirInfo[i].m_Path)
				goto error;

			g_pDirInfo[i].m_Path->m_NumDirectories = 0;
			token = _tcstok(buf, TEXT(";"));
			while ((DirInfo_NumDir(g_pDirInfo, i) < MAX_DIRS) && token)
			{
				_tcscpy(DirInfo_Path(g_pDirInfo, i, DirInfo_NumDir(g_pDirInfo, i)), token);
				DirInfo_NumDir(g_pDirInfo, i)++;
				token = _tcstok(NULL, TEXT(";"));
			}
			DirInfo_SetModified(g_pDirInfo, i, FALSE);
		}
		else
		{
			DirInfo_SetDir(g_pDirInfo, i, -1, t_s);
		}
		osd_free(t_s);
		t_s = NULL;
	}

	UpdateDirectoryList(hDlg);
	return TRUE;

error:
	if( t_s )
		osd_free(t_s);
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
			if (g_pDirInfo[i].m_Path)
				free(g_pDirInfo[i].m_Path);
			if (g_pDirInfo[i].m_tDirectory)
				osd_free(g_pDirInfo[i].m_tDirectory);
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
	TCHAR buf[MAX_PATH * MAX_DIRS];
	char* utf8_buf;

	if (DirInfo_Modified(g_pDirInfo, nDir))
	{
		memset(buf, 0, sizeof(buf));
		nPaths = DirInfo_NumDir(g_pDirInfo, nDir);
		for (i = 0; i < nPaths; i++)
		{
			_tcscat(buf, FixSlash(DirInfo_Path(g_pDirInfo, nDir, i)));

			if (i < nPaths - 1)
				_tcscat(buf, TEXT(";"));
		}
		utf8_buf = utf8_from_tstring(buf);
		SetTheseDirs(utf8_buf);
		osd_free(utf8_buf);

		nResult |= nFlagResult;
    }
	return nResult;
}

static void Directories_OnOk(HWND hDlg)
{
	int i;
	int nResult = 0;
	LPTSTR s;
	char* utf8_s;

	for (i = 0; g_directoryInfo[i].lpName; i++)
	{
		if (g_directoryInfo[i].bMulti)
		{
			nResult |= RetrieveDirList(i, g_directoryInfo[i].nDirDlgFlags, g_directoryInfo[i].pfnSetTheseDirs);
		}
		else
		{
			s = FixSlash(DirInfo_Dir(g_pDirInfo, i));
			utf8_s = utf8_from_tstring(s);
			g_directoryInfo[i].pfnSetTheseDirs(utf8_s);
			osd_free(utf8_s);
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
	TCHAR	buf[MAX_PATH];
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
				_tcscpy(DirInfo_Path(g_pDirInfo, nType, i),
					   DirInfo_Path(g_pDirInfo, nType, i - 1));

			_tcscpy(DirInfo_Path(g_pDirInfo, nType, nItem), buf);
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
	TCHAR	inbuf[MAX_PATH];
	TCHAR	outbuf[MAX_PATH];
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
			_tcscpy(DirInfo_Path(g_pDirInfo, nType, i),
				   DirInfo_Path(g_pDirInfo, nType, i + 1));

		_tcscpy(DirInfo_Path(g_pDirInfo, nType, DirInfo_NumDir(g_pDirInfo, nType) - 1), TEXT(""));
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
			Edit_SetText(hEdit, TEXT(""));
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
		struct _stat file_stat;

		/* Don't allow empty entries. */
		if (!_tcscmp(pItem->pszText, TEXT("")))
		{
			return FALSE;
		}

		/* Check validity of edited directory. */
		if (_tstat(pItem->pszText, &file_stat) == 0
		&&	(file_stat.st_mode & S_IFDIR))
		{
			bResult = TRUE;
		}
		else
		{
			if (MessageBox(NULL, TEXT("Directory does not exist, continue anyway?"), TEXT(MAMEUINAME), MB_OKCANCEL) == IDOK)
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
					_tcscpy(DirInfo_Path(g_pDirInfo, nType, i),
						   DirInfo_Path(g_pDirInfo, nType, i - 1));

				_tcscpy(DirInfo_Path(g_pDirInfo, nType, pItem->iItem), pItem->pszText);

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

BOOL BrowseForDirectory(HWND hwnd, LPCTSTR pStartDir, TCHAR* pResult) 
{
	BOOL		bResult = FALSE;
	IMalloc*	piMalloc = 0;
	BROWSEINFO	Info;
	LPITEMIDLIST pItemIDList = NULL;
	TCHAR		buf[MAX_PATH];
	
	if (!SUCCEEDED(SHGetMalloc(&piMalloc)))
		return FALSE;

	Info.hwndOwner		= hwnd;
	Info.pidlRoot		= NULL;
	Info.pszDisplayName = buf;
	Info.lpszTitle		= TEXT("Directory name:");
	Info.ulFlags		= BIF_RETURNONLYFSDIRS;
	Info.lpfn			= BrowseCallbackProc;
	Info.lParam 		= (LPARAM)pStartDir;

	pItemIDList = SHBrowseForFolder(&Info);

	if (pItemIDList != NULL)
	{
		if (SHGetPathFromIDList(pItemIDList, buf) == TRUE)
		{
			_sntprintf(pResult, MAX_PATH, TEXT("%s"), buf);
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

