/*********************************************************************

    sonydriv.h

    Apple/Sony 3.5" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#ifndef SONYDRIV_H
#define SONYDRIV_H

#include "image.h"
#include "flopdrv.h"

#if 0
enum
{
	SONY_FLOPPY_ALLOW400K           = 0x0001,
	SONY_FLOPPY_ALLOW800K           = 0x0002,

	SONY_FLOPPY_SUPPORT2IMG         = 0x4000,
	SONY_FLOPPY_EXT_SPEED_CONTROL   = 0x8000    // means the speed is controlled by computer
};
#endif

void sony_set_lines(running_device *device,UINT8 lines);
void sony_set_enable_lines(running_device *device,int enable_mask);
void sony_set_sel_line(running_device *device,int sel);

void sony_set_speed(int speed);

UINT8 sony_read_data(running_device *device);
void sony_write_data(running_device *device,UINT8 data);
int sony_read_status(running_device *device);

#define FLOPPY_SONY	DEVICE_GET_INFO_NAME(sonydriv)
DEVICE_GET_INFO(sonydriv);

#define MDRV_FLOPPY_SONY_2_DRIVES_ADD(_config) 	\
	MDRV_DEVICE_ADD(FLOPPY_0, FLOPPY_SONY, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_ADD(FLOPPY_1, FLOPPY_SONY, 0)		\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_SONY_2_DRIVES_ADDITIONAL_ADD(_config) 	\
	MDRV_DEVICE_ADD(FLOPPY_2, FLOPPY_SONY, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_ADD(FLOPPY_3, FLOPPY_SONY, 0)		\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_FLOPPY_SONY_2_DRIVES_MODIFY(_config) 	\
	MDRV_DEVICE_MODIFY(FLOPPY_0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_DEVICE_MODIFY(FLOPPY_1)		\
	MDRV_DEVICE_CONFIG(_config)



#endif /* SONYDRIV_H */
