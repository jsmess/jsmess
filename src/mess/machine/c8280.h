/**********************************************************************

    Commodore 8280 Dual 8" Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C8280__
#define __C8280__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C8280, c8280);

#define MCFG_C8280_ADD(_tag, _bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C8280, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c8280_config, bus_tag, _bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c8280_config, address, _address)

#define C8280_IEEE488(_tag) \
	_tag, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c8280_ieee488_ifc_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c8280_ieee488_atn_w), DEVCB_NULL

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c8280_config c8280_config;
struct _c8280_config
{
	const char *bus_tag;		/* bus device */
	int address;				/* bus address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( c8280_ieee488_atn_w );
WRITE_LINE_DEVICE_HANDLER( c8280_ieee488_ifc_w );

#endif
