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

DECLARE_LEGACY_DEVICE(PCF8593, pcf8593);

#define MDRV_PCF8593_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, PCF8593, 0) \

#define MDRV_PCF8593_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
/* pins */
void pcf8593_pin_scl(running_device *device, int data);
void pcf8593_pin_sda_w(running_device *device, int data);
int  pcf8593_pin_sda_r(running_device *device);

/* load/save */
void pcf8593_load(running_device *device, mame_file *file);
void pcf8593_save(running_device *device, mame_file *file);

#endif /* __PCF8593_H__ */
