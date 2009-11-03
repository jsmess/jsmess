/***************************************************************************

    mesvalid.c

    Validity checks on internal MESS data structures.

***************************************************************************/

#include <ctype.h>

#include "mame.h"
#include "mess.h"
#include "device.h"
#include "inputx.h"


/*************************************
 *
 *  Validate devices
 *
 *************************************/

static int validate_device(const mess_device_class *devclass)
{
	int error = 0;
	int is_invalid, i;
	const char *s;
	INT64 devcount, optcount;
	char buf[256];
	char *s1;
	char *s2;
	iodevice_t devtype;
	int (*validity_check)(const mess_device_class *devclass);

	/* critical information */
	devtype = (iodevice_t) (int) mess_device_get_info_int(devclass, MESS_DEVINFO_INT_TYPE);
	devcount = mess_device_get_info_int(devclass, MESS_DEVINFO_INT_COUNT);

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
	s = mess_device_get_info_string(devclass, MESS_DEVINFO_STR_FILE_EXTENSIONS);
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
			if (!mess_device_get_info_int(devclass, MESS_DEVINFO_INT_READABLE)
				|| mess_device_get_info_int(devclass, MESS_DEVINFO_INT_WRITEABLE)
				|| mess_device_get_info_int(devclass, MESS_DEVINFO_INT_CREATABLE))
			{
				mame_printf_error("%s: devices of type '%s' has invalid open modes\n", devclass->gamedrv->name, device_typename(devtype));
				error = 1;
			}
			break;

		default:
			break;
	}

	/* check creation options */
	optcount = mess_device_get_info_int(devclass, MESS_DEVINFO_INT_CREATE_OPTCOUNT);
	if ((optcount < 0) || (optcount >= DEVINFO_CREATE_OPTMAX))
	{
		mame_printf_error("%s: device type '%s' has an invalid creation optcount\n", devclass->gamedrv->name, device_typename(devtype));
		error = 1;
	}
	else
	{
		for (i = 0; i < (int) optcount; i++)
		{
			if (!mess_device_get_info_string(devclass, MESS_DEVINFO_STR_CREATE_OPTNAME + i))
			{
				mame_printf_error("%s: device type '%s' create option #%d: name not present\n",
					devclass->gamedrv->name, device_typename(devtype), i);
				error = 1;
			}
			if (!mess_device_get_info_string(devclass, MESS_DEVINFO_STR_CREATE_OPTDESC + i))
			{
				mame_printf_error("%s: device type '%s' create option #%d: description not present\n",
					devclass->gamedrv->name, device_typename(devtype), i);
				error = 1;
			}
			if (!mess_device_get_info_string(devclass, MESS_DEVINFO_STR_CREATE_OPTEXTS + i))
			{
				mame_printf_error("%s: device type '%s' create option #%d: extensions not present\n",
					devclass->gamedrv->name, device_typename(devtype), i);
				error = 1;
			}
		}
	}

	/* is there a custom validity check? */
	validity_check = (int (*)(const mess_device_class *)) mess_device_get_info_fct(devclass, MESS_DEVINFO_PTR_VALIDITY_CHECK);
	if (validity_check)
	{
		if (validity_check(devclass))
			error = 1;
	}

	return error;
}



static int validate_driver_devices(const game_driver *drv)
{
	int error = 0;
	int i;
	struct SystemConfigurationParamBlock cfg;
	device_getinfo_handler handlers[64];
	mess_device_class devclass;
	int count_overrides[ARRAY_LENGTH(handlers)];

	if (drv->sysconfig_ctor)
	{
		memset(&cfg, 0, sizeof(cfg));
		memset(handlers, 0, sizeof(handlers));
		cfg.device_slotcount = ARRAY_LENGTH(handlers);
		cfg.device_handlers = handlers;
		cfg.device_countoverrides = count_overrides;
		drv->sysconfig_ctor(&cfg);

		for (i = 0; handlers[i]; i++)
		{
			devclass.gamedrv = drv;
			devclass.get_info = handlers[i];

			error = validate_device(&devclass) || error;
		}
	}
	return error;
}



/*************************************
 *
 *  Validate compatibility chain
 *
 *************************************/

static int validate_compatibility_chain(const game_driver *drv)
{
	int error = 0;
	const game_driver *next_drv;
	const game_driver *other_drv;
	const char *compatible_with;

	/* normalize drv->compatible_with */
	compatible_with = drv->compatible_with;
	if ((compatible_with != NULL) && !strcmp(compatible_with, "0"))
		compatible_with = NULL;

	/* check for this driver being compatible with a non-existant driver */
	if ((compatible_with != NULL) && (driver_get_name(drv->compatible_with) == NULL))
	{
		mame_printf_error("%s: is compatible with %s, which is not in drivers[]\n", drv->name, drv->compatible_with);
		error = 1;
	}

	/* check for clone_of and compatible_with being specified at the same time */
	if ((driver_get_clone(drv) != NULL) && (compatible_with != NULL))
	{
		mame_printf_error("%s: both compatible_with and clone_of are specified\n", drv->name);
		error = 1;
	}

	/* look for recursive compatibility */
	while(!error && (drv != NULL))
	{
		/* identify the next driver */
		next_drv = mess_next_compatible_driver(drv);

		/* find any recursive dependencies on the current driver */
		for (other_drv = next_drv; other_drv != NULL; other_drv = mess_next_compatible_driver(other_drv))
		{
			if (drv == other_drv)
			{
				mame_printf_error("%s: recursive compatibility\n", drv->name);
				error = 1;
				break;
			}
		}

		/* advance to next driver */
		drv = next_drv;
	}

	return error;
}




/*************************************
 *
 *  MESS validity checks
 *
 *************************************/

int mess_validitychecks(void)
{
	int i;
	int error = 0;

	/* MESS specific driver validity checks */
	for (i = 0; drivers[i]; i++)
	{
		/* check devices */
		error = validate_driver_devices(drivers[i]) || error;

		/* check compatibility chain */
		error = validate_compatibility_chain(drivers[i]) || error;
	}

	/* call other validity checks */
	error = mess_validate_natural_keyboard_statics() || error;
	error = device_valididtychecks() || error;

	return error;
}
