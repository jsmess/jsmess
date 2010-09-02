/***************************************************************************

    mess.c

    This file is a set of function calls and defs required for MESS

***************************************************************************/

#include "emu.h"
#include "emuopts.h"

#include "lcd.lh"
#include "lcd_rot.lh"

#include "devices/messram.h"

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
		"Multi Emulator Super System - Copyright (C) 1997-2009 by the MESS Team\n"
		"MESS is based on MAME Source code\n"
		"Copyright (C) 1997-2010 by Nicola Salmoria and the MAME Team\n\n",
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

/*************************************
 *
 *  Code used by print_mame_xml()
 *
 *************************************/

/* device iteration helpers */
#define ram_first(config)				(config)->m_devicelist.first(MESSRAM)
#define ram_next(previous)				((previous)->typenext())

/*-------------------------------------------------
    print_game_ramoptions - prints out all RAM
    options for this system
-------------------------------------------------*/
static void print_game_ramoptions(FILE *out, const game_driver *game, const machine_config *config)
{
	const device_config *device;

	for (device = ram_first(config); device != NULL; device = ram_next(device))
	{
		ram_config *ram = (ram_config *)downcast<const legacy_device_config_base *>(device)->inline_config();
		fprintf(out, "\t\t<ramoption default=\"1\">%u</ramoption>\n",  messram_parse_string(ram->default_size));
		if (ram->extra_options != NULL)
		{
			int j;
			int size = strlen(ram->extra_options);
			char * const s = mame_strdup(ram->extra_options);
			char * const e = s + size;
			char *p = s;
			for (j=0;j<size;j++) {
				if (p[j]==',') p[j]=0;
			}
			/* try to parse each option */
			while(p <= e)
			{
				fprintf(out, "\t\t<ramoption>%u</ramoption>\n",  messram_parse_string(p));
				p += strlen(p);
				if (p == e)
					break;
				p += 1;
			}

			osd_free(s);
		}
	}
}


/*-------------------------------------------------
    print_mess_game_xml - print MESS specific game
    information.
-------------------------------------------------*/

void print_mess_game_xml(FILE *out, const game_driver *game, const machine_config *config)
{
	print_game_ramoptions( out, game, config );
}

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
