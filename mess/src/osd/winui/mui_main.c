/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
#ifndef WINUI
#define WINUI
#endif

// import the main() from MAME, but rename it so we can call it indirectly
#undef main
#undef wmain
#define main mame_main
#define wmain mame_main
#include "windows/main.c"
#undef main
#undef wmain

#include "winui.h"


int WINAPI wWinMain(HINSTANCE    hInstance,
                   HINSTANCE    hPrevInstance,
                   LPWSTR       lpCmdLine,
                   int          nCmdShow)
{
	return MameUIMain(hInstance, lpCmdLine, nCmdShow);
}
