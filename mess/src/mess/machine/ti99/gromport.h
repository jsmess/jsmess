/*********************************************************************

    gromport.h

    TI99 family cartridge management

*********************************************************************/

#ifndef __TI99CART_H__
#define __TI99CART_H__

#include "imagedev/cartslot.h"
#include "ti99defs.h"

typedef struct _ti99_multicart_config
{
	write_line_device_func	ready;

} ti99_multicart_config;

typedef struct _gromport_callback
{
	devcb_write_line ready;
} gromport_callback;

void set_gk_switches(device_t *cartsys, int number, int value);

/* GROM part */
WRITE8_DEVICE_HANDLER( gromportg_w );
READ8Z_DEVICE_HANDLER( gromportg_rz );

/* ROM part */
WRITE8_DEVICE_HANDLER( gromportr_w );
READ8Z_DEVICE_HANDLER( gromportr_rz );

/* CRU part */
WRITE8_DEVICE_HANDLER( gromportc_w );
READ8Z_DEVICE_HANDLER( gromportc_rz );

DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_MULTICART, ti99_multicart);

#define MCFG_TI99_GROMPORT_ADD(_tag, _ready) \
	MCFG_DEVICE_ADD(_tag, TI99_MULTICART, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(ti99_multicart_config, ready, _ready)

#endif /* __TI99CART_H__ */
