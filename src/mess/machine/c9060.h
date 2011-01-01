/**********************************************************************

    Commodore 9060/9090 Hard Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C9060__
#define __C9060__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C9060, c9060);
DECLARE_LEGACY_DEVICE(C9090, c9090);

#define MCFG_C9060_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C9060, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c9060_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c9060_config, address, _address)

#define MCFG_C9090_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C9090, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c9060_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c9060_config, address, _address)

#define C9060_IEEE488(_tag) \
	_tag, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c9060_ieee488_ifc_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c9060_ieee488_atn_w), DEVCB_NULL

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c9060_config c9060_config;
struct _c9060_config
{
	const char *bus_tag;		/* bus device */
	int address;				/* bus address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( c9060_ieee488_atn_w );
WRITE_LINE_DEVICE_HANDLER( c9060_ieee488_ifc_w );

#endif
