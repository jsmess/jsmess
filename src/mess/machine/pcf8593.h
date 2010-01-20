/*********************************************************************

    Philips PCF8593 CMOS clock/calendar circuit

    (c) 2001-2007 Tim Schuerewegen

*********************************************************************/

#ifndef __PCF8593_H__
#define __PCF8593_H__

#include "emu.h"


/***************************************************************************
    MACROS
***************************************************************************/

#define PCF8593		DEVICE_GET_INFO_NAME(pcf8593)

#define MDRV_PCF8593_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, PCF8593, 0) \

#define MDRV_PCF8593_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(pcf8593);

/* pins */
void pcf8593_pin_scl(const device_config *device, int data);
void pcf8593_pin_sda_w(const device_config *device, int data);
int  pcf8593_pin_sda_r(const device_config *device);

/* load/save */
void pcf8593_load(const device_config *device, mame_file *file);
void pcf8593_save(const device_config *device, mame_file *file);

#endif /* __PCF8593_H__ */
