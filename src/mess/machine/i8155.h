/**********************************************************************

    Intel 8155 - 2048-Bit Static MOS RAM with I/O Ports and Timer emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
							_____   _____
				   PC3   1 |*    \_/     | 40  Vcc
				   PC4   2 |			 | 39  PC2
			  TIMER IN   3 |			 | 38  PC1
				 RESET   4 |			 | 37  PC0
				   PC5   5 |			 | 36  PB7
			_TIMER OUT   6 |			 | 35  PB6
				 IO/_M   7 |			 | 34  PB5
			 CE or _CE   8 |			 | 33  PB4
				   _RD   9 |			 | 32  PB3
				   _WR  10 |	8155	 | 31  PB2
				   ALE  11 |	8156	 | 30  PB1
				   AD0  12 |			 | 29  PB0
				   AD1  13 |			 | 28  PA7
				   AD2  14 |			 | 27  PA6
				   AD3  15 |			 | 26  PA5
				   AD4  16 |			 | 25  PA4
				   AD5  17 |			 | 24  PA3
				   AD6  18 |			 | 23  PA2
				   AD7  19 |			 | 22  PA1
				   Vss  20 |_____________| 21  PA0

**********************************************************************/

#ifndef __I8155__
#define __I8155__

#include "driver.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define I8155 DEVICE_GET_INFO_NAME( i8155 )

#define MDRV_I8155_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD((_tag), I8155, _clock)	\
	MDRV_DEVICE_CONFIG(_config)

#define I8155_INTERFACE(name) \
	const i8155_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i8155_interface i8155_interface;
struct _i8155_interface
{
	devcb_read8				in_pa_func;
	devcb_read8				in_pb_func;
	devcb_read8				in_pc_func;

	devcb_write8			out_pa_func;
	devcb_write8			out_pb_func;
	devcb_write8			out_pc_func;

	/* this gets called for each change of the TIMER OUT pin (pin 6) */
	devcb_write_line		out_to_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( i8155 );

/* register access */
READ8_DEVICE_HANDLER( i8155_r );
WRITE8_DEVICE_HANDLER( i8155_w );

/* memory access */
READ8_DEVICE_HANDLER( i8155_ram_r );
WRITE8_DEVICE_HANDLER( i8155_ram_w );

#endif
