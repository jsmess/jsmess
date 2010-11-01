#ifndef __GENBOARD__
#define __GENBOARD__

#include "machine/tms9901.h"

READ8_DEVICE_HANDLER( geneve_r );
WRITE8_DEVICE_HANDLER( geneve_w );
READ8_DEVICE_HANDLER( geneve_cru_r );
WRITE8_DEVICE_HANDLER( geneve_cru_w );

extern const tms9901_interface tms9901_wiring_geneve;
void tms9901_gen_set_int2(running_machine *machine, int state);

/* device interface */
DECLARE_LEGACY_DEVICE( GENBOARD, genboard );

WRITE_LINE_DEVICE_HANDLER( board_inta );
WRITE_LINE_DEVICE_HANDLER( board_intb );
WRITE_LINE_DEVICE_HANDLER( board_ready );

#define MDRV_GENEVE_BOARD_ADD(_tag )			\
	MDRV_DEVICE_ADD(_tag, GENBOARD, 0)

#endif
