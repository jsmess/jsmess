/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/
 
/***************************************************************************

  history.c

    history functions.

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include <driver.h>
#include "m32util.h"

#include "datafile.h"
#include "history.h"


/**************************************************************
 * functions
 **************************************************************/

// Load indexes from history.dat if found
char * GetGameHistory(int driver_index)
{
	static char buffer[32768];
	buffer[0] = '\0';

	if (load_driver_history(drivers[driver_index],buffer,sizeof(buffer)) != 0)
		return buffer;

	return ConvertToWindowsNewlines(buffer);
}
