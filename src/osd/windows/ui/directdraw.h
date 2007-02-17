/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef DIRECTDRAW_H
#define DIRECTDRAW_H

#include <ddraw.h>

#define MAXMODES    256 /* Maximum number of DirectDraw Display modes. */

/* Display mode node */
struct tDisplayMode
{
	DWORD m_dwWidth;
	DWORD m_dwHeight;
	DWORD m_dwBPP;
	DWORD m_dwRefresh;
};

/* EnumDisplayMode Context */
struct tDisplayModes
{
	struct tDisplayMode m_Modes[MAXMODES];
	int                 m_nNumModes;
};

extern BOOL DirectDraw_Initialize(void);
extern void DirectDraw_Close(void);

extern struct tDisplayModes* DirectDraw_GetDisplayModes(void);
extern int            DirectDraw_GetNumDisplays(void);
extern BOOL           DirectDraw_HasHWStretch(void);
extern BOOL           DirectDraw_HasRefresh(void);
extern const char*    DirectDraw_GetDisplayName(int num_display);

#endif
