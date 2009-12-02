#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include "devcb.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _terminal_interface terminal_interface;
struct _terminal_interface
{
	devcb_write8 terminal_keyboard_func;
};

#define GENERIC_TERMINAL_INTERFACE(name) const terminal_interface (name) =

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/
#define TERMINAL_TAG "terminal"
#define TERMINAL_SCREEN_TAG "terminal_screen"
 
#define GENERIC_TERMINAL DEVICE_GET_INFO_NAME( terminal )

#define MDRV_GENERIC_TERMINAL_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, GENERIC_TERMINAL, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_GENERIC_TERMINAL_REMOVE(_tag)		\
    MDRV_DEVICE_REMOVE(_tag)

	
/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

WRITE8_DEVICE_HANDLER ( terminal_write );

/* device interface */
DEVICE_GET_INFO( terminal );

MACHINE_DRIVER_EXTERN( generic_terminal );

INPUT_PORTS_EXTERN(generic_terminal);

UINT8 terminal_keyboard_handler(running_machine *machine, devcb_resolved_write8 *callback, UINT8 last_code);

#endif /* __TERMINAL_H__ */
