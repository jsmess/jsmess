/***************************************************************************

    mesvalid.c

    Validity checks on internal MESS data structures.

***************************************************************************/

#include <ctype.h>

#include "mess.h"
#include "device.h"
#include "uitext.h"
#include "inputx.h"


static int validate_device(const device_class *devclass)
{
	int error = 0;
	int is_invalid, i;
	const char *s;
	INT64 devcount, optcount;
	char buf[256];
	char *s1;
	char *s2;
	iodevice_t devtype;
	int (*validity_check)(const device_class *devclass);

	/* critical information */
	devtype = (iodevice_t) (int) device_get_info_int(devclass, DEVINFO_INT_TYPE);
	devcount = device_get_info_int(devclass, DEVINFO_INT_COUNT);

	/* sanity check device type */
	if (devtype >= IO_COUNT)
	{
		mame_printf_error("%s: invalid device type %i\n", devclass->gamedrv->name, (int) devtype);
		error = 1;
	}

	/* sanity check device count */
	if ((devcount <= 0) || (devcount > MAX_DEV_INSTANCES))
	{
		mame_printf_error("%s: device type '%s' has an invalid device count %i\n", devclass->gamedrv->name, device_typename(devtype), (int) devcount);
		error = 1;
	}

	/* File Extensions Checks
	 * 
	 * Checks the following
	 *
	 * 1.  Tests the integrity of the string list
	 * 2.  Checks for duplicate extensions
	 * 3.  Makes sure that all extensions are either lower case chars or numbers
	 */
	s = device_get_info_string(devclass, DEVINFO_STR_FILE_EXTENSIONS);
	if (!s)
	{
		mame_printf_error("%s: device type '%s' has null file extensions\n", devclass->gamedrv->name, device_typename(devtype));
		error = 1;
	}
	else
	{
		memset(buf, 0, sizeof(buf));
		strcpy(buf, s);

		/* convert to be null delimited */
		s1 = buf;
		while(*s1)
		{
			if (*s1 == ',')
				*s1 = '\0';
			s1++;
		}

		s1 = buf;
		while(*s1)
		{
			/* check for invalid chars */
			is_invalid = 0;
			for (s2 = s1; *s2; s2++)
			{
				if (!isdigit(*s2) && !islower(*s2))
					is_invalid = 1;
			}
			if (is_invalid)
			{
				mame_printf_error("%s: device type '%s' has an invalid extension '%s'\n", devclass->gamedrv->name, device_typename(devtype), s1);
				error = 1;
			}
			s2++;

			/* check for dupes */
			is_invalid = 0;
			while(*s2)
			{
				if (!strcmp(s1, s2))
					is_invalid = 1;
				s2 += strlen(s2) + 1;
			}
			if (is_invalid)
			{
				mame_printf_error("%s: device type '%s' has duplicate extensions '%s'\n", devclass->gamedrv->name, device_typename(devtype), s1);
				error = 1;
			}

			s1 += strlen(s1) + 1;
		}
	}

	/* enforce certain rules for certain device types */
	switch(devtype)
	{
		case IO_QUICKLOAD:
		case IO_SNAPSHOT:
			if (devcount != 1)
			{
				mame_printf_error("%s: there can only be one instance of devices of type '%s'\n", devclass->gamedrv->name, device_typename(devtype));
				error = 1;
			}
			/* fallthrough */

		case IO_CARTSLOT:
			if (!device_get_info_int(devclass, DEVINFO_INT_READABLE)
				|| device_get_info_int(devclass, DEVINFO_INT_WRITEABLE)
				|| device_get_info_int(devclass, DEVINFO_INT_CREATABLE))
			{
				mame_printf_error("%s: devices of type '%s' has invalid open modes\n", devclass->gamedrv->name, device_typename(devtype));
				error = 1;
			}
			break;
			
		default:
			break;
	}

	/* check creation options */
	optcount = device_get_info_int(devclass, DEVINFO_INT_CREATE_OPTCOUNT);
	if ((optcount < 0) || (optcount >= DEVINFO_CREATE_OPTMAX))
	{
		mame_printf_error("%s: device type '%s' has an invalid creation optcount\n", devclass->gamedrv->name, device_typename(devtype));
		error = 1;
	}
	else
	{
		for (i = 0; i < (int) optcount; i++)
		{
			if (!device_get_info_string(devclass, DEVINFO_STR_CREATE_OPTNAME + i))
			{
				mame_printf_error("%s: device type '%s' create option #%d: name not present\n",
					devclass->gamedrv->name, device_typename(devtype), i);
				error = 1;
			}
			if (!device_get_info_string(devclass, DEVINFO_STR_CREATE_OPTDESC + i))
			{
				mame_printf_error("%s: device type '%s' create option #%d: description not present\n",
					devclass->gamedrv->name, device_typename(devtype), i);
				error = 1;
			}
			if (!device_get_info_string(devclass, DEVINFO_STR_CREATE_OPTEXTS + i))
			{
				mame_printf_error("%s: device type '%s' create option #%d: extensions not present\n",
					devclass->gamedrv->name, device_typename(devtype), i);
				error = 1;
			}
		}
	}

	/* is there a custom validity check? */
	validity_check = (int (*)(const device_class *)) device_get_info_fct(devclass, DEVINFO_PTR_VALIDITY_CHECK);
	if (validity_check)
	{
		if (validity_check(devclass))
			error = 1;
	}

	return error;
}



