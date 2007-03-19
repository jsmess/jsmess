/*********************************************************************

	chd_cd.h

	MESS interface to the MAME CHD CDROM code

*********************************************************************/

#ifndef MESS_CD_H
#define MESS_CD_H

#include "cdrom.h"
#include "image.h"
#include "messdrv.h"

const struct IODevice *mess_cd_device_specify(struct IODevice *iodev, int count);

cdrom_file *mess_cd_get_cdrom_file(mess_image *image);
chd_file *mess_cd_get_chd_file(mess_image *image);

void cdrom_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

cdrom_file *mess_cd_get_cdrom_file_by_number(int drivenum);

#endif /* MESS_CD_H */
