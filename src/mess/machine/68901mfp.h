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
				   TAO  13 |			 | 36  GND
				   TBO  14 |			 | 35  CLK
				   TCO  15 |			 | 34  _IEI
				   TDO  16 |			 | 33  _IEO
				 XTAL1  17 |			 | 32  _IRQ
				 XTAL2  18 |			 | 31  _RR
				   TAI  19 |			 | 30  _TR
				   TBI  10 |			 | 29  I7
				_RESET	21 |		     | 28  I6
					I0  22 |			 | 27  I5
					I1  23 |			 | 26  I4
					I2  24 |_____________| 25  I3

**********************************************************************/

#ifndef __MC68901__
#define __MC68901__

#include "driver.h"

typedef struct _mc68901_t mc68901_t;
typedef struct _mc68901_interface mc68901_interface;

#define MC68901_TAO_LOOPBACK	-1
#define MC68901_TBO_LOOPBACK	-2
#define MC68901_TCO_LOOPBACK	-3
#define MC68901_TDO_LOOPBACK	-4

#define MC68901 DEVICE_GET_INFO_NAME( mc68901 )

/* interface */
struct _mc68901_interface
{
	int	chip_clock;
	int	timer_clock;
	int	rx_clock;
	int	tx_clock;

	UINT8 *rx_pin, *tx_pin;

	void (*to_w)(mc68901_t *chip, int timer, int value);

	void (*irq_callback)(mc68901_t *chip, int state);

	read8_handler gpio_r;
	write8_handler gpio_w;
};

/* device interface */
DEVICE_GET_INFO( mc68901 );

/* read from MFP register */
UINT8 mc68901_register_r(mc68901_t *chip, int reg);
/* write to MFP register */
void mc68901_register_w(mc68901_t *chip, int reg, UINT8 data);

/* write to MFP Timer A input */
void mc68901_tai_w(mc68901_t *chip, int value);
/* write to MFP Timer B input */
void mc68901_tbi_w(mc68901_t *chip, int value);

/* get MFP interrupt vector */
int mc68901_get_vector(mc68901_t *chip);

#endif
