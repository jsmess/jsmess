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
		"MESS v%s\n"
		"Multi Emulator Super System - Copyright (C) 1997-2008 by the MESS Team\n"
		"MESS is based on MAME Source code\n"
		"Copyright (C) 1997-2008 by Nicola Salmoria and the MAME Team\n\n",
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

int info_listdevices(core_options *opts, const char *gamename)
{
	int i;
	machine_config *config;
	const device_config *dev;
	const struct IODevice *iodev;
	const char *src;
	const char *driver_name;
	const char *name;
	const char *shortname;
	char paren_shortname[16];

	/* since we expand the machine driver, we need to set things up */
	init_resource_tracking();

	i = 0;

	mame_printf_info(" SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
	mame_printf_info("----------  --------------------  ------------------------------------\n");

	while (drivers[i])
	{
		if (!core_strwildcmp(gamename, drivers[i]->name))
		{
			/* allocate the machine config */
			config = machine_config_alloc_with_mess_devices(drivers[i]);

			driver_name = drivers[i]->name;

			for (dev = image_device_first(config); dev != NULL; dev = image_device_next(dev))
			{
				iodev = mess_device_from_core_device(dev);

				src = iodev->file_extensions;

				name = device_instancename(&iodev->devclass, iodev->index_in_device);
				shortname = device_briefinstancename(&iodev->devclass, iodev->index_in_device);

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

	/* clean up our tracked resources */
	exit_resource_tracking();

	return 0;
}



