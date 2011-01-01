/**********************************************************************

    Intel 8214 Priority Interrupt Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   _B0   1 |*    \_/     | 24  Vcc
                   _B1   2 |             | 23  _ECS
                   _B2   3 |             | 22  _R7
                  _SGS   4 |             | 21  _R6
                  _INT   5 |             | 20  _R5
                  _CLK   6 |    8214     | 19  _R4
                  INTE   7 |             | 18  _R3
                   _A0   8 |             | 17  _R2
                   _A1   9 |             | 16  _R1
                   _A2  10 |             | 15  _R0
                  _ELR  11 |             | 14  ENLG
                   GND  12 |_____________| 13  ETLG

**********************************************************************/

#ifndef __I8214__
#define __I8214__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(I8214, i8214);

#define MCFG_I8214_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, I8214, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)

#define I8214_INTERFACE(name) \
	const i8214_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i8214_interface i8214_interface;
struct _i8214_interface
{
	devcb_write_line	out_int_func;
	devcb_write_line	out_enlg_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* request level read */
READ8_DEVICE_HANDLER( i8214_a_r );

/* current status register write (bit 3 is SGS) */
WRITE8_DEVICE_HANDLER( i8214_b_w );

/* enable this level group */
WRITE_LINE_DEVICE_HANDLER( i8214_etlg_w );

/* interrupt enable */
WRITE_LINE_DEVICE_HANDLER( i8214_inte_w );

/* interrupt requests */
WRITE8_DEVICE_HANDLER( i8214_r_w );

#endif
