/***************************************************************************

    Z80 DART (Z8470) implementation

    Copyright (c) 2007, The MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __Z80DART_H_
#define __Z80DART_H_


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _z80dart_interface z80dart_interface;
struct _z80dart_interface
{
	const char *cpu;
	int baseclock;
	void (*irq_cb)(const device_config *device, int state);
	write8_device_func dtr_changed_cb;
	write8_device_func rts_changed_cb;
	write8_device_func break_changed_cb;
	write8_device_func transmit_cb;
	int (*receive_poll_cb)(const device_config *device, int ch);
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_Z80DART_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, Z80DART) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_Z80DART_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag, Z80DART)



/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

WRITE8_DEVICE_HANDLER( z80dart_c_w );
READ8_DEVICE_HANDLER( z80dart_c_r );



/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

WRITE8_DEVICE_HANDLER( z80dart_d_w );
READ8_DEVICE_HANDLER( z80dart_d_r );



/***************************************************************************
    CONTROL LINE READ/WRITE
***************************************************************************/

int z80dart_get_dtr(const device_config *device, int ch);
int z80dart_get_rts(const device_config *device, int ch);
void z80dart_set_cts(const device_config *device, int ch, int state);
void z80dart_set_dcd(const device_config *device, int ch, int state);
void z80dart_set_ri(const device_config *device, int ch, int state);
void z80dart_receive_data(const device_config *device, int ch, UINT8 data);


/* ----- device interface ----- */

#define Z80DART DEVICE_GET_INFO_NAME(z80dart)
DEVICE_GET_INFO( z80dart );

#endif
