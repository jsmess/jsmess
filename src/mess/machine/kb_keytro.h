/***************************************************************************

    Keytronic Keyboard

***************************************************************************/

#ifndef __KB_KEYTRO_H__
#define __KB_KEYTRO_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _kb_keytronic_interface kb_keytronic_interface;
struct _kb_keytronic_interface
{
	devcb_write_line out_clock_func;
	devcb_write_line out_data_func;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
WRITE_LINE_DEVICE_HANDLER( kb_keytronic_clock_w );
WRITE_LINE_DEVICE_HANDLER( kb_keytronic_data_w );


/***************************************************************************
    INPUT PORTS
***************************************************************************/

INPUT_PORTS_EXTERN( kb_keytronic_pc );
INPUT_PORTS_EXTERN( kb_keytronic_at );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(KB_KEYTRONIC, kb_keytr);

#define MCFG_KB_KEYTRONIC_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, KB_KEYTRONIC, 0) \
	MCFG_DEVICE_CONFIG(_interface)


#endif  /* __KB_KEYTRO_H__ */
