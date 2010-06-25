/**********************************************************************

    RCA CDP1871 Keyboard Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                    D1   1 |*    \_/     | 40  Vdd
                    D2   2 |             | 39  SHIFT
                    D3   3 |             | 38  CONTROL
                    D4   4 |             | 37  ALPHA
                    D5   5 |             | 36  DEBOUNCE
                    D6   6 |             | 35  _RTP
                    D7   7 |             | 34  TPB
                    D8   8 |             | 33  _DA
                    D9   9 |             | 32  BUS 7
                   D10  10 |   CDP1871   | 31  BUS 6
                   D11  11 |             | 30  BUS 5
                    S1  12 |             | 29  BUS 4
                    S2  13 |             | 28  BUS 3
                    S3  14 |             | 27  BUS 2
                    S4  15 |             | 26  BUS 1
                    S5  16 |             | 25  BUS 0
                    S6  17 |             | 24  CS4
                    S7  18 |             | 23  CS3
                    S8  19 |             | 22  CS2
                   Vss  20 |_____________| 21  _CS1

**********************************************************************/

#ifndef __CDP1871__
#define __CDP1871__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(CDP1871, cdp1871);

#define MDRV_CDP1871_ADD(_tag, _intrf, _clock) \
	MDRV_DEVICE_ADD(_tag, CDP1871, _clock) \
	MDRV_DEVICE_CONFIG(_intrf)

#define CDP1871_INTERFACE(name) \
	const cdp1871_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cdp1871_interface cdp1871_interface;
struct _cdp1871_interface
{
	devcb_read8			in_d1_func;
	devcb_read8			in_d2_func;
	devcb_read8			in_d3_func;
	devcb_read8			in_d4_func;
	devcb_read8			in_d5_func;
	devcb_read8			in_d6_func;
	devcb_read8			in_d7_func;
	devcb_read8			in_d8_func;
	devcb_read8			in_d9_func;
	devcb_read8			in_d10_func;
	devcb_read8			in_d11_func;

	devcb_read_line		in_shift_func;
	devcb_read_line		in_control_func;
	devcb_read_line		in_alpha_func;

	/* this gets called for every change of the DA pin (pin 33) */
	devcb_write_line	out_da_func;

	/* this gets called for every change of the RPT pin (pin 35) */
	devcb_write_line	out_rpt_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* keyboard data */
READ8_DEVICE_HANDLER( cdp1871_data_r );

#endif
