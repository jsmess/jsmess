#include "emu.h"
#include "image.h"
#include "msuiutil.h"

BOOL DriverIsComputer(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER) != 0;
}

BOOL DriverIsModified(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER_MODIFIED) != 0;
}

BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type)
{
	BOOL b = FALSE;
	machine_config *config;
	const device_config *device;

	// allocate the machine config
	config = machine_config_alloc(gamedrv->machine_config);

	for (device = image_device_first(config); device != NULL; device = image_device_next(device))
	{
		if (image_device_getinfo(config, device).type == type)
		{
			b = TRUE;
			break;
		}
	}

	machine_config_free(config);
	return b;
}


