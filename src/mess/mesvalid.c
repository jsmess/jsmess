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
		/* check compatibility chain */
		error = validate_compatibility_chain(drivers[i]) || error;
	}

	/* call other validity checks */
	error = mess_validate_natural_keyboard_statics() || error;
	error = device_valididtychecks() || error;

	return error;
}
