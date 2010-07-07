/*********************************************************************

    z80bin.h

    A binary quickload format used by the Microbee, the Exidy Sorcerer
    VZ200/300 and the Super 80

*********************************************************************/

#ifndef __Z80BIN_H__
#define __Z80BIN_H__

#include "image.h"
#include "snapquik.h"


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_IMAGE_DEVICE(Z80BIN, z80bin);

#define Z80BIN_EXECUTE_NAME(name)	z80bin_execute_##name
#define Z80BIN_EXECUTE(name)		void Z80BIN_EXECUTE_NAME(name)(running_machine *machine, UINT16 start_address, UINT16 end_address, UINT16 execute_address, int autorun)

#define z80bin_execute_default		NULL



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*z80bin_execute_func)(running_machine *machine, UINT16 start_address, UINT16 end_address, UINT16 execute_address, int autorun);

typedef struct _z80bin_config z80bin_config;
struct _z80bin_config
{
	snapquick_config base;
	z80bin_execute_func execute;
};



/***************************************************************************
    Z80BIN QUICKLOAD DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_Z80BIN_QUICKLOAD_ADD(_tag, _execute, _delay) \
	MDRV_DEVICE_ADD(_tag, Z80BIN, 0) \
	MDRV_DEVICE_CONFIG_DATA64(snapquick_config, delay_seconds, (seconds_t) (_delay)) \
	MDRV_DEVICE_CONFIG_DATA64(snapquick_config, delay_attoseconds, (attoseconds_t) (((_delay) - (int)(_delay)) * ATTOSECONDS_PER_SECOND)) \
	MDRV_DEVICE_CONFIG_DATAPTR(z80bin_config, execute, Z80BIN_EXECUTE_NAME(_execute))

#endif /* __Z80BIN_H__ */
