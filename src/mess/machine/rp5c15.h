/*
 *  Ricoh RP5C15 RTC header
 */

#ifndef RP5C15_H_
#define RP5C15_H_

#include "emu.h"

typedef struct rp5c15_interface rp5c15_intf;
struct rp5c15_interface
{
	void (*alarm_irq_callback)(int state);
};

DEVICE_GET_INFO(rp5c15);

#define RP5C15 DEVICE_GET_INFO_NAME(rp5c15)

#define MDRV_RP5C15_ADD(_tag, _config) \
	MDRV_DEVICE_ADD(_tag, RP5C15, 0) \
	MDRV_DEVICE_CONFIG(_config)

READ16_DEVICE_HANDLER( rp5c15_r );
WRITE16_DEVICE_HANDLER( rp5c15_w );

#endif /*RP5C15_H_*/
