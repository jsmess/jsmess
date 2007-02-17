/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Splitter code Copyright (C) 1997-98 Mike Haaland

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  Splitters.c

  Splitter GUI code.

  Created 12/03/98 (C) by Mike Haaland (mhaaland@hypertech.com)

***************************************************************************/

/*

  Tree, spliiter, list, splitter, pict

*/
  

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "splitters.h"
#include "bitmask.h"
#include "options.h"
#include "resource.h"
#include "screenshot.h"
#include "win32ui.h"

/* Local Variables */
static BOOL         bTracking = 0;
static int          numSplitters = 0;
static int          currentSplitter = 0;
static HZSPLITTER   *splitter;
static LPHZSPLITTER lpCurSpltr = 0;
static HCURSOR      hSplitterCursor = 0;

int *nSplitterOffset;

BOOL InitSplitters(void)
{
	int nSplitterCount;

	/* load the cursor for the splitter */
	hSplitterCursor = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_CURSOR_HSPLIT));

	nSplitterCount = GetSplitterCount();

	splitter = malloc(sizeof(HZSPLITTER) * nSplitterCount);
	if (!splitter)
		goto error;		
	memset(splitter, 0, sizeof(HZSPLITTER) * nSplitterCount);

	nSplitterOffset = malloc(sizeof(int) * nSplitterCount);
	if (!nSplitterOffset)
		goto error;		
	memset(nSplitterOffset, 0, sizeof(int) * nSplitterCount);	

	return TRUE;

error:
	SplittersExit();
	return FALSE;
}

void SplittersExit(void)
{
	if (splitter)
	{
		free(splitter);
		splitter = NULL;
	}
	if (nSplitterOffset)
	{
		free(nSplitterOffset);
		nSplitterOffset = NULL;
	}
}


/* Called with hWnd = Parent Window */
void CalcSplitter(HWND hWnd, LPHZSPLITTER lpSplitter)
{
	POINT	p = {0,0};
	RECT	leftRect, rightRect;
	int 	dragWidth;

	ClientToScreen(hWnd, &p);

	GetWindowRect(lpSplitter->m_hWnd,		&lpSplitter->m_dragRect);
	GetWindowRect(lpSplitter->m_hWndLeft,	&leftRect);
	GetWindowRect(lpSplitter->m_hWndRight,	&rightRect);

	OffsetRect(&lpSplitter->m_dragRect, -p.x, -p.y);
	OffsetRect(&leftRect, -p.x, -p.y);
	OffsetRect(&rightRect, -p.x, -p.y);

	dragWidth = lpSplitter->m_dragRect.right - lpSplitter->m_dragRect.left;

	lpSplitter->m_limitRect.left	= leftRect.left + 20;
	lpSplitter->m_limitRect.right	= rightRect.right - 20;
	lpSplitter->m_limitRect.top 	= lpSplitter->m_dragRect.top;
	lpSplitter->m_limitRect.bottom	= lpSplitter->m_dragRect.bottom;
	if (lpSplitter->m_func)
		lpSplitter->m_func(hWnd, &lpSplitter->m_limitRect);

	if (lpSplitter->m_dragRect.left < lpSplitter->m_limitRect.left)
		lpSplitter->m_dragRect.left = lpSplitter->m_limitRect.left;

	if (lpSplitter->m_dragRect.right > lpSplitter->m_limitRect.right)
		lpSplitter->m_dragRect.left = lpSplitter->m_limitRect.right - dragWidth;

	lpSplitter->m_dragRect.right = lpSplitter->m_dragRect.left + dragWidth;
}

void AdjustSplitter2Rect(HWND hWnd, LPRECT lpRect)
{
	RECT pRect;

	GetClientRect(hWnd, &pRect);

	if (lpRect->right > pRect.right)
		lpRect->right = pRect.right;

	lpRect->right = MIN(lpRect->right - GetMinimumScreenShotWindowWidth(), lpRect->right);

	lpRect->left = MAX((pRect.right - (pRect.right - pRect.left)) / 2, lpRect->left);
}

