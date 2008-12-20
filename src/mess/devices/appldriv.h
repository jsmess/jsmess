/*********************************************************************

	appldriv.h

	Apple 5.25" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#ifndef APPLDRIV_H
#define APPLDRIV_H

#include "mame.h"
#include "device.h"


enum
{
	MESS_DEVINFO_INT_APPLE525_SPINFRACT_DIVIDEND = MESS_DEVINFO_INT_DEV_SPECIFIC,
	MESS_DEVINFO_INT_APPLE525_SPINFRACT_DIVISOR
};

void apple525_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

void apple525_set_lines(const device_config *device,UINT8 lines);
void apple525_set_enable_lines(const device_config *device,int enable_mask);

UINT8 apple525_read_data(const device_config *device);
void apple525_write_data(const device_config *device,UINT8 data);
int apple525_read_status(const device_config *device);


#endif /* APPLDRIV_H */
