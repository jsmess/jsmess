/***************************************************************************

    Z80 PIO (Z8420) implementation

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************
							_____   _____
					D2	 1 |*    \_/     | 40  D3
					D7	 2 |			 | 39  D4
					D6	 3 |			 | 38  D5
				   _CE	 4 |			 | 37  _M1
				  C/_D	 5 |			 | 36  _IORQ
				  B/_A	 6 |			 | 35  RD
				   PA7	 7 |			 | 34  PB7
				   PA6	 8 |			 | 33  PB6
				   PA5	 9 |			 | 32  PB5
				   PA4	10 |   Z80-PIO	 | 31  PB4
				   GND	11 |			 | 30  PB3
				   PA3	12 |			 | 29  PB2
				   PA2	13 |			 | 28  PB1
				   PA1	14 |			 | 27  PB0
				   PA0	15 |			 | 26  +5V
				 _ASTB	16 |			 | 25  CLK
				 _BSTB	17 |			 | 24  IEI
				  ARDY	18 |			 | 23  _INT
					D0	19 |			 | 22  IEO
					D1	20 |_____________| 21  BRDY

***************************************************************************/

#ifndef __Z80PIO_H__
#define __Z80PIO_H__

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*z80pio_on_int_changed_func) (const device_config *device, int state);
#define Z80PIO_ON_INT_CHANGED(name) void name(const device_config *device, int state)

typedef void (*z80pio_on_ardy_changed_func) (const device_config *device, int state);
#define Z80PIO_ON_ARDY_CHANGED(name) void name(const device_config *device, int state)

typedef void (*z80pio_on_brdy_changed_func) (const device_config *device, int state);
#define Z80PIO_ON_BRDY_CHANGED(name) void name(const device_config *device, int state)

typedef void (*z80pio_on_bstb_changed_func) (const device_config *device, int state);
#define Z80PIO_ON_BSTB_CHANGED(name) void name(const device_config *device, int state)

typedef struct _z80pio_interface z80pio_interface;
struct _z80pio_interface
{
	const char *cpu;				/* CPU whose clock we use for our base */
	int clock;						/* clock (pin 25) of the chip */

	/* this gets called for every change of the _INT pin (pin 23) */
	z80pio_on_int_changed_func		on_int_changed;

	/* this gets called for every read from port A */
	read8_device_func				port_a_r;

	/* this gets called for every read from port B */
	read8_device_func				port_b_r;

	/* this gets called for every write to port A */
	write8_device_func				port_a_w;

	/* this gets called for every write to port B */
	write8_device_func				port_b_w;

	/* this gets called for every change of the ARDY pin (pin 18) */
	z80pio_on_ardy_changed_func		on_ardy_changed;

	/* this gets called for every change of the BRDY pin (pin 21) */
	z80pio_on_brdy_changed_func		on_brdy_changed;
};
#define Z80PIO_INTERFACE(name) const z80pio_interface (name)=

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_Z80PIO_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, Z80PIO) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_Z80PIO_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag, Z80PIO)

/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

READ8_DEVICE_HANDLER( z80pio_c_r );
WRITE8_DEVICE_HANDLER( z80pio_c_w );

/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

READ8_DEVICE_HANDLER( z80pio_d_r );
WRITE8_DEVICE_HANDLER( z80pio_d_w );

/***************************************************************************
    STROBE STATE MANAGEMENT
***************************************************************************/

void z80pio_astb_w(const device_config *device, int state);
void z80pio_bstb_w(const device_config *device, int state);

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

READ8_DEVICE_HANDLER(z80pio_r);
WRITE8_DEVICE_HANDLER(z80pio_w);

/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

#define Z80PIO DEVICE_GET_INFO_NAME(z80pio)
DEVICE_GET_INFO( z80pio );

#endif
