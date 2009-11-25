/*

    Keyboard connector pinout

    pin     description     connected to
    -------------------------------------------
    1       TxD             i8035 pin 36 (P25)
    2       GND
    3       RxD             i8035 pin 6 (_INT)
    4       CLOCK           i8035 pin 39 (T1)
    5       _KEYDOWN        i8035 pin 37 (P26)
    6       +12V
    7       _RESET          NE556 pin 8 (2TRIG)

*/

#ifndef __ABC77__
#define __ABC77__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define ABC77_TAG	"abc77"

#define ABC77 DEVICE_GET_INFO_NAME( abc77 )

#define MDRV_ABC77_ADD(_config) \
	MDRV_DEVICE_ADD(ABC77_TAG, ABC77, 0)\
	MDRV_DEVICE_CONFIG(_config)

#define ABC77_INTERFACE(_name) \
	const abc77_interface (_name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _abc77_interface abc77_interface;
struct _abc77_interface
{
	/* this gets called for every change of the TxD pin (pin 1) */
	devcb_write_line	out_txd_func;

	/* this gets called for every change of the CLOCK pin (pin 4) */
	devcb_write_line	out_clock_func;

	/* this gets called for every change of the _KEYDOWN pin (pin 5) */
	devcb_write_line	out_keydown_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( abc77 );

/* keyboard matrix */
INPUT_PORTS_EXTERN( abc77 );

/* receive data */
WRITE_LINE_DEVICE_HANDLER( abc77_rxd_w );

/* transmit data */
READ_LINE_DEVICE_HANDLER( abc77_txd_r );

/* reset */
WRITE_LINE_DEVICE_HANDLER( abc77_reset_w );

#endif
