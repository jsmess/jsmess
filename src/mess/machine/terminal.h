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

DECLARE_LEGACY_DEVICE(GENERIC_TERMINAL, terminal);

#define MCFG_GENERIC_TERMINAL_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, GENERIC_TERMINAL, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#define MCFG_GENERIC_TERMINAL_REMOVE(_tag)		\
    MCFG_DEVICE_REMOVE(_tag)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ_LINE_DEVICE_HANDLER( terminal_serial_r );
WRITE_LINE_DEVICE_HANDLER( terminal_serial_w );

WRITE8_DEVICE_HANDLER ( terminal_write );

MACHINE_CONFIG_EXTERN( generic_terminal );

INPUT_PORTS_EXTERN(generic_terminal);

UINT8 terminal_keyboard_handler(running_machine &machine, devcb_resolved_write8 *callback, UINT8 last_code, UINT8 *scan_line, UINT8 *tx_shift, int *tx_state, device_t *device);

#endif /* __TERMINAL_H__ */
