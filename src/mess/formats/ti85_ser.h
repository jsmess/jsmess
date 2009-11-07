
#ifndef __TI85SERIAL_H_
#define __TI85SERIAL_H_

#include "device.h"

#define TI85SERIAL	DEVICE_GET_INFO_NAME(ti85serial)
#define TI86SERIAL	DEVICE_GET_INFO_NAME(ti86serial)

DEVICE_GET_INFO(ti85serial);
DEVICE_GET_INFO(ti86serial);

extern void ti85_update_serial(const device_config *device);

#define MDRV_TI85SERIAL_ADD( _tag ) \
		MDRV_DEVICE_ADD( _tag, TI85SERIAL, 0 )

#define MDRV_TI86SERIAL_ADD( _tag ) \
		MDRV_DEVICE_ADD( _tag, TI86SERIAL, 0 )

WRITE8_DEVICE_HANDLER( ti85serial_red_out );
WRITE8_DEVICE_HANDLER( ti85serial_white_out );
READ8_DEVICE_HANDLER( ti85serial_red_in );
READ8_DEVICE_HANDLER( ti85serial_white_in );

#endif
