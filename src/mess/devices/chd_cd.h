/*********************************************************************

	chd_cd.h

	MESS interface to the MAME CHD CDROM code

*********************************************************************/

#ifndef MESS_CD_H
#define MESS_CD_H

#include "cdrom.h"

#define CDROM DEVICE_GET_INFO_NAME(cdrom)

DEVICE_GET_INFO(cdrom);

cdrom_file *mess_cd_get_cdrom_file(const device_config *device);

#endif /* MESS_CD_H */
