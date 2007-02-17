/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Splitter Code (C) 1998 Mike Haaland <mhaaland@hypertech.com>

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/* Written by Mike Haaland <mhaaland@hypertech.com> */

#ifndef SPLITTER_H
#define SPLITTER_H

#if !defined(MAX)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

enum eSplitterHits
{
	SPLITTER_HITNOTHING = 0,
	SPLITTER_HITITEM
};

typedef struct horzSplitter
{
	HWND m_hWnd;
	HWND m_hWndLeft;
	HWND m_hWndRight;
	RECT m_limitRect;
	RECT m_dragRect;
	void (*m_func)(HWND hWnd, LPRECT lpRect);
} HZSPLITTER, *LPHZSPLITTER;

/* Splitter routines */
void    OnMouseMove(HWND hWnd, UINT nFlags, POINTS p);
void    OnLButtonDown(HWND hWnd, UINT nFlags, POINTS p);
void    OnLButtonUp(HWND hWnd, UINT nFlags, POINTS p);
void    OnSizeSplitter(HWND hWnd);
void    AddSplitter(HWND hWnd, HWND hWndLeft, HWND hWndRight,
                    void (*func)(HWND hWnd,LPRECT lpRect));
void    RecalcSplitters(void);
void    AdjustSplitter2Rect(HWND hWnd, LPRECT lpRect);
void    AdjustSplitter1Rect(HWND hWnd, LPRECT lpRect);
BOOL    InitSplitters(void);
void    SplittersExit(void);
int     GetSplitterCount(void);

extern int *nSplitterOffset;

typedef struct 
{
	double dPosition;
	int nSplitterWindow;
	int nLeftWindow;
	int nRightWindow;
	void (*pfnAdjust)(HWND hWnd,LPRECT lpRect);
} SPLITTERINFO;

extern const SPLITTERINFO g_splitterInfo[];


#endif /* SPLITTER_H */
