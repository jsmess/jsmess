/*********************************************************************

	sonydriv.h

	Apple/Sony 3.5" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#ifndef SONYDRIV_H
#define SONYDRIV_H

#include "device.h"
#include "image.h"


enum
{
	MESS_DEVINFO_INT_SONYDRIV_ALLOWABLE_SIZES = MESS_DEVINFO_INT_DEV_SPECIFIC
};

/* defines for the allowablesizes param below */
enum
{
	SONY_FLOPPY_ALLOW400K			= 0x0001,
	SONY_FLOPPY_ALLOW800K			= 0x0002,

	SONY_FLOPPY_SUPPORT2IMG			= 0x4000,
	SONY_FLOPPY_EXT_SPEED_CONTROL	= 0x8000	/* means the speed is controlled by computer */
};

void sonydriv_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

void sony_set_lines(const device_config *device,UINT8 lines);
void sony_set_enable_lines(const device_config *device,int enable_mask);
void sony_set_sel_line(const device_config *device,int sel);

void sony_set_speed(int speed);

UINT8 sony_read_data(const device_config *device);
void sony_write_data(const device_config *device,UINT8 data);
int sony_read_status(const device_config *device);


#endif /* SONYDRIV_H */
