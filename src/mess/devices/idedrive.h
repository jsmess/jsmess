/*********************************************************************

	idedrive.h

	Code to interface the MESS image code with MAME's IDE core

*********************************************************************/

#ifndef IDEDRIVE_H
#define IDEDRIVE_H

#include "driver.h"

enum
{
	DEVINFO_INT_IDEDRIVE_ADDRESS = DEVINFO_INT_DEV_SPECIFIC,

	DEVINFO_PTR_IDEDRIVE_INTERFACE = DEVINFO_PTR_DEV_SPECIFIC
};

void ide_harddisk_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);


#endif /* IDEDRIVE_H */
