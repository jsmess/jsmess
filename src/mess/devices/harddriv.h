/*********************************************************************

	harddriv.h

	MESS interface to the MAME CHD code

*********************************************************************/

#ifndef MESS_HD_H
#define MESS_HD_H

#include "harddisk.h"
#include "image.h"
#include "messdrv.h"

const struct IODevice *mess_hd_device_specify(struct IODevice *iodev, int count);

int device_init_mess_hd(mess_image *image);
int device_load_mess_hd(mess_image *image);
void device_unload_mess_hd(mess_image *image);

hard_disk_file *mess_hd_get_hard_disk_file(mess_image *image);
hard_disk_file *mess_hd_get_hard_disk_file_by_number(int drivenum);
chd_file *mess_hd_get_chd_file(mess_image *image);

void harddisk_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

#endif /* MESS_HD_H */
