/**********************************************************************

    Intel 8355 - 16,384-Bit ROM with I/O emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                  _CE1   1 |*    \_/     | 40  Vcc
                   CE2   2 |             | 39  PB7
                   CLK   3 |             | 38  PB6
                 RESET   4 |             | 37  PB5
                  N.C.   5 |             | 36  PB4
                 READY   6 |             | 35  PB3
                 IO/_M   7 |             | 34  PB2
                  _IOR   8 |             | 33  PB1
                   _RD   9 |             | 32  PB0
                  _IOW  10 |    8355     | 31  PA7
                   ALE  11 |    8355-2   | 30  PA6
                   AD0  12 |             | 29  PA5
                   AD1  13 |             | 28  PA4
                   AD2  14 |             | 27  PA3
                   AD3  15 |             | 26  PA2
                   AD4  16 |             | 25  PA1
                   AD5  17 |             | 24  PA0
                   AD6  18 |             | 23  A10
                   AD7  19 |             | 22  A9
                   Vss  20 |_____________| 21  A8

**********************************************************************/

#ifndef __I8355__
#define __I8355__

#include "driver.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define I8355 DEVICE_GET_INFO_NAME( i8355 )

#define MDRV_I8355_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD((_tag), I8355, _clock)	\
	MDRV_DEVICE_CONFIG(_config)

#define I8355_INTERFACE(name) \
	const i8355_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i8355_interface i8355_interface;
struct _i8355_interface
{
	devcb_read8				in_pa_func;
	devcb_read8				in_pb_func;

	devcb_write8			out_pa_func;
	devcb_write8			out_pb_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( i8355 );

/* register access */
READ8_DEVICE_HANDLER( i8355_r );
WRITE8_DEVICE_HANDLER( i8355_w );

/* memory access */
READ8_DEVICE_HANDLER( i8355_rom_r );

#endif
