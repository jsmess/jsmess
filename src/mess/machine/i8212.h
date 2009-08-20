/**********************************************************************

    Intel 8212 8-Bit Input/Output Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
							_____   _____
				  _DS1	 1 |*    \_/     | 24  Vcc
				    MD	 2 |			 | 23  _INT
				   DI1	 3 |			 | 22  DI8
				   DO1	 4 |			 | 21  DO8
				   DI2	 5 |			 | 20  DI7
				   DO2	 6 |	8212	 | 19  DO7
				   DI3	 7 |			 | 18  DI6
				   DO3	 8 |			 | 17  DO6
				   DI4	 9 |			 | 16  DI5
				   DO4	10 |			 | 15  DO5
				   STB	11 |			 | 14  DI4
				   GND  12 |_____________| 13  DO4

**********************************************************************/

#ifndef __I8212__
#define __I8212__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define I8212		DEVICE_GET_INFO_NAME(i8212)

#define MDRV_I8212_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, I8212, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define I8212_INTERFACE(name) \
	const i8212_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

enum
{
	I8212_MODE_INPUT = 0,
	I8212_MODE_OUTPUT
};

typedef struct _i8212_interface i8212_interface;
struct _i8212_interface
{
	devcb_write_line	out_int_func;

	devcb_read8			in_di_func;
	devcb_write8		out_do_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( i8212 );

/* data latch access */
READ8_DEVICE_HANDLER( i8212_r );
WRITE8_DEVICE_HANDLER( i8212_w );

/* mode */
WRITE_LINE_DEVICE_HANDLER( i8212_mode_w );

/* strobe */
WRITE_LINE_DEVICE_HANDLER( i8212_stb_w );

#endif
