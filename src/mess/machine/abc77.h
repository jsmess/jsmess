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

#include "driver.h"

#define ABC77_TAG	"abc77"
#define I8035_TAG	"z16"

#define ABC77 DEVICE_GET_INFO_NAME( abc77 )

typedef void (*abc77_on_txd_changed_func) (const device_config *device, int level);
#define ABC77_ON_TXD_CHANGED(name) void name(const device_config *device, int level)

typedef void (*abc77_on_keydown_changed_func) (const device_config *device, int level);
#define ABC77_ON_KEYDOWN_CHANGED(name) void name(const device_config *device, int level)

typedef void (*abc77_on_clock_changed_func) (const device_config *device, int level);
#define ABC77_ON_CLOCK_CHANGED(name) void name(const device_config *device, int level)

/* interface */
typedef struct _abc77_interface abc77_interface;
struct _abc77_interface
{
	/* this gets called for every change of the TxD pin (pin 1) */
	abc77_on_txd_changed_func		txd_w;

	/* this gets called for every change of the CLOCK pin (pin 4) */
	abc77_on_clock_changed_func		clock_w;

	/* this gets called for every change of the _KEYDOWN pin (pin 5) */
	abc77_on_keydown_changed_func	keydown_w;
};
#define ABC77_INTERFACE(name) const abc77_interface (name) =

/* device interface */
DEVICE_GET_INFO( abc77 );

/* keyboard matrix */
INPUT_PORTS_EXTERN( abc77 );

/* receive data */
void abc77_rxd_w(const device_config *device, int level);

/* reset */
void abc77_reset_w(const device_config *device, int level);

#endif
