/***************************************************************************

    climess.c

    Command-line interface frontend for MESS.

***************************************************************************/

#include "emu.h"
#include "climess.h"
#include "output.h"
#include "hash.h"


/*-------------------------------------------------
    mess_display_help - display MESS help to
    standard output
-------------------------------------------------*/

void mess_display_help(void)
{
	mame_printf_info(
		"MESS v%s\n"
		"Multi Emulator Super System - Copyright (C) 1997-2009 by the MESS Team\n"
		"MESS is based on MAME Source code\n"
		"Copyright (C) 1997-2009 by Nicola Salmoria and the MAME Team\n\n",
		build_version);
	mame_printf_info("%s\n", mess_disclaimer);
	mame_printf_info(
		"Usage:  MESS <system> <device> <software> <options>\n"
		"\n"
		"        MESS -showusage    for a brief list of options\n"
		"        MESS -showconfig   for a list of configuration options\n"
		"        MESS -listdevices  for a full list of supported devices\n"
		"        MESS -createconfig to create a mess.ini\n"
		"\n"
		"See config.txt and windows.txt for usage instructions.\n");
}