void AdjustSplitter1Rect(HWND hWnd, LPRECT lpRect)
{
}

void RecalcSplitters(void)
{
	int 	i;
	
	for (i = 0; i < numSplitters; i++)
	{
		CalcSplitter(GetParent(splitter[i].m_hWnd), &splitter[i]);
		nSplitterOffset[i] = splitter[i].m_dragRect.left;
	}
	
	for (i = numSplitters - 1; i >= 0; i--)
	{
		CalcSplitter(GetParent(splitter[i].m_hWnd), &splitter[i]);
		nSplitterOffset[i] = splitter[i].m_dragRect.left;
	}

}

void OnSizeSplitter(HWND hWnd)
{
	static int firstTime = TRUE;
	int changed = FALSE;
	RECT rWindowRect;
	POINT p = {0,0};
	int i;
	int nSplitterCount;
	BOOL bMustChange;

	nSplitterCount = GetSplitterCount();

	if (firstTime)
	{
		for (i = 0; i < nSplitterCount; i++)
			nSplitterOffset[i] = GetSplitterPos(i);
		changed = TRUE;
		firstTime = FALSE;
	}

	GetWindowRect(hWnd, &rWindowRect);

	for (i = 0; i < nSplitterCount; i++)
	{
		p.x = 0;
		p.y = 0;
		ClientToScreen(splitter[i].m_hWnd, &p);

		/* We must change if our window is not in the window rect */
		bMustChange = !PtInRect(&rWindowRect, p);

		/* We should also change if we are ahead the next splitter */
		if ((i < nSplitterCount-1) && (nSplitterOffset[i] >= nSplitterOffset[i+1]))
			bMustChange = TRUE;

		/* ...or if we are behind the previous splitter */
		if ((i > 0) && (nSplitterOffset[i] <= nSplitterOffset[i-1]))
			bMustChange = TRUE;

		if (bMustChange)
		{
			nSplitterOffset[i] = (rWindowRect.right - rWindowRect.left) * g_splitterInfo[i].dPosition;
			changed = TRUE;
		}
	}

	if (changed)
	{
		ResizePickerControls(hWnd);
		RecalcSplitters();
		//UpdateScreenShot();
	}
}

void AddSplitter(HWND hWnd, HWND hWndLeft, HWND hWndRight, void (*func)(HWND hWnd,LPRECT lpRect))
{
	LPHZSPLITTER lpSpltr = &splitter[numSplitters];

	if (numSplitters >= GetSplitterCount())
		return;

	lpSpltr->m_hWnd = hWnd;
	lpSpltr->m_hWndLeft = hWndLeft;
	lpSpltr->m_hWndRight = hWndRight;
	lpSpltr->m_func = func;
	CalcSplitter(GetParent(hWnd), lpSpltr);

	numSplitters++;
}

void OnInvertTracker(HWND hWnd, const RECT *rect)
{
	HDC 	hDC = GetDC(hWnd);
	HBRUSH	hBrush = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
	HBRUSH	hOldBrush = 0;

	if (hBrush != 0)
		hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);
	PatBlt(hDC, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, DSTINVERT);
	if (hOldBrush != 0)
		SelectObject(hDC, hOldBrush);
	ReleaseDC(hWnd, hDC);
	DeleteObject(hBrush);
}

void StartTracking(HWND hWnd, UINT hitArea)
{
	if (!bTracking && lpCurSpltr != 0 && hitArea == SPLITTER_HITITEM)
	{
		// Ensure we have and updated cursor structure
		CalcSplitter(hWnd, lpCurSpltr);
		// Draw the first splitter shadow
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);
		// Capture the mouse
		SetCapture(hWnd);
		// Set tracking to TRUE
		bTracking = TRUE;
		SetCursor(hSplitterCursor);
	}
}

