/**********************************************************************

    Commodore 64H165 Gate Array emulation

    Used in 1541B/1541C/1541-II/1571

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                  TEST   1 |*    \_/     | 40  _BYTE
                   YB0   2 |             | 39  SOE
                   YB1   3 |             | 38  B
                   YB2   4 |             | 37  CK
                   YB3   5 |             | 36  _QX
                   YB4   6 |             | 35  Q
                   YB5   7 |             | 34  R/_W
                   YB6   8 |             | 33  LOCK
                   YB7   9 |             | 32  PLL
                   Vss  10 |  64H156-01  | 31  CLR
                  STP1  11 |  251828-01  | 30  Vcc
                  STP0  12 |             | 29  _XRW
                   MTR  13 |             | 28  Y3
                    _A  14 |             | 27  Y2
                   DS0  15 |             | 26  Y1
                   DS1  16 |             | 25  Y0
                 _SYNC  17 |             | 24  ATN
                   TED  18 |             | 23  ATNI
                    OE  19 |             | 22  ATNA
                 _ACCL  20 |_____________| 21  OSC

                            _____   _____
                  TEST   1 |*    \_/     | 42  _BYTE
                   YB0   2 |             | 41  SOE
                   YB1   3 |             | 40  B
                   YB2   4 |             | 39  CK
                   YB3   5 |             | 38  _QX
                   YB4   6 |             | 37  Q
                   YB5   7 |             | 36  R/_W
                   YB6   8 |             | 35  LOCK
                   YB7   9 |             | 34  PLL
                   Vss  10 |  64H156-02  | 33  CLR
                  STP1  11 |  251828-02  | 32  Vcc
                  STP0  12 |             | 31  _XRW
                   MTR  13 |             | 30  Y3
                    _A  14 |             | 29  Y2
                   DS0  15 |             | 28  Y1
                   DS1  16 |             | 27  Y0
                 _SYNC  17 |             | 26  ATN
                   TED  18 |             | 25  ATNI
                    OE  19 |             | 24  ATNA
                 _ACCL  20 |             | 23  ATNA
                   Vcc  21 |_____________| 22  Vss

**********************************************************************/

#ifndef __64H156__
#define __64H156__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C64H156, c64h156);

#define MCFG_64H156_ADD(_tag, _clock, _intrf) \
	MCFG_DEVICE_ADD(_tag, C64H156, _clock) \
	MCFG_DEVICE_CONFIG(_intrf)

#define C64H156_INTERFACE(name) \
	const c64h156_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c64h156_interface c64h156_interface;
struct _c64h156_interface
{
	devcb_write_line	out_atn_func;
	devcb_write_line	out_sync_func;
	devcb_write_line	out_byte_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* parallel data */
READ8_DEVICE_HANDLER( c64h156_yb_r );
WRITE8_DEVICE_HANDLER( c64h156_yb_w );

/* sync mark detected */
READ_LINE_DEVICE_HANDLER( c64h156_sync_r );

/* byte ready */
READ_LINE_DEVICE_HANDLER( c64h156_byte_r );

/* stepper driver */
void c64h156_stp_w(device_t *device, int data);

/* stepper motor */
WRITE_LINE_DEVICE_HANDLER( c64h156_mtr_w );

/* density select */
void c64h156_ds_w(device_t *device, int data);

/* */
WRITE_LINE_DEVICE_HANDLER( c64h156_ted_w );

/* mode select */
WRITE_LINE_DEVICE_HANDLER( c64h156_oe_w );

/* byte input enable */
WRITE_LINE_DEVICE_HANDLER( c64h156_soe_w );

/* attention input */
WRITE_LINE_DEVICE_HANDLER( c64h156_atni_w );

/* attention acknowledge */
WRITE_LINE_DEVICE_HANDLER( c64h156_atna_w );

#endif
