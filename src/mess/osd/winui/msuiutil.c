#include "driver.h"
#include "image.h"
#include "device.h"
#include "msuiutil.h"

BOOL DriverIsComputer(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER) != 0;
}

BOOL DriverIsModified(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER_MODIFIED) != 0;
}

BOOL DriverUsesMouse(int driver_index)
{
	const input_port_entry *input_ports;
	BOOL retval = FALSE;

	if (drivers[driver_index]->ipt)
	{
		begin_resource_tracking();
		input_ports = input_port_allocate(drivers[driver_index]->ipt, NULL);

		while (input_ports->type != IPT_END)
		{
			if (input_ports->type == IPT_MOUSE_X || input_ports->type == IPT_MOUSE_Y)
			{
				retval = TRUE;
				break;
			}
			input_ports++;
		}

		end_resource_tracking();
	}

	return retval;
}

BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type)
{
	BOOL b = FALSE;
	machine_config *config;
	const device_config *device;

	// allocate the machine config
	config = machine_config_alloc_with_mess_devices(gamedrv);

	for (device = image_device_first(config); device != NULL; device = image_device_next(device))
	{
		const struct IODevice *iodev = mess_device_from_core_device(device);
		if (iodev->type == type)
		{
			b = TRUE;
			break;
		}
	}

	machine_config_free(config);
	return b;
}


