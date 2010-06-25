#ifndef __TELEPRINTER_H__
#define __TELEPRINTER_H__

#include "devcb.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _teleprinter_interface teleprinter_interface;
struct _teleprinter_interface
{
	devcb_write8 teleprinter_keyboard_func;
};

#define GENERIC_TELEPRINTER_INTERFACE(name) const teleprinter_interface (name) =

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/
#define TELEPRINTER_TAG "teleprinter"
#define TELEPRINTER_SCREEN_TAG "tty_screen"

DECLARE_LEGACY_DEVICE(GENERIC_TELEPRINTER, teleprinter);

#define MDRV_GENERIC_TELEPRINTER_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, GENERIC_TELEPRINTER, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_GENERIC_TELEPRINTER_REMOVE(_tag)		\
    MDRV_DEVICE_REMOVE(_tag)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

WRITE8_DEVICE_HANDLER ( teleprinter_write );

MACHINE_DRIVER_EXTERN( generic_teleprinter );

INPUT_PORTS_EXTERN(generic_teleprinter);

#endif /* __TELEPRINTER_H__ */
