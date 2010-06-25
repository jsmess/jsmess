/***************************************************************************

    messopts.c - MESS specific option handling

****************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "options.h"

/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

const options_entry mess_core_options[] =
{
	{ NULL,							NULL,   OPTION_HEADER,						"MESS SPECIFIC OPTIONS" },
	{ "ramsize;ram",				NULL,	0,									"size of RAM (if supported by driver)" },
	{ "newui;nu",                   "0",    OPTION_BOOLEAN,						"use the new MESS UI" },
	{ NULL }
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/
/*-------------------------------------------------
    mess_options_init - called from core to add
    MESS specific options
-------------------------------------------------*/

void mess_options_init(core_options *opts)
{
	/* add MESS-specific options */
	options_add_entries(opts, mess_core_options);
}

