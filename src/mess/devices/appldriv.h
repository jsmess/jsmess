/*********************************************************************

	appldriv.h

	Apple 5.25" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#ifndef APPLDRIV_H
#define APPLDRIV_H

#include "mame.h"
#include "device.h"

void apple525_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

void apple525_set_lines(const device_config *device,UINT8 lines);
void apple525_set_enable_lines(const device_config *device,int enable_mask);

UINT8 apple525_read_data(const device_config *device);
void apple525_write_data(const device_config *device,UINT8 data);
int apple525_read_status(const device_config *device);
int apple525_get_count(running_machine *machine);

typedef struct _appledriv_config appledriv_config;
struct _appledriv_config
{
	int	dividend;
	int	divisor;

};

#define FLOPPY_APPLE	DEVICE_GET_INFO_NAME(apple525)
DEVICE_GET_INFO(apple525);

#define MDRV_FLOPPY_APPLE_PARAMS(_dividend,_divisor)	\
	MDRV_DEVICE_CONFIG_DATA32(appledriv_config, dividend, _dividend)	\
	MDRV_DEVICE_CONFIG_DATA32(appledriv_config, divisor,  _divisor)

#define MDRV_FLOPPY_APPLE_2_DRIVES_ADD(_config,_dividend,_divisor) 	\
	MDRV_DEVICE_ADD(FLOPPY_0, FLOPPY_APPLE, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MDRV_DEVICE_ADD(FLOPPY_1, FLOPPY_APPLE, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_FLOPPY_APPLE_PARAMS(_dividend,_divisor)

#define MDRV_FLOPPY_APPLE_4_DRIVES_ADD(_config,_dividend,_divisor) 	\
	MDRV_DEVICE_ADD(FLOPPY_0, FLOPPY_APPLE, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MDRV_DEVICE_ADD(FLOPPY_1, FLOPPY_APPLE, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MDRV_DEVICE_ADD(FLOPPY_2, FLOPPY_APPLE, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MDRV_DEVICE_ADD(FLOPPY_3, FLOPPY_APPLE, 0)		\
	MDRV_DEVICE_CONFIG(_config)	\
	MDRV_FLOPPY_APPLE_PARAMS(_dividend,_divisor) 

#define MDRV_FLOPPY_APPLE_2_DRIVES_REMOVE() 	\
	MDRV_DEVICE_REMOVE(FLOPPY_0)		\
	MDRV_DEVICE_REMOVE(FLOPPY_1)		

#endif /* APPLDRIV_H */
