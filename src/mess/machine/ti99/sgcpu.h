#ifndef __SGCPU__
#define __SGCPU__

#include "machine/tms9901.h"

void tms9901_sg_set_int2(running_machine &machine, int state);

READ16_DEVICE_HANDLER( sgcpu_r );
WRITE16_DEVICE_HANDLER( sgcpu_w );

READ8_DEVICE_HANDLER( sgcpu_cru_r );
WRITE8_DEVICE_HANDLER( sgcpu_cru_w );

WRITE_LINE_DEVICE_HANDLER( card_extint );
WRITE_LINE_DEVICE_HANDLER( card_notconnected );
WRITE_LINE_DEVICE_HANDLER( card_ready );

/* device interface */
DECLARE_LEGACY_DEVICE( SGCPU, sgcpu );

#define MCFG_SGCPUB_ADD(_tag )			\
	MCFG_DEVICE_ADD(_tag, SGCPU, 0)

extern const tms9901_interface tms9901_wiring_ti99_4p;

#endif
