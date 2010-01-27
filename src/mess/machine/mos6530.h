/**********************************************************************

    MOS Technology 6530 Memory, I/O, Timer Array emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   Vss   1 |*    \_/     | 40  PA1
                   PA0   2 |             | 39  PA2
                  phi2   3 |             | 38  PA3
                   RS0   4 |             | 37  PA4
                    A9   5 |             | 36  PA5
                    A8   6 |             | 35  PA6
                    A7   7 |             | 34  PA7
                    A6   8 |             | 33  DB0
                   R/W   9 |             | 32  DB1
                    A5  10 |   MCS6530   | 31  DB2
                    A4  11 |             | 30  DB3
                    A3  12 |             | 29  DB4
                    A2  13 |             | 28  DB5
                    A1  14 |             | 27  DB6
                    A0  15 |             | 26  DB7
                  _RES  16 |             | 25  PB0
               IRQ/PB7  17 |             | 24  PB1
               CS1/PB6  18 |             | 23  PB2
               CS2/PB5  19 |             | 22  PB3
                   Vcc  20 |_____________| 21  PB4

**********************************************************************/

#ifndef __MOS6530__
#define __MOS6530__

#include "emu.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define MOS6530 DEVICE_GET_INFO_NAME( mos6530 )

#define MDRV_MOS6530_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD((_tag), MOS6530, _clock)	\
	MDRV_DEVICE_CONFIG(_config)

#define MOS6530_INTERFACE(name) \
	const mos6530_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mos6530_interface mos6530_interface;
struct _mos6530_interface
{
	devcb_write_line		out_irq_func;

	devcb_read8				in_pa_func;
	devcb_write8			out_pa_func;

	devcb_read8				in_pb_func;
	devcb_write8			out_pb_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( mos6530 );

/* register access */
READ8_DEVICE_HANDLER( mos6530_r );
WRITE8_DEVICE_HANDLER( mos6530_w );

/* memory access */
/*
READ8_DEVICE_HANDLER( mos6530_ram_r );
WRITE8_DEVICE_HANDLER( mos6530_ram_w );

READ8_DEVICE_HANDLER( mos6530_rom_r );
*/
#endif
