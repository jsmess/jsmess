/*********************************************************************

	idedrive.c

	Code to interface the MESS image code with MAME's IDE core

	Raphael Nabet 2003

 *********************************************************************/

#include "driver.h"
#include "machine/idectrl.h"
#include "devices/harddriv.h"
#include "idedrive.h"



static void ide_get_params(mess_image *image, int *which_bus, int *which_address,
	struct ide_interface **intf,
	device_init_handler *parent_init,
	device_load_handler *parent_load,
	device_unload_handler *parent_unload)
{
	const device_class *devclass = &image_device(image)->devclass;
	device_class parent_devclass;

	*which_bus = image_index_in_device(image);
	*which_address = (int) device_get_info_int(devclass, DEVINFO_INT_IDEDRIVE_ADDRESS);
	*intf = (struct ide_interface *) device_get_info_ptr(devclass, DEVINFO_PTR_IDEDRIVE_INTERFACE);

	parent_devclass = *devclass;
	parent_devclass.get_info = harddisk_device_getinfo;

	if (parent_init)
		*parent_init = (device_init_handler) device_get_info_fct(&parent_devclass, DEVINFO_PTR_INIT);
	if (parent_load)
		*parent_load = (device_load_handler) device_get_info_fct(&parent_devclass, DEVINFO_PTR_LOAD);
	if (parent_unload)
		*parent_unload = (device_unload_handler) device_get_info_fct(&parent_devclass, DEVINFO_PTR_UNLOAD);

	assert(*which_address == 0);
	assert(*intf);
}



/*-------------------------------------------------
	ide_hd_init - Init an IDE hard disk device
-------------------------------------------------*/

static int ide_hd_init(mess_image *image)
{
	int result, which_bus, which_address;
	struct ide_interface *intf;
	device_init_handler parent_init;

	/* get the basics */
	ide_get_params(image, &which_bus, &which_address, &intf, &parent_init, NULL, NULL);

	/* call the parent init function */
	result = parent_init(image);
	if (result != INIT_PASS)
		return result;

	/* configure IDE */
	ide_controller_init_custom(which_bus, intf, NULL);
	return INIT_PASS;
}



/*-------------------------------------------------
	ide_hd_load - Load an IDE hard disk image
-------------------------------------------------*/

static int ide_hd_load(mess_image *image)
{
	int result, which_bus, which_address;
	struct ide_interface *intf;
	device_load_handler parent_load;

	/* get the basics */
	ide_get_params(image, &which_bus, &which_address, &intf, NULL, &parent_load, NULL);

	/* call the parent load function */
	result = parent_load(image);
	if (result != INIT_PASS)
		return result;

	/* configure IDE */
	ide_controller_init_custom(which_bus, intf, mess_hd_get_chd_file(image));
	ide_controller_reset(which_bus);
	return INIT_PASS;
}



/*-------------------------------------------------
	ide_hd_unload - Unload an IDE hard disk image
-------------------------------------------------*/

static void ide_hd_unload(mess_image *image)
{
	int which_bus, which_address;
	struct ide_interface *intf;
	device_unload_handler parent_unload;

	/* get the basics */
	ide_get_params(image, &which_bus, &which_address, &intf, NULL, NULL, &parent_unload);

	/* call the parent unload function */
	parent_unload(image);

	/* configure IDE */
	ide_controller_init_custom(which_bus, intf, NULL);
	ide_controller_reset(which_bus);
}



/*-------------------------------------------------
	ide_hd_validity_check - check this device's validity
-------------------------------------------------*/

static int ide_hd_validity_check(const device_class *devclass)
{
	int error = 0;
	int which_address;
	INT64 count;
	struct ide_interface *intf;

	which_address = (int) device_get_info_int(devclass, DEVINFO_INT_IDEDRIVE_ADDRESS);
	intf = (struct ide_interface *) device_get_info_ptr(devclass, DEVINFO_PTR_IDEDRIVE_INTERFACE);
	count = device_get_info_int(devclass, DEVINFO_INT_COUNT);

	if (which_address != 0)
	{
		mame_printf_error("%s: IDE device has non-zero address\n", devclass->gamedrv->name);
		error = 1;
	}

	if (!intf)
	{
		mame_printf_error("%s: IDE device does not specify an interface\n", devclass->gamedrv->name);
		error = 1;
	}

	if (count > MAX_IDE_CONTROLLERS)
	{
		mame_printf_error("%s: Too many IDE devices; maximum is %d\n", devclass->gamedrv->name, MAX_IDE_CONTROLLERS);
		error = 1;
	}

	return error;
}



/*-------------------------------------------------
	ide_harddisk_device_getinfo - Get info proc
-------------------------------------------------*/

void ide_harddisk_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:					info->init = ide_hd_init; break;
		case DEVINFO_PTR_LOAD:					info->load = ide_hd_load; break;
		case DEVINFO_PTR_UNLOAD:				info->unload = ide_hd_unload; break;
		case DEVINFO_PTR_VALIDITY_CHECK:		info->validity_check = ide_hd_validity_check; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s = device_temp_str(), "ideharddrive"); break;
		case DEVINFO_STR_SHORT_NAME:			strcpy(info->s = device_temp_str(), "idehd"); break;
		case DEVINFO_STR_DESCRIPTION:			strcpy(info->s = device_temp_str(), "IDE Hard Disk"); break;

		default: harddisk_device_getinfo(devclass, state, info); break;
	}
}
