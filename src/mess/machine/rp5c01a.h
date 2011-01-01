/**********************************************************************

    Ricoh RP5C01A Real Time Clock With Internal RAM emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   _CS   1 |*    \_/     | 18  Vcc
                    CS   2 |             | 17  OSCOUT
                   ADJ   3 |             | 16  OSCIN
                    A0   4 |             | 15  _ALARM
                    A1   5 |   RP5C01A   | 14  D3
                    A2   6 |             | 13  D2
                    A3   7 |             | 12  D1
                   _RD   8 |             | 11  D0
                   GND   9 |_____________| 10  _WR

**********************************************************************/

#ifndef __RP5C01A__
#define __RP5C01A__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(RP5C01A, rp5c01a);

#define MCFG_RP5C01A_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, RP5C01A, _clock) \
	MCFG_DEVICE_CONFIG(_config)

#define MCFG_RP5C01A_REMOVE(_tag) \
	MCFG_DEVICE_REMOVE(_tag)

#define RP5C01A_INTERFACE(name) \
	const rp5c01a_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _rp5c01a_interface rp5c01a_interface;
struct _rp5c01a_interface
{
	/* this gets called for every change of the _ALARM pin (pin 15) */
	devcb_write_line		out_alarm_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* adjust */
WRITE_LINE_DEVICE_HANDLER( rp5c01a_adj_w );

/* register access */
READ8_DEVICE_HANDLER( rp5c01a_r );
WRITE8_DEVICE_HANDLER( rp5c01a_w );

#endif /* __RP5C01A__ */
