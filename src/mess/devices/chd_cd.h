/*********************************************************************

	chd_cd.h

	MESS interface to the MAME CHD CDROM code

*********************************************************************/

#ifndef MESS_CD_H
#define MESS_CD_H

#include "device.h"
#include "image.h"
#include "cdrom.h"


cdrom_file *mess_cd_get_cdrom_file(const device_config *image);
chd_file *mess_cd_get_chd_file(const device_config *image);

void cdrom_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

cdrom_file *mess_cd_get_cdrom_file_by_number(const char *diskregion);


#endif /* MESS_CD_H */
