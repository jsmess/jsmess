//============================================================
//
//	configms.c - Win32 MESS specific options
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
//	LOCAL VARIABLES
//============================================================

static const options_entry win_mess_opts[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"SDL MESS SPECIFIC OPTIONS" },
//	{ "newui;nu",                   "1",    OPTION_BOOLEAN,		"use the new MESS UI" },
	{ "threads;thr",				"0",	0,					"number of threads to use for parallel operations" },
	{ "natural;nat",				"0",	OPTION_BOOLEAN,		"specifies whether to use a natural keyboard or not" },
	{ NULL }
};

int win_use_natural_keyboard;

//============================================================

void osd_mess_options_init(void)
{
	options_add_entries(win_mess_opts);
}


void sdl_mess_options_parse(void)
{
	win_task_count = options_get_int("threads");
	win_use_natural_keyboard = options_get_bool("natural");
}


