/*********************************************************************

	beta.h

	Implementation of Beta disk drive support for Spectrum and clones
	
	04/05/2008 Created by Miodrag Milanovic

*********************************************************************/
#ifndef BETA_H
#define BETA_H

#include "machine/wd17xx.h"

int betadisk_is_active(void);
void betadisk_enable(void);
void betadisk_disable(void);
void betadisk_clear_status(void);

extern const wd17xx_interface beta_wd17xx_interface;

READ8_HANDLER(betadisk_status_r);
READ8_HANDLER(betadisk_track_r);
READ8_HANDLER(betadisk_sector_r);
READ8_HANDLER(betadisk_data_r);
READ8_HANDLER(betadisk_state_r);

WRITE8_HANDLER(betadisk_param_w);
WRITE8_HANDLER(betadisk_command_w);
WRITE8_HANDLER(betadisk_track_w);
WRITE8_HANDLER(betadisk_sector_w);
WRITE8_HANDLER(betadisk_data_w);

void beta_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

#endif /* BETA_H */
