/**********************************************************************

    General Instruments AY-5-3600 Keyboard Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                         1 |*    \_/     | 40  X0
                         2 |             | 39  X1
                         3 |             | 38  X2
                         4 |             | 37  X3
                         5 |             | 36  X4
                    B9   6 |             | 35  X5
                    B8   7 |             | 34  X6
                    B7   8 |             | 33  X7
                    B6   9 |             | 32  X8
                    B5  10 |  AY-5-3600  | 31  DELAY NODE
                    B4  11 |             | 30  Vcc
                    B3  12 |             | 29  SHIFT
                    B2  13 |             | 28  CONTROL
                    B1  14 |             | 27  Vgg
                   Vdd  15 |             | 26  Y9
            DATA READY  16 |             | 25  Y8
                    Y0  17 |             | 24  Y7
                    Y1  18 |             | 23  Y6
                    Y2  19 |             | 22  Y5
                    Y3  20 |_____________| 21  Y4

                            _____   _____
                   Vcc   1 |*    \_/     | 40  Vss
                    B9   2 |             | 39  Vgg
                    B8   3 |             | 38  _STCL?
                    B7   4 |             | 37  _MCLR
                  TEST   5 |             | 36  OSC
                    B6   6 |             | 35  CLK OUT
                    B5   7 |             | 34  X7
                    B4   8 |             | 33  X6
                    B3   9 |             | 32  X5
                    B2  10 |  AY-5-3600  | 31  X4
                    B1  11 |   PRO 002   | 30  X3
                    X8  12 |             | 29  X2
                   AKO  13 |             | 28  X1
                  CTRL  14 |             | 27  X0
                 SHIFT  15 |             | 26  Y9
            DATA READY  16 |             | 25  Y8
                    Y0  17 |             | 24  Y7
                    Y1  18 |             | 23  Y6
                    Y2  19 |             | 22  Y5
                    Y3  20 |_____________| 21  Y4

**********************************************************************/

#ifndef __AY3600__
#define __AY3600__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(AY3600PRO002, ay3600pro002);

#define MCFG_AY3600PRO002_ADD(_tag, _intf) \
	MCFG_DEVICE_ADD(_tag, AY3600PRO002, 0) \
	MCFG_DEVICE_CONFIG(_intf)

#define AY3600_INTERFACE(name) \
	const ay3600_interface (name) =

typedef UINT16 (*ay3600_y_r)(device_t *device, int x);
#define AY3600_Y_READ(name) UINT16 name(device_t *device, int x)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ay3600_interface ay3600_interface;
struct _ay3600_interface
{
	ay3600_y_r			in_y_func;

	devcb_read_line		in_shift_func;
	devcb_read_line		in_control_func;

	/* this gets called for every change of the DATA READY pin */
	devcb_write_line	out_data_ready_func;

	/* this gets called for every change of the AKO pin */
	devcb_write_line	out_ako_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* data read */
UINT16 ay3600_b_r(device_t *device);

#endif
