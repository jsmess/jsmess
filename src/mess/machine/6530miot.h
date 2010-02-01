/**********************************************************************

    MOS Technology 6530 Memory, I/O, Timer Array emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   Vss   1 |*    \_/     | 40  PA1
                   PA0   2 |             | 39  PA2
                  phi2   3 |             | 38  PA3
                   RS0   4 |             | 37  PA4
                    A9   5 |             | 36  PA5
                    A8   6 |             | 35  PA6
                    A7   7 |             | 34  PA7
                    A6   8 |             | 33  DB0
                   R/W   9 |             | 32  DB1
                    A5  10 |   MCS6530   | 31  DB2
                    A4  11 |             | 30  DB3
                    A3  12 |             | 29  DB4
                    A2  13 |             | 28  DB5
                    A1  14 |             | 27  DB6
                    A0  15 |             | 26  DB7
                  _RES  16 |             | 25  PB0
               IRQ/PB7  17 |             | 24  PB1
               CS1/PB6  18 |             | 23  PB2
               CS2/PB5  19 |             | 22  PB3
                   Vcc  20 |_____________| 21  PB4

**********************************************************************/

#ifndef __MIOT6530_H__
#define __MIOT6530_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef UINT8 (*miot_read_func)(running_device *device, UINT8 olddata);
typedef void (*miot_write_func)(running_device *device, UINT8 newdata, UINT8 olddata);


typedef struct _miot6530_interface miot6530_interface;
struct _miot6530_interface
{
	miot_read_func		in_a_func;
	miot_read_func		in_b_func;
	miot_write_func		out_a_func;
	miot_write_func		out_b_func;
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_MIOT6530_ADD(_tag, _clock, _intrf) \
	MDRV_DEVICE_ADD(_tag, MIOT6530, _clock) \
	MDRV_DEVICE_CONFIG(_intrf)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ8_DEVICE_HANDLER( miot6530_r );
WRITE8_DEVICE_HANDLER( miot6530_w );

void miot6530_porta_in_set(running_device *device, UINT8 data, UINT8 mask);
void miot6530_portb_in_set(running_device *device, UINT8 data, UINT8 mask);

UINT8 miot6530_porta_in_get(running_device *device);
UINT8 miot6530_portb_in_get(running_device *device);

UINT8 miot6530_porta_out_get(running_device *device);
UINT8 miot6530_portb_out_get(running_device *device);


/* ----- device interface ----- */

#define MIOT6530 DEVICE_GET_INFO_NAME(miot6530)
DEVICE_GET_INFO( miot6530 );

#endif
