#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#include <windows.h>

#include "emu.h"
#include "image.h"
#include "msuiutil.h"

BOOL DriverIsComputer(int driver_index)
{
	machine_config *config;
	ioport_list portlist;
	config = machine_config_alloc(drivers[driver_index]->machine_config);
	input_port_list_init(portlist, drivers[driver_index]->ipt, NULL, 0, FALSE);

	const input_field_config *field;
	const input_port_config *port;
	int has_keyboard = FALSE;
	if (portlist.first()==NULL) has_keyboard = TRUE;
	for (port = portlist.first(); port != NULL; port = port->next())
	{
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			if (field->type == IPT_KEYBOARD)
				has_keyboard = TRUE;
				break;
		}
	}
	machine_config_free(config);

	return has_keyboard;
}

BOOL DriverIsModified(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_UNOFFICIAL) != 0;
}

BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type)
{
	BOOL b = FALSE;
	machine_config *config;
	const device_config_image_interface *device;

	// allocate the machine config
	config = machine_config_alloc(gamedrv->machine_config);

	for (bool gotone = config->devicelist.first(device); gotone; gotone = device->next(device))
	{	
		if (device->image_type() == type)
		{
			b = TRUE;
			break;
		}
	}

	machine_config_free(config);
	return b;
}


