//============================================================
//
//	sdlutil.c - SDLMESS utilities that can't be in COREOBJS
//
//============================================================

// MESS headers
#include "driver.h"
#include "windows/config.h"
#include "parallel.h"
#include "menu.h"
#include "device.h"
#include "configms.h"
#include "mscommon.h"
#include "pool.h"
#include "config.h"

//============================================================
//	GLOBAL VARIABLES
//============================================================

extern int win_use_natural_keyboard;

//============================================================
//	LOCAL VARIABLES
//============================================================

//============================================================
//	osd_keyboard_disabled
//============================================================

int osd_keyboard_disabled(void)
{
	return win_use_natural_keyboard;
}

