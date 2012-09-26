#ifndef __GENBOARD__
#define __GENBOARD__

#include "machine/tms9901.h"

enum
{
	MDIP1 = 0x01,
	MDIP2 = 0x02,
	MDIP3 = 0x04,
	MDIP4 = 0x08,
	MDIP5 = 0x10,
	MDIP6 = 0x20,
	MDIP7 = 0x40,
	MDIP8 = 0x80
};

#define GENMOD 0x01

READ8_DEVICE_HANDLER( geneve_r );
WRITE8_DEVICE_HANDLER( geneve_w );
READ8_DEVICE_HANDLER( geneve_cru_r );
WRITE8_DEVICE_HANDLER( geneve_cru_w );

extern const tms9901_interface tms9901_wiring_geneve;
void tms9901_gen_set_int2(running_machine &machine, int state);
void set_gm_switches(device_t *board, int number, int value);

INTERRUPT_GEN( geneve_hblank_interrupt );

/* device interface */
DECLARE_LEGACY_DEVICE( GENBOARD, genboard );

WRITE_LINE_DEVICE_HANDLER( board_inta );
WRITE_LINE_DEVICE_HANDLER( board_intb );
WRITE_LINE_DEVICE_HANDLER( board_ready );

enum
{
	GENEVE_098 = 0,
	GENEVE_100 = 1
};

#define MCFG_GENEVE_BOARD_ADD(_tag )			\
	MCFG_DEVICE_ADD(_tag, GENBOARD, 0)

#endif
