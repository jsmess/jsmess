/**********************************************************************

    OKI MSM58321RS Real Time Clock/Calendar emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   CS2   1 |*    \_/     | 16  Vdd
                 WRITE   2 |             | 15  XT
                  READ   3 |             | 14  _XT
                    D0   4 | MSM58321RS  | 13  CS1
                    D1   5 |             | 12  TEST
                    D2   6 |             | 11  STOP
                    D3   7 |             | 10  _BUSY
                   GND   8 |_____________| 9   ADDRESS WRITE

**********************************************************************/

#ifndef __MSM58321__
#define __MSM58321__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(MSM58321RS, msm58321rs);

#define MCFG_MSM58321RS_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, MSM58321RS, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)

#define MSM58321_INTERFACE(name) \
	const msm58321_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _msm58321_interface msm58321_interface;
struct _msm58321_interface
{
	devcb_write_line	out_busy_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* register access */
READ8_DEVICE_HANDLER( msm58321_r );
WRITE8_DEVICE_HANDLER( msm58321_w );

/* chip select */
WRITE_LINE_DEVICE_HANDLER( msm58321_cs1_w );
WRITE_LINE_DEVICE_HANDLER( msm58321_cs2_w );

/* handshaking */
READ_LINE_DEVICE_HANDLER( msm58321_busy_r );
WRITE_LINE_DEVICE_HANDLER( msm58321_read_w );
WRITE_LINE_DEVICE_HANDLER( msm58321_write_w );
WRITE_LINE_DEVICE_HANDLER( msm58321_address_write_w );

/* stop */
WRITE_LINE_DEVICE_HANDLER( msm58321_stop_w );

/* test */
WRITE_LINE_DEVICE_HANDLER( msm58321_test_w );

#endif
