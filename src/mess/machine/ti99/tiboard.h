#ifndef __TIBOARD__
#define __TIBOARD__

#include "machine/tms9901.h"

void tms9901_set_int2(running_machine &machine, int state);

extern const tms9901_interface tms9901_wiring_ti99_4;
extern const tms9901_interface tms9901_wiring_ti99_4a;
extern const tms9901_interface tms9901_wiring_ti99_8;

typedef struct _tiboard_config
{
	int mode;
} tiboard_config;

enum
{
	TI994 = 1,
	TI994A,
	TI998
};

/* device interface */
DECLARE_LEGACY_DEVICE( TIBOARD, tiboard );

WRITE_LINE_DEVICE_HANDLER( console_extint );
WRITE_LINE_DEVICE_HANDLER( console_notconnected );
WRITE_LINE_DEVICE_HANDLER( console_ready );

INTERRUPT_GEN( ti99_vblank_interrupt );

#define MCFG_TI994_BOARD_ADD(_tag )			\
	MCFG_DEVICE_ADD(_tag, TIBOARD, 0) \
	MCFG_DEVICE_CONFIG_DATA32( tiboard_config, mode, TI994)

#define MCFG_TI994A_BOARD_ADD(_tag )			\
	MCFG_DEVICE_ADD(_tag, TIBOARD, 0) \
	MCFG_DEVICE_CONFIG_DATA32( tiboard_config, mode, TI994A)

#define MCFG_TI998_BOARD_ADD(_tag )			\
	MCFG_DEVICE_ADD(_tag, TIBOARD, 0)	\
	MCFG_DEVICE_CONFIG_DATA32( tiboard_config, mode, TI998)

#endif