void StopTracking(HWND hWnd)
{
	if (bTracking)
	{
		// erase the tracking image
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);
		// Release the mouse
		ReleaseCapture();
		// set tracking to false
		bTracking = FALSE;
		SetCursor(LoadCursor(0, IDC_ARROW));
		// set the new splitter position
		nSplitterOffset[currentSplitter] = lpCurSpltr->m_dragRect.left;
		// Redraw the screen area
		ResizePickerControls(hWnd);
		UpdateScreenShot();
		InvalidateRect(GetMainWindow(),NULL,TRUE);
	}
}

UINT SplitterHitTest(HWND hWnd, POINTS p)
{
	RECT  rect;
	POINT pt;
	int   i;

	pt.x = p.x;
	pt.y = p.y;

	// Check which area we hit
	ClientToScreen(hWnd, &pt);

	for (i = 0; i < numSplitters; i++)
	{
		GetWindowRect(splitter[i].m_hWnd, &rect);
		if (PtInRect(&rect, pt))
		{
			lpCurSpltr = &splitter[i];
			currentSplitter = i;
			// We hit the splitter
			return SPLITTER_HITITEM;
		}
	}
	lpCurSpltr = 0;
	// We missed the splitter
	return SPLITTER_HITNOTHING;
}

void OnMouseMove(HWND hWnd, UINT nFlags, POINTS p)
{
	if (bTracking)	// move the tracking image
	{
		int 	nWidth;
		RECT	rect;
		POINT	pt;

		pt.x = (int)p.x;
		pt.y = (int)p.y;

		ClientToScreen(hWnd, &pt);
		GetWindowRect(hWnd, &rect);
		if (! PtInRect(&rect, pt))
		{
			if ((short)pt.x < (short)rect.left)
				pt.x = rect.left;
			if ((short)pt.x > (short)rect.right)
				pt.x = rect.right;
			pt.y = rect.top + 1;
		}

		ScreenToClient(hWnd, &pt);

		// Erase the old tracking image
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);
		
		// calc the new one based on p.x draw it
		nWidth = lpCurSpltr->m_dragRect.right - lpCurSpltr->m_dragRect.left;
		lpCurSpltr->m_dragRect.right = pt.x + nWidth / 2;
		lpCurSpltr->m_dragRect.left  = pt.x - nWidth / 2;

		if (pt.x - nWidth / 2 > lpCurSpltr->m_limitRect.right)
		{
			lpCurSpltr->m_dragRect.right = lpCurSpltr->m_limitRect.right;
			lpCurSpltr->m_dragRect.left  = lpCurSpltr->m_dragRect.right - nWidth;
		}
		if (pt.x + nWidth / 2 < lpCurSpltr->m_limitRect.left)
		{
			lpCurSpltr->m_dragRect.left  = lpCurSpltr->m_limitRect.left;
			lpCurSpltr->m_dragRect.right = lpCurSpltr->m_dragRect.left + nWidth;
		}
		OnInvertTracker(hWnd, &lpCurSpltr->m_dragRect);
	}
	else
	{
		switch(SplitterHitTest(hWnd, p))
		{
		case SPLITTER_HITNOTHING:
			SetCursor(LoadCursor(0, IDC_ARROW));
			break;
		case SPLITTER_HITITEM:
			SetCursor(hSplitterCursor);
			break;
		}
	}
}

void OnLButtonDown(HWND hWnd, UINT nFlags, POINTS p)
{
	if (!bTracking) // See if we need to start a splitter drag
	{
		StartTracking(hWnd, SplitterHitTest(hWnd, p));
	}
}

void OnLButtonUp(HWND hWnd, UINT nFlags, POINTS p)
{
	if (bTracking)
	{
		StopTracking(hWnd);
	}
}

int GetSplitterCount(void)
{
	int nSplitterCount = 0;
	while(g_splitterInfo[nSplitterCount].dPosition > 0)
		nSplitterCount++;
	return nSplitterCount;
}

/* End of file */
