//============================================================
//
//  configms.c - Win32 MESS specific options
//
//============================================================

// MESS headers
#include "driver.h"
//#include "windows/config.h"
#include "menu.h"
#include "device.h"
#include "configms.h"
#include "pool.h"
#include "config.h"

//============================================================
//  LOCAL VARIABLES
//============================================================

static const options_entry win_mess_opts[] =
{
	{ NULL,							NULL,   OPTION_HEADER,		"SDL MESS SPECIFIC OPTIONS" },
//  { "newui;nu",                   "1",    OPTION_BOOLEAN,     "use the new MESS UI" },
	#if defined(__APPLE__) && defined(__MACH__)
	{ "uimodekey;umk",        "ITEM_ID_INSERT", 0,    "specifies the key used to toggle between full and partial UI mode" },
	#else
	{ "uimodekey;umk",        "ITEM_ID_SCRLOCK", 0,    "specifies the key used to toggle between full and partial UI mode" },
	#endif
	{ NULL }
};

extern char *osd_get_startup_cwd(void);

//============================================================

void osd_mess_options_init(core_options *opts)
{
	options_add_entries(opts, win_mess_opts);
}

void osd_get_emulator_directory(char *dir, size_t dir_size)
{
	strncpy(dir, osd_get_startup_cwd(), dir_size);
}

