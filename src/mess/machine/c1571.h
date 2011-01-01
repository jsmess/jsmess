/**********************************************************************

    Commodore 1570/1571/1571CR Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1571__
#define __C1571__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C1570,  c1570);
DECLARE_LEGACY_DEVICE(C1571, c1571);
DECLARE_LEGACY_DEVICE(C1571CR, c1571cr);

#define MCFG_C1570_ADD(_tag, _serial_bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1570, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1571_config, serial_bus_tag, _serial_bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1571_config, address, _address)

#define MCFG_C1571_ADD(_tag, _serial_bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1571, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1571_config, serial_bus_tag, _serial_bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1571_config, address, _address)

#define MCFG_C1571CR_ADD(_tag, _serial_bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1571CR, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1571_config, serial_bus_tag, _serial_bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1571_config, address, _address)

#define C1571_IEC(_tag) \
	_tag, DEVCB_DEVICE_LINE(_tag, c1571_iec_srq_w), DEVCB_DEVICE_LINE(_tag, c1571_iec_atn_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c1571_iec_data_w), DEVCB_DEVICE_LINE(_tag, c1571_iec_reset_w)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1571_config c1571_config;
struct _c1571_config
{
	const char *serial_bus_tag;	/* serial bus device */
	int address;				/* serial address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* IEC interface */
WRITE_LINE_DEVICE_HANDLER( c1571_iec_srq_w );
WRITE_LINE_DEVICE_HANDLER( c1571_iec_atn_w );
WRITE_LINE_DEVICE_HANDLER( c1571_iec_data_w );
WRITE_LINE_DEVICE_HANDLER( c1571_iec_reset_w );

#endif
