#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#include <windows.h>

#include "emu.h"
#include "image.h"
#include "msuiutil.h"
#include "mui_opts.h"

BOOL DriverIsComputer(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_TYPE_COMPUTER) != 0;
}

BOOL DriverIsConsole(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_TYPE_CONSOLE) != 0;
}

BOOL DriverIsModified(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_UNOFFICIAL) != 0;
}

BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type)
{
	BOOL b = FALSE;
	const device_config_image_interface *device;

	// allocate the machine config
	machine_config config(*gamedrv,MameUIGlobal());

	for (bool gotone = config.m_devicelist.first(device); gotone; gotone = device->next(device))
	{
		if (device->image_type() == type)
		{
			b = TRUE;
			break;
		}
	}
	return b;
}


