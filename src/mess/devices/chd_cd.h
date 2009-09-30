/*********************************************************************

    chd_cd.h

    MESS interface to the MAME CHD CDROM code

*********************************************************************/

#ifndef MESS_CD_H
#define MESS_CD_H

#include "cdrom.h"

#define CDROM DEVICE_GET_INFO_NAME(cdrom)


#define MDRV_CDROM_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, CDROM, 0) \


DEVICE_GET_INFO(cdrom);


cdrom_file *mess_cd_get_cdrom_file(const device_config *device);

#endif /* MESS_CD_H */
