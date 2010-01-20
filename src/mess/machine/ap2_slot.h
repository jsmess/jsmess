/*********************************************************************

    ap2_slot.h

    Implementation of Apple II device slots

*********************************************************************/

#ifndef __AP2_SLOT__
#define __AP2_SLOT__

#include "emu.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define APPLE2_SLOT		DEVICE_GET_INFO_NAME(apple2_slot)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _apple2_slot_config apple2_slot_config;
struct _apple2_slot_config
{
	const char *tag;
	read8_device_func rh;
	write8_device_func wh;
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_APPLE2_SLOT_ADD(_slot_number, _slot_device_tag, _rh, _wh)		\
	MDRV_DEVICE_ADD("slot_" #_slot_number, APPLE2_SLOT, 0)			\
	MDRV_DEVICE_CONFIG_DATAPTR(apple2_slot_config, tag, _slot_device_tag)	\
	MDRV_DEVICE_CONFIG_DATAPTR(apple2_slot_config, rh, _rh)		\
	MDRV_DEVICE_CONFIG_DATAPTR(apple2_slot_config, wh, _wh)		\



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device get info function */
DEVICE_GET_INFO(apple2_slot);

/* slot read function */
READ8_DEVICE_HANDLER(apple2_slot_r);

/* slot write function */
WRITE8_DEVICE_HANDLER(apple2_slot_w);

/* slot device lookup */
const device_config *apple2_slot(running_machine *machine, int slotnum);

#endif /* __AP2_SLOT__ */
