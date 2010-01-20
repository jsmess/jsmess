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

/* New listmedia is below - left this here just in case.. */
#if 0
/*-------------------------------------------------
    info_listdevices - list all devices in MESS
-------------------------------------------------*/

int info_listdevices(core_options *opts, const char *gamename)
{
	int i;
	machine_config *config;
	const device_config *dev;
	image_device_info info;
	const char *src;
	const char *driver_name;
	const char *name;
	const char *shortname;
	char paren_shortname[16];

	i = 0;

	mame_printf_info(" SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
	mame_printf_info("----------  --------------------  ------------------------------------\n");

	while (drivers[i])
	{
		if (!core_strwildcmp(gamename, drivers[i]->name))
		{
			/* allocate the machine config */
			config = machine_config_alloc(drivers[i]->machine_config);

			driver_name = drivers[i]->name;

			for (dev = image_device_first(config); dev != NULL; dev = image_device_next(dev))
			{
				info = image_device_getinfo(config, dev);

				src = info.file_extensions;
				name = info.instance_name;
				shortname = info.brief_instance_name;

				sprintf(paren_shortname, "(%s)", shortname);

				mame_printf_info("%-13s%-12s%-8s   ", driver_name, name, paren_shortname);
				driver_name = " ";

				while (src && *src)
				{
					mame_printf_info(".%-5s", src);
					src += strlen(src) + 1;
				}
				mame_printf_info("\n");
			}
			machine_config_free(config);
		}
		i++;
	}
	return 0;
}
#endif


/*-------------------------------------------------
    cli_info_listdevices - output the list of
    devices referenced by a given game or set of
    games
-------------------------------------------------*/

int info_listmedia(core_options *options, const char *gamename)
{
	int count = 0, devcount;
	int drvindex;
	machine_config *config;
	const device_config *dev;
	image_device_info info;
	const char *src;
	const char *driver_name;
	const char *name;
	const char *shortname;
	char paren_shortname[16];

	printf(" SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
	printf("----------  --------------------  ------------------------------------\n");

	/* iterate over drivers */
	for (drvindex = 0; drivers[drvindex] != NULL; drvindex++)
		if (mame_strwildcmp(gamename, drivers[drvindex]->name) == 0)
		{
			/* allocate the machine config */
			config = machine_config_alloc(drivers[drvindex]->machine_config);

			driver_name = drivers[drvindex]->name;

			devcount = 0;

			for (dev = image_device_first(config); dev != NULL; dev = image_device_next(dev))
			{
				info = image_device_getinfo(config, dev);

				src = info.file_extensions;
				name = info.instance_name;
				shortname = info.brief_instance_name;

				sprintf(paren_shortname, "(%s)", shortname);

				printf("%-13s%-12s%-8s   ", driver_name, name, paren_shortname);
				driver_name = " ";

				while (src && *src)
				{
					printf(".%-5s", src);
					src += strlen(src) + 1;
					devcount++;
				}
				printf("\n");
			}
			if (!devcount)
				printf("%-13s(none)\n",driver_name);

			count++;
			machine_config_free(config);
		}

	if (!count)
		printf("There are no Computers or Consoles named %s\n", gamename);

	return (count > 0) ? MAMERR_NONE : MAMERR_NO_SUCH_GAME;
}


/*-------------------------------------------------
    match_roms - scan for a matching software ROM by hash
-------------------------------------------------*/
void mess_match_roms(const char *hash, int length, int *found)
{
	int listnum;

	for ( listnum = 0; software_lists[listnum] != NULL; listnum++ )
	{
		const software_list *swlist = software_lists[listnum];
		int j;

		for ( j = 0; swlist->entries[j].name != NULL; j++ )
		{
			const software_entry *sw = &swlist->entries[j];
			const rom_entry *region, *rom;

			for (region = &sw->rom_info[0]; region; region = rom_next_region(region))
			{
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					if (hash_data_is_equal(hash, ROM_GETHASHDATA(rom), 0))
					{
						int baddump = hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP);

						/* output information about the match */
						if (*found != 0)
							mame_printf_info("                    ");
						mame_printf_info("= %s%-20s  %-10s %s [%s]\n", baddump ? "(BAD) " : "", ROM_GETNAME(rom), sw->name, sw->fullname, swlist->name);
						(*found)++;
					}
				}
			}
		}
	}
}