int mess_validitychecks(void)
{
	int i, j;
	int error = 0;
	iodevice_t devtype;
	struct IODevice *devices;
	const char *name;
	input_port_entry *inputports = NULL;
	extern int device_valididtychecks(void);
	extern const char *mess_default_text[];
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	int count_overrides[sizeof(handlers) / sizeof(handlers[0])];
	device_class devclass;

	/* make sure that all of the UI_* strings are set for all devices */
	for (devtype = 0; devtype < IO_COUNT; devtype++)
	{
		name = mess_default_text[UI_cartridge - IO_CARTSLOT - UI_last_mame_entry + devtype];
		if (!name || !name[0])
		{
			mame_printf_error("Device type %d does not have an associated UI string\n", devtype);
			error = 1;
		}
	}

	/* MESS specific driver validity checks */
	for (i = 0; drivers[i]; i++)
	{
		devices = devices_allocate(drivers[i]);

		/* make sure that there are no clones that reference nonexistant drivers */
		if (driver_get_clone(drivers[i]))
		{
			if (drivers[i]->compatible_with && !(drivers[i]->compatible_with->flags & NOT_A_DRIVER))
			{
				mame_printf_error("%s: both compatbile_with and clone_of are specified\n", drivers[i]->name);
				error = 1;
			}

			for (j = 0; drivers[j]; j++)
			{
				if (driver_get_clone(drivers[i]) == drivers[j])
					break;
			}
			if (!drivers[j])
			{
				mame_printf_error("%s: is a clone of %s, which is not in drivers[]\n", drivers[i]->name, driver_get_clone(drivers[i])->name);
				error = 1;
			}
		}

		/* make sure that there are no clones that reference nonexistant drivers */
		if (drivers[i]->compatible_with && !(drivers[i]->compatible_with->flags & NOT_A_DRIVER))
		{
			for (j = 0; drivers[j]; j++)
			{
				if (drivers[i]->compatible_with == drivers[j])
					break;
			}
			if (!drivers[j])
			{
				mame_printf_error("%s: is compatible with %s, which is not in drivers[]\n", drivers[i]->name, drivers[i]->compatible_with->name);
				error = 1;
			}
		}

		/* check devices */
		if (drivers[i]->sysconfig_ctor)
		{
			memset(&cfg, 0, sizeof(cfg));
			memset(handlers, 0, sizeof(handlers));
			cfg.device_slotcount = sizeof(handlers) / sizeof(handlers[0]);
			cfg.device_handlers = handlers;
			cfg.device_countoverrides = count_overrides;
			drivers[i]->sysconfig_ctor(&cfg);

			for (j = 0; handlers[j]; j++)
			{
				devclass.gamedrv = drivers[i];
				devclass.get_info = handlers[j];

				if (validate_device(&devclass))
					error = 1;
			}
		}

		/* check system config */
		ram_option_count(drivers[i]);

		/* make sure that our input system likes this driver */
		if (inputx_validitycheck(drivers[i], &inputports))
			error = 1;

		devices = NULL;
	}

	/* call other validity checks */
	if (inputx_validitycheck(NULL, &inputports))
		error = 1;
	if (device_valididtychecks())
		error = 1;

	/* now that we are completed, re-expand the actual driver to compensate
	 * for the tms9928a hack */
	if (Machine && Machine->gamedrv)
	{
		machine_config drv;
		expand_machine_driver(Machine->gamedrv->drv, &drv);
	}

	return error;
}
