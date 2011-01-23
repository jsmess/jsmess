/***************************************************************************

    mess.c

    This file is a set of function calls and defs required for MESS

***************************************************************************/

#include "emu.h"

/* Globals */
const char mess_disclaimer[] =
		"MESS is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		"several computer and console systems. But hardware is useless without software\n"
		"so a file dump of the ROM, cartridges, discs, and cassettes which run on that\n"
		"hardware is required. Such files, like any other commercial software, are\n"
		"copyrighted material and it is therefore illegal to use them if you don't own\n"
		"the original media from which the files are derived. Needless to say, these\n"
		"files are not distributed together with MESS. Distribution of MESS together\n"
		"with these files is a violation of copyright law and should be promptly\n"
		"reported to the authors so that appropriate legal action can be taken.\n\n";

/*-------------------------------------------------
    mess_display_help - display MESS help to
    standard output
-------------------------------------------------*/

void mess_display_help(void)
{
	mame_printf_info(
		"MESS v%s\n"
		"Multi Emulator Super System - Copyright (C) 1997-2010 by the MESS Team\n"
		"MESS is based on MAME Source code\n"
		"Copyright (C) 1997-2010 by Nicola Salmoria and the MAME Team\n\n",
		build_version);
	mame_printf_info("%s\n", mess_disclaimer);
	mame_printf_info(
		"Usage:  MESS <system> <media> <software> <options>\n"
		"\n"
		"        MESS -showusage    for a brief list of options\n"
		"        MESS -showconfig   for a list of configuration options\n"
		"        MESS -listmedia    for a full list of supported media\n"
		"        MESS -createconfig to create a mess.ini\n"
		"\n"
		"See config.txt and windows.txt for usage instructions.\n");
}
