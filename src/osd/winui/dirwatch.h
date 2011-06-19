/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef DIRWATCHER_H
#define DIRWATCHER_H

typedef struct DirWatcher *PDIRWATCHER;

PDIRWATCHER DirWatcher_Init(HWND hwndTarget, UINT nMessage);
BOOL DirWatcher_Watch(PDIRWATCHER pWatcher, WORD nIndex, LPCSTR pszPathList, BOOL bWatchSubtrees);
void DirWatcher_Free(PDIRWATCHER pWatcher);

#endif
