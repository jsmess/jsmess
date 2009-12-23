/*********************************************************************

    harddriv.h

    MESS interface to the MAME CHD code

*********************************************************************/

#ifndef MESS_HD_H
#define MESS_HD_H

#include "image.h"
#include "harddisk.h"

#define HARDDISK		DEVICE_GET_INFO_NAME(mess_hd)
#define IDE_HARDDISK	DEVICE_GET_INFO_NAME(mess_ide)

#define MDRV_HARDDISK_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, HARDDISK, 0) \

#define MDRV_IDE_HARDDISK_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, IDE_HARDDISK, 0) \

DEVICE_GET_INFO( mess_hd );
DEVICE_GET_INFO( mess_ide );

hard_disk_file *mess_hd_get_hard_disk_file(const device_config *device);
chd_file *mess_hd_get_chd_file(const device_config *device);

struct harddisk_callback_config
{
	device_image_load_func		device_image_load;
	device_image_unload_func	device_image_unload;
};


#endif /* MESS_HD_H */
