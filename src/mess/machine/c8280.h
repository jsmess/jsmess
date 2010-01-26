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

#define C8280	DEVICE_GET_INFO_NAME( c8280 )

#define MDRV_C8280_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C8280, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c8280_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c8280_config, address, _address)

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

/* device interface */
DEVICE_GET_INFO( c8280 );

/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( c8280_ieee488_atn_w );
WRITE_LINE_DEVICE_HANDLER( c8280_ieee488_ifc_w );

#endif
