/*********************************************************************

    appldriv.h

    Apple 5.25" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#ifndef APPLDRIV_H
#define APPLDRIV_H

#include "emu.h"

void apple525_set_lines(device_t *device,UINT8 lines);
void apple525_set_enable_lines(device_t *device,int enable_mask);

UINT8 apple525_read_data(device_t *device);
void apple525_write_data(device_t *device,UINT8 data);
int apple525_read_status(device_t *device);
int apple525_get_count(running_machine &machine);

typedef struct _appledriv_config appledriv_config;
struct _appledriv_config
{
	int	dividend;
	int	divisor;

};

DECLARE_LEGACY_IMAGE_DEVICE(FLOPPY_APPLE, apple525);

#define MCFG_FLOPPY_APPLE_PARAMS(_dividend,_divisor)	\
	MCFG_DEVICE_CONFIG_DATA32(appledriv_config, dividend, _dividend)	\
	MCFG_DEVICE_CONFIG_DATA32(appledriv_config, divisor,  _divisor)

#define MCFG_FLOPPY_APPLE_2_DRIVES_ADD(_config,_dividend,_divisor)	\
	MCFG_DEVICE_ADD(FLOPPY_0, FLOPPY_APPLE, 0)		\
	MCFG_DEVICE_CONFIG(_config)	\
	MCFG_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MCFG_DEVICE_ADD(FLOPPY_1, FLOPPY_APPLE, 0)		\
	MCFG_DEVICE_CONFIG(_config)	\
	MCFG_FLOPPY_APPLE_PARAMS(_dividend,_divisor)

#define MCFG_FLOPPY_APPLE_4_DRIVES_ADD(_config,_dividend,_divisor)	\
	MCFG_DEVICE_ADD(FLOPPY_0, FLOPPY_APPLE, 0)		\
	MCFG_DEVICE_CONFIG(_config)	\
	MCFG_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MCFG_DEVICE_ADD(FLOPPY_1, FLOPPY_APPLE, 0)		\
	MCFG_DEVICE_CONFIG(_config)	\
	MCFG_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MCFG_DEVICE_ADD(FLOPPY_2, FLOPPY_APPLE, 0)		\
	MCFG_DEVICE_CONFIG(_config)	\
	MCFG_FLOPPY_APPLE_PARAMS(_dividend,_divisor) \
	MCFG_DEVICE_ADD(FLOPPY_3, FLOPPY_APPLE, 0)		\
	MCFG_DEVICE_CONFIG(_config)	\
	MCFG_FLOPPY_APPLE_PARAMS(_dividend,_divisor)

#define MCFG_FLOPPY_APPLE_2_DRIVES_REMOVE() 	\
	MCFG_DEVICE_REMOVE(FLOPPY_0)		\
	MCFG_DEVICE_REMOVE(FLOPPY_1)

#endif /* APPLDRIV_H */
