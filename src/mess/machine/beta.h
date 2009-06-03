/*********************************************************************

	beta.h

	Implementation of Beta disk drive support for Spectrum and clones
	
	04/05/2008 Created by Miodrag Milanovic

*********************************************************************/
#ifndef BETA_H
#define BETA_H

#include "machine/wd17xx.h"

int betadisk_is_active(const device_config *device);
void betadisk_enable(const device_config *device);
void betadisk_disable(const device_config *device);
void betadisk_clear_status(const device_config *device);

void beta_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

#define BETA_DISK_TAG	"beta"

#define BETA_DISK DEVICE_GET_INFO_NAME( beta_disk )

#define MDRV_BETA_DISK_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, BETA_DISK, 0)

#define MDRV_BETA_DISK_REMOVE(_tag)		\
    MDRV_DEVICE_REMOVE(_tag)
	
READ8_DEVICE_HANDLER(betadisk_status_r);
READ8_DEVICE_HANDLER(betadisk_track_r);
READ8_DEVICE_HANDLER(betadisk_sector_r);
READ8_DEVICE_HANDLER(betadisk_data_r);
READ8_DEVICE_HANDLER(betadisk_state_r);

WRITE8_DEVICE_HANDLER(betadisk_param_w);
WRITE8_DEVICE_HANDLER(betadisk_command_w);
WRITE8_DEVICE_HANDLER(betadisk_track_w);
WRITE8_DEVICE_HANDLER(betadisk_sector_w);
WRITE8_DEVICE_HANDLER(betadisk_data_w);

/* device interface */
DEVICE_GET_INFO( beta_disk );
	
#endif /* BETA_H */
