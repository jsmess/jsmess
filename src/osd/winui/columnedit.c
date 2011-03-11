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

  columnedit.c

  Column Edit dialog

***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>

// MAME/MAMEUI headers
#include <stdlib.h>
#include "resource.h"
#include "mui_opts.h"
#include "winui.h"

// Returns TRUE if successful
static int DoExchangeItem(HWND hFrom, HWND hTo, int nMinItem)
{
	LV_ITEM lvi;
	TCHAR	buf[80];
	//int 	nFrom, nTo;
	BOOL 	b_res;

	//nFrom = ListView_GetItemCount(hFrom);
	//nTo   = ListView_GetItemCount(hTo);

	lvi.iItem = ListView_GetNextItem(hFrom, -1, LVIS_SELECTED | LVIS_FOCUSED);
	if (lvi.iItem < nMinItem)
	{
		if (lvi.iItem != -1) // Can't remove the first column
			MessageBox(0, TEXT("Cannot Move Selected Item"), TEXT("Move Item"), IDOK);
		SetFocus(hFrom);
		return FALSE;
	}
	lvi.iSubItem   = 0;
	lvi.mask	   = LVIF_PARAM | LVIF_TEXT;
	lvi.pszText    = buf;
	lvi.cchTextMax = sizeof(buf);
	if (ListView_GetItem(hFrom, &lvi))
	{
		// Add this item to the Show and delete it from Available
		b_res = ListView_DeleteItem(hFrom, lvi.iItem);
		lvi.iItem = ListView_GetItemCount(hTo);
		(void)ListView_InsertItem(hTo, &lvi);
		ListView_SetItemState(hTo, lvi.iItem,
							  LVIS_FOCUSED | LVIS_SELECTED,
							  LVIS_FOCUSED | LVIS_SELECTED);
		SetFocus(hTo);
		return lvi.iItem;
	}
	return FALSE;
}

static void DoMoveItem( HWND hWnd, BOOL bDown)
{
	LV_ITEM lvi;
	TCHAR	buf[80];
	int 	nMaxpos;
	BOOL 	b_res;
	
	lvi.iItem = ListView_GetNextItem(hWnd, -1, LVIS_SELECTED | LVIS_FOCUSED);
	nMaxpos = ListView_GetItemCount(hWnd);
	if (lvi.iItem == -1 || 
		(lvi.iItem <  2 && bDown == FALSE) || // Disallow moving First column
		(lvi.iItem == 0 && bDown == TRUE)  || // ""
		(lvi.iItem == nMaxpos - 1 && bDown == TRUE))
	{
		SetFocus(hWnd);
		return;
	}
	lvi.iSubItem   = 0;
	lvi.mask	   = LVIF_PARAM | LVIF_TEXT;
	lvi.pszText    = buf;
	lvi.cchTextMax = sizeof(buf);
	if (ListView_GetItem(hWnd, &lvi))
	{
		// Add this item to the Show and delete it from Available
		b_res = ListView_DeleteItem(hWnd, lvi.iItem);
		lvi.iItem += (bDown) ? 1 : -1;
		(void)ListView_InsertItem(hWnd,&lvi);
		ListView_SetItemState(hWnd, lvi.iItem,
							  LVIS_FOCUSED | LVIS_SELECTED,
							  LVIS_FOCUSED | LVIS_SELECTED);
		if (lvi.iItem == nMaxpos - 1)
			EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEDOWN), FALSE);
		else
			EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEDOWN), TRUE);

		if (lvi.iItem < 2)
			EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEUP),   FALSE);
		else
			EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEUP),   TRUE);

		SetFocus(hWnd);
	}
}

