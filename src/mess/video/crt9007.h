/**********************************************************************

    SMC CRT9007 CRT Video Processor and Controller VPAC emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   VA2   1 |*    \_/     | 40  GND
                  VA10   2 |             | 39  VA9
                   VA3   3 |             | 38  VA1
                  VA11   4 |             | 37  VA8
                  VA12   5 |             | 36  VA0
                   VA4   6 |             | 35  CBLANK
                  VA13   7 |             | 34  CURS
                   VA5   8 |             | 33  ACK/_TSC
                   VA6   9 |             | 32  _CSYNC/LPSTB
                   VA7  10 |   CRT9007   | 31  SLD/SL0
                   VLT  11 |             | 30  _SLG/SL1
                   _VS  12 |             | 29  WBEN/SL2/_CSYNC
                   _HS  13 |             | 28  DMAR/SL3/VBLANK
                 _CCLK  14 |             | 27  INT
                  _DRB  15 |             | 26  _RST
                   VD7  16 |             | 25  _CS
                   VD6  17 |             | 24  VD0
                   VD5  18 |             | 23  VD1
                   VD4  19 |             | 22  VD2
                   VD3  20 |_____________| 21  +5V

**********************************************************************/

#ifndef __CRT9007__
#define __CRT9007__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CRT9007 DEVICE_GET_INFO_NAME( crt9007 )

#define MDRV_CRT9007_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD((_tag), CRT9007, _clock)	\
	MDRV_DEVICE_CONFIG(_config)

#define CRT9007_INTERFACE(name) \
	const crt9007_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _crt9007_interface crt9007_interface;
struct _crt9007_interface
{
	devcb_write_line		out_int_func;

	devcb_write_line		out_hs_func;
	devcb_write_line		out_vs_func;

	devcb_read8				in_vd_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( crt9007 );

/* register access */
READ8_DEVICE_HANDLER( crt9007_r );
WRITE8_DEVICE_HANDLER( crt9007_w );

/* screen update */
void crt9007_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
