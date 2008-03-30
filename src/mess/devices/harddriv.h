/*********************************************************************

	harddriv.h

	MESS interface to the MAME CHD code

*********************************************************************/

#ifndef MESS_HD_H
#define MESS_HD_H

#include "device.h"
#include "image.h"
#include "harddisk.h"


DEVICE_START( mess_hd );
DEVICE_IMAGE_LOAD( mess_hd );
DEVICE_IMAGE_UNLOAD( mess_hd );

hard_disk_file *mess_hd_get_hard_disk_file(const device_config *image);
hard_disk_file *mess_hd_get_hard_disk_file_by_number(int drivenum);
chd_file *mess_hd_get_chd_file(const device_config *image);

void harddisk_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* MESS_HD_H */
