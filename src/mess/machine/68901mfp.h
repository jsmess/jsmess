/**********************************************************************

    Motorola MC68901 Multi Function Peripheral emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
							_____   _____
				  R/_W   1 |*    \_/     | 48  _CS
				   RS1   2 |			 | 47  _DS
				   RS2   3 |			 | 46  _DTACK
				   RS3   4 |			 | 45  _IACK
				   RS4   5 |			 | 44  D7
				   RS5   6 |			 | 43  D6
					TC   7 |			 | 42  D5
					SO   8 |			 | 41  D4
					SI   9 |			 | 40  D3
					RC  10 |			 | 39  D2
				   Vcc  11 |			 | 38  D1
					NC  12 |   MC68901	 | 37  D0
				   TAO  13 |   MK68901	 | 36  GND
				   TBO  14 |			 | 35  CLK
				   TCO  15 |			 | 34  _IEI
				   TDO  16 |			 | 33  _IEO
				 XTAL1  17 |			 | 32  _IRQ
				 XTAL2  18 |			 | 31  _RR
				   TAI  19 |			 | 30  _TR
				   TBI  20 |			 | 29  I7
				_RESET	21 |		     | 28  I6
					I0  22 |			 | 27  I5
					I1  23 |			 | 26  I4
					I2  24 |_____________| 25  I3

**********************************************************************/

#ifndef __MC68901__
#define __MC68901__

#include "driver.h"

enum {
	MC68901_TAO_LOOPBACK = -1,
	MC68901_TBO_LOOPBACK = -2,
	MC68901_TCO_LOOPBACK = -3,
	MC68901_TDO_LOOPBACK = -4
};

typedef void (*mc68901_on_irq_changed_func) (const device_config *device, int level);
#define MC68901_ON_IRQ_CHANGED(name) void name(const device_config *device, int level)

typedef void (*mc68901_on_to_changed_func) (const device_config *device, int timer, int level);
#define MC68901_ON_TO_CHANGED(name) void name(const device_config *device, int timer, int level)

typedef UINT8 (*mc68901_gpio_read_func) (const device_config *device);
#define MC68901_GPIO_READ(name) UINT8 name(const device_config *device)

typedef void (*mc68901_gpio_write_func) (const device_config *device, UINT8 data);
#define MC68901_GPIO_WRITE(name) void name(const device_config *device, UINT8 data)

#define MC68901 DEVICE_GET_INFO_NAME( mc68901 )
#define MK68901 DEVICE_GET_INFO_NAME( mk68901 )

/* interface */
typedef struct _mc68901_interface mc68901_interface;
struct _mc68901_interface
{
	int	chip_clock;			/* chip clock */
	int	timer_clock;		/* timer clock */
	int	rx_clock;			/* serial receive clock */
	int	tx_clock;			/* serial transmit clock */

	UINT8 *rx_pin;			/* serial receive pin (pin 9) */
	UINT8 *tx_pin;			/* serial transmit pin (pin 8) */
	
	/* this gets called for each change of the TxO pins (pins 13-16) */
	mc68901_on_to_changed_func	on_to_changed;

	/* this gets called on each change of the interrupt line */
	mc68901_on_irq_changed_func	on_irq_changed;

	/* this is called on each read of the GPIO pins */
	mc68901_gpio_read_func		gpio_r;

	/* this is called on each write of the GPIO pins */
	mc68901_gpio_write_func		gpio_w;
};
#define MC68901_INTERFACE(name) const mc68901_interface (name) =

/* device interface */
DEVICE_GET_INFO( mc68901 );

/* register access */
READ8_DEVICE_HANDLER( mc68901_register_r );
WRITE8_DEVICE_HANDLER( mc68901_register_w );

/* write to Timer A input (pin 19) */
void mc68901_tai_w(const device_config *device, int value);

/* write to Timer B input (pin 20) */
void mc68901_tbi_w(const device_config *device, int value);

/* get interrupt vector */
int mc68901_get_vector(const device_config *device);

#endif
