/**********************************************************************

    Commodore 1540/1541/1541C/1541-II/2031 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1541__
#define __C1541__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C1540, c1540);
DECLARE_LEGACY_DEVICE(C1541, c1541);
DECLARE_LEGACY_DEVICE(C1541C, c1541c);
DECLARE_LEGACY_DEVICE(C1541II, c1541ii);
DECLARE_LEGACY_DEVICE(SX1541, sx1541);
DECLARE_LEGACY_DEVICE(C2031, c2031);
DECLARE_LEGACY_DEVICE(OC118, oc118);

#define MCFG_C1540_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1540, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MCFG_C1541_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1541, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MCFG_C1541C_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1541C, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MCFG_C1541II_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1541II, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MCFG_SX1541_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, SX1541, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MCFG_C2031_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C2031, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MCFG_OC118_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, OC118, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define C1541_IEC(_tag) \
	_tag, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c1541_iec_atn_w), DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c1541_iec_reset_w)

#define C2031_IEEE488(_tag) \
	_tag, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c2031_ieee488_ifc_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c2031_ieee488_atn_w), DEVCB_NULL

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1541_config c1541_config;
struct _c1541_config
{
	const char *bus_tag;		/* bus device */
	int address;				/* bus address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* IEC interface */
WRITE_LINE_DEVICE_HANDLER( c1541_iec_atn_w );
WRITE_LINE_DEVICE_HANDLER( c1541_iec_reset_w );

/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( c2031_ieee488_atn_w );
WRITE_LINE_DEVICE_HANDLER( c2031_ieee488_ifc_w );

#endif
