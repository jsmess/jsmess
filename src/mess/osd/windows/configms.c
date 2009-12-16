//============================================================
//
//  configms.c - Win32 MESS specific options
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>

// MESS headers
#include "driver.h"
#include "winmain.h"
#include "menu.h"
#include "device.h"
#include "configms.h"
#include "pool.h"
#include "config.h"
#include "winutf8.h"


//============================================================
//  GLOBAL VARIABLES
//============================================================

const options_entry mess_win_options[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"WINDOWS MESS SPECIFIC OPTIONS" },
	{ "newui;nu",                   "0",    OPTION_BOOLEAN,		"use the new MESS UI" },
	{ NULL }
};

//============================================================

void osd_mess_options_init(core_options *opts)
{
	extern const options_entry mess_win_options[];
	options_add_entries(opts, mess_win_options);
}



void osd_get_emulator_directory(char *dir, size_t dir_size)
{
	char *s;
	win_get_module_file_name_utf8(NULL, dir, dir_size);
	s = strrchr(dir, '\\');
	if (s)
		s[1] = '\0';
}
