/**********************************************************************

    Motorola MC68901 Multi Function Peripheral emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                  R/_W   1 |*    \_/     | 48  _CS
                   RS1   2 |             | 47  _DS
                   RS2   3 |             | 46  _DTACK
                   RS3   4 |             | 45  _IACK
                   RS4   5 |             | 44  D7
                   RS5   6 |             | 43  D6
                    TC   7 |             | 42  D5
                    SO   8 |             | 41  D4
                    SI   9 |             | 40  D3
                    RC  10 |             | 39  D2
                   Vcc  11 |             | 38  D1
                    NC  12 |   MC68901   | 37  D0
                   TAO  13 |   MK68901   | 36  GND
                   TBO  14 |             | 35  CLK
                   TCO  15 |             | 34  _IEI
                   TDO  16 |             | 33  _IEO
                 XTAL1  17 |             | 32  _IRQ
                 XTAL2  18 |             | 31  _RR
                   TAI  19 |             | 30  _TR
                   TBI  20 |             | 29  I7
                _RESET  21 |             | 28  I6
                    I0  22 |             | 27  I5
                    I1  23 |             | 26  I4
                    I2  24 |_____________| 25  I3

**********************************************************************/

#ifndef __MC68901__
#define __MC68901__

#include "driver.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define MC68901 DEVICE_GET_INFO_NAME( mc68901 )
#define MK68901 DEVICE_GET_INFO_NAME( mk68901 )

#define MDRV_MC68901_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD((_tag), MC68901, _clock)	\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_MK68901_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD((_tag), MK68901, _clock)	\
	MDRV_DEVICE_CONFIG(_config)

#define MC68901_INTERFACE(name) const mc68901_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mc68901_interface mc68901_interface;
struct _mc68901_interface
{
	int	timer_clock;		/* timer clock */
	int	rx_clock;			/* serial receive clock */
	int	tx_clock;			/* serial transmit clock */

	/* this is called on each read of the GPIO pins */
	devcb_read8				in_gpio_func;

	/* this is called on each write of the GPIO pins */
	devcb_write8			out_gpio_func;

	/* this gets called for each read of the SI pin (pin 9) */
	devcb_read_line			in_si_func;

	/* this gets called for each change of the SO pin (pin 8) */
	devcb_write_line		out_so_func;

	/* this gets called for each change of the TAO pin (pin 13) */
	devcb_write_line		out_tao_func;

	/* this gets called for each change of the TBO pin (pin 14) */
	devcb_write_line		out_tbo_func;

	/* this gets called for each change of the TCO pin (pin 15) */
	devcb_write_line		out_tco_func;

	/* this gets called for each change of the TDO pin (pin 16) */
	devcb_write_line		out_tdo_func;

	/* this gets called on each change of the interrupt line */
	devcb_write_line		out_irq_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( mc68901 );

/* register access */
READ8_DEVICE_HANDLER( mc68901_register_r );
WRITE8_DEVICE_HANDLER( mc68901_register_w );

/* get interrupt vector */
int mc68901_get_vector(const device_config *device) ATTR_NONNULL(1);

/* write to Timer A input (pin 19) */
WRITE_LINE_DEVICE_HANDLER( mc68901_tai_w ) ATTR_NONNULL(1);

/* write to Timer B input (pin 20) */
WRITE_LINE_DEVICE_HANDLER( mc68901_tbi_w ) ATTR_NONNULL(1);

/* receive clock */
WRITE_LINE_DEVICE_HANDLER( mc68901_rx_clock_w ) ATTR_NONNULL(1);

/* transmit clock */
WRITE_LINE_DEVICE_HANDLER( mc68901_tx_clock_w ) ATTR_NONNULL(1);

#endif
