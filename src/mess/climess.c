/***************************************************************************

    climess.c

    Command-line interface frontend for MESS.

***************************************************************************/

#include "driver.h"
#include "climess.h"
#include "output.h"


/*-------------------------------------------------
    mess_display_help - display MESS help to
	standard output
-------------------------------------------------*/

void mess_display_help(void)
{
	mame_printf_info(
		"M.E.S.S. v%s\n"
		"Multiple Emulation Super System - Copyright (C) 1997-2007 by the MESS Team\n"
		"M.E.S.S. is based on the ever excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2007 by Nicola Salmoria and the MAME Team\n\n",
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
		"See mess.txt for help, readme.txt for options.\n");
}


/*-------------------------------------------------
    info_listdevices - list all devices in MESS
-------------------------------------------------*/

int info_listdevices(const char *gamename)
{
	int i, dev, id;
	const struct IODevice *devices;
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
			begin_resource_tracking();
			devices = devices_allocate(drivers[i]);

			driver_name = drivers[i]->name;

			for (dev = 0; devices[dev].type < IO_COUNT; dev++)
			{
				src = devices[dev].file_extensions;

				for (id = 0; id < devices[dev].count; id++)
				{
					name = device_instancename(&devices[dev].devclass, id);
					shortname = device_briefinstancename(&devices[dev].devclass, id);

					sprintf(paren_shortname, "(%s)", shortname);

					mame_printf_info("%-13s%-12s%-8s   ", driver_name, name, paren_shortname);
					driver_name = " ";

					if (id == 0)
					{
						while (src && *src)
						{
							mame_printf_info(".%-5s", src);
							src += strlen(src) + 1;
						}
					}
					mame_printf_info("\n");
				}
			}
			end_resource_tracking();
		}
		i++;
	}
	return 0;
}



