/**********************************************************************

    Commodore 2040/3040/4040/8050/8250/SFD-1001 Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C2040__
#define __C2040__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C2040, c2040);
DECLARE_LEGACY_DEVICE(C3040, c3040);
DECLARE_LEGACY_DEVICE(C4040, c4040);
DECLARE_LEGACY_DEVICE(C8050, c8050);
DECLARE_LEGACY_DEVICE(C8250, c8250);
DECLARE_LEGACY_DEVICE(SFD1001, sfd1001);

#define MCFG_C2040_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C2040, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c2040_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c2040_config, address, _address)

#define MCFG_C3040_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C3040, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c2040_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c2040_config, address, _address)

#define MCFG_C4040_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C4040, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c2040_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c2040_config, address, _address)

#define MCFG_C8050_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C8050, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c2040_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c2040_config, address, _address)

#define MCFG_C8250_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C8250, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c2040_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c2040_config, address, _address)

#define MCFG_SFD1001_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, SFD1001, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c2040_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c2040_config, address, _address)

#define C2040_IEEE488(_tag) \
	_tag, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c2040_ieee488_ifc_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c2040_ieee488_atn_w), DEVCB_NULL

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c2040_config c2040_config;
struct _c2040_config
{
	const char *bus_tag;		/* bus device */
	int address;				/* bus address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( c2040_ieee488_atn_w );
WRITE_LINE_DEVICE_HANDLER( c2040_ieee488_ifc_w );

#endif