INT_PTR InternalColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam,
	int nColumnMax, int *shown, int *order,
	const LPCTSTR *names, void (*pfnGetRealColumnOrder)(int *),
	void (*pfnGetColumnInfo)(int *pnOrder, int *pnShown),
	void (*pfnSetColumnInfo)(int *pnOrder, int *pnShown))
{
	static HWND hShown;
	static HWND hAvailable;
	static BOOL showMsg = FALSE;
	int         nShown;
	int         nAvail;
	int         i, nCount = 0;
	LV_ITEM     lvi;
	DWORD		dwShowStyle, dwAvailableStyle, dwView;
	BOOL		b_res;

	switch (Msg)
	{
	case WM_INITDIALOG:
		hShown	   = GetDlgItem(hDlg, IDC_LISTSHOWCOLUMNS);
		hAvailable = GetDlgItem(hDlg, IDC_LISTAVAILABLECOLUMNS);
		/*Change Style to Always Show Selection */
		dwShowStyle = GetWindowLong(hShown, GWL_STYLE); 
		dwAvailableStyle = GetWindowLong(hAvailable, GWL_STYLE); 
		dwView = LVS_SHOWSELALWAYS | LVS_LIST;

		/* Only set the window style if the view bits have changed. */
		if ((dwShowStyle & LVS_TYPEMASK) != dwView) 
		SetWindowLong(hShown, GWL_STYLE, 
			(dwShowStyle & ~LVS_TYPEMASK) | dwView); 
		if ((dwAvailableStyle & LVS_TYPEMASK) != dwView) 
		SetWindowLong(hAvailable, GWL_STYLE, 
			(dwAvailableStyle & ~LVS_TYPEMASK) | dwView); 

		pfnGetColumnInfo(order, shown);

		showMsg = TRUE;
		nShown	= 0;
		nAvail	= 0;
						
		lvi.mask	  = LVIF_TEXT | LVIF_PARAM; 
		lvi.stateMask = 0;				   
		lvi.iSubItem  = 0; 
		lvi.iImage	  = -1;

		/* Get the Column Order and save it */
		pfnGetRealColumnOrder(order);

		for (i = 0 ; i < nColumnMax; i++)
		{		 
			lvi.pszText = (TCHAR *) names[order[i]];
			lvi.lParam	= order[i];

			if (shown[order[i]])
			{
				lvi.iItem = nShown;
				(void)ListView_InsertItem(hShown, &lvi);
				nShown++;
			}
			else
			{
				lvi.iItem = nAvail;
				(void)ListView_InsertItem(hAvailable, &lvi);
				nAvail++;
			}
		}
		if( nShown > 0)
		{
			/*Set to Second, because first is not allowed*/
			ListView_SetItemState(hShown, 1,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
		if( nAvail > 0)
		{
			ListView_SetItemState(hAvailable, 0,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD)  ,    TRUE);
		return TRUE;

	case WM_NOTIFY:
		{
			NMHDR *nm = (NMHDR *)lParam;
			NM_LISTVIEW *pnmv;
			int 		nPos;
			
			switch (nm->code)
			{
			case NM_DBLCLK:
				// Do Data Exchange here, which ListView was double clicked?
				switch (nm->idFrom)
				{
				case IDC_LISTAVAILABLECOLUMNS:
					// Move selected Item from Available to Shown column
					nPos = DoExchangeItem(hAvailable, hShown, 0);
					if (nPos)
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),	   FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
					}
					break;

				case IDC_LISTSHOWCOLUMNS:
					// Move selected Item from Show to Available column
					if (DoExchangeItem(hShown, hAvailable, 1))
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD)  ,    TRUE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
					}
					break;
				}
				return TRUE;
				
			case LVN_ITEMCHANGED:
				// Don't handle this message for now
				pnmv = (NM_LISTVIEW *)nm;
				if (//!(pnmv->uOldState & LVIS_SELECTED) &&
					(pnmv->uNewState  & LVIS_SELECTED))
				{
					if (pnmv->iItem == 0 && pnmv->hdr.idFrom == IDC_LISTSHOWCOLUMNS)
					{
						// Don't allow selecting the first item
						ListView_SetItemState(hShown, pnmv->iItem,
							LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
						if (showMsg)
						{
							MessageBox(0, TEXT("Changing this item is not permitted"), TEXT("Select Item"), IDOK);
							showMsg = FALSE;
						}
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
						/*Leave Focus on Control*/
						//SetFocus(GetDlgItem(hDlg,IDOK));
						return TRUE;
					}
					else
						showMsg = TRUE;
				}
				if( pnmv->uOldState & LVIS_SELECTED && pnmv->iItem == 0 && pnmv->hdr.idFrom == IDC_LISTSHOWCOLUMNS )
				{
					/*we enable the buttons again, if the first Entry loses selection*/
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), TRUE);
					//SetFocus(GetDlgItem(hDlg,IDOK));
				}
				break;
			case NM_SETFOCUS:
				{
					switch (nm->idFrom)
					{
					case IDC_LISTAVAILABLECOLUMNS:
						if (ListView_GetItemCount(nm->hwndFrom) != 0)
						{
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),	   TRUE);
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
						}
						break;
					case IDC_LISTSHOWCOLUMNS:
						if (ListView_GetItemCount(nm->hwndFrom) != 0)
						{
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),	   FALSE);
							
							if (ListView_GetNextItem(hShown, -1, LVIS_SELECTED | LVIS_FOCUSED) == 0 )
							{
								EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
								EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
								EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
							}
							else
							{
								EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
								EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), TRUE);
								EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
							}
						}
						break;
					}
				}
				break;
			case LVN_KEYDOWN:
			case NM_CLICK:
				pnmv = (NM_LISTVIEW *)nm;
				if (//!(pnmv->uOldState & LVIS_SELECTED) &&
					(pnmv->uNewState  & LVIS_SELECTED))
				{
					if (pnmv->iItem == 0 && pnmv->hdr.idFrom == IDC_LISTSHOWCOLUMNS)
					{
					}
				}
				switch (nm->idFrom)
				{
				case IDC_LISTAVAILABLECOLUMNS:
					if (ListView_GetItemCount(nm->hwndFrom) != 0)
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),	   TRUE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
					}
					break;

				case IDC_LISTSHOWCOLUMNS:
					if (ListView_GetItemCount(nm->hwndFrom) != 0)
					{
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),	   FALSE);
						if (ListView_GetNextItem(hShown, -1, LVIS_SELECTED | LVIS_FOCUSED) == 0 )
						{
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
						}
						else
						{
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), TRUE);
							EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
						}
					}
					break;
				}
				//SetFocus( nm->hwndFrom );
				return TRUE;
			}
		}
		return FALSE;
	case WM_COMMAND:
	   {
			WORD wID	  = GET_WM_COMMAND_ID(wParam, lParam);
			HWND hWndCtrl = GET_WM_COMMAND_HWND(wParam, lParam);
			int  nPos = 0;

			switch (wID)
			{
				case IDC_LISTSHOWCOLUMNS:
					break;
				case IDC_BUTTONADD:
					// Move selected Item in Available to Shown
					nPos = DoExchangeItem(hAvailable, hShown, 0);
					if (nPos)
					{
						EnableWindow(hWndCtrl,FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
					}
					break;

				case IDC_BUTTONREMOVE:
					// Move selected Item in Show to Available
					if (DoExchangeItem( hShown, hAvailable, 1))
					{
						EnableWindow(hWndCtrl,FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),	   TRUE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
					}
					break;

				case IDC_BUTTONMOVEDOWN:
					// Move selected item in the Show window up 1 item
					DoMoveItem(hShown, TRUE);
					break;

				case IDC_BUTTONMOVEUP:
					// Move selected item in the Show window down 1 item
					DoMoveItem(hShown, FALSE);
					break;

				case IDOK:
					// Save users choices
					nShown = ListView_GetItemCount(hShown);
					nAvail = ListView_GetItemCount(hAvailable);
					nCount = 0;
					for (i = 0; i < nShown; i++)
					{
						lvi.iSubItem = 0;
						lvi.mask     = LVIF_PARAM;
						lvi.pszText  = 0;
						lvi.iItem    = i;
						b_res = ListView_GetItem(hShown, &lvi);
						order[nCount++]   = lvi.lParam;
						shown[lvi.lParam] = TRUE;
					}
					for (i = 0; i < nAvail; i++)
					{
						lvi.iSubItem = 0;
						lvi.mask     = LVIF_PARAM;
						lvi.pszText  = 0;
						lvi.iItem    = i;
						b_res = ListView_GetItem(hAvailable, &lvi);
						order[nCount++]   = lvi.lParam;
						shown[lvi.lParam] = FALSE;
					}
					pfnSetColumnInfo(order, shown);
					EndDialog(hDlg, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					return TRUE;
			}
		}
		break;
	}
	return 0;
}

static void GetColumnInfo(int *order, int *shown)
{
	GetColumnOrder(order);
	GetColumnShown(shown);
}

static void SetColumnInfo(int *order, int *shown)
{
	SetColumnOrder(order);
	SetColumnShown(shown);
}

INT_PTR CALLBACK ColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static int shown[COLUMN_MAX];
	static int order[COLUMN_MAX];
	extern const LPCTSTR column_names[COLUMN_MAX]; // from winui.c, should improve

	return InternalColumnDialogProc(hDlg, Msg, wParam, lParam, COLUMN_MAX,
		shown, order, column_names, GetRealColumnOrder, GetColumnInfo, SetColumnInfo);
}



