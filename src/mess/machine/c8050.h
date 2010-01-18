/**********************************************************************

    Commodore 8050/8250/SFD-1001 Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C8050__
#define __C8050__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define C8050	DEVICE_GET_INFO_NAME( c8050 )
#define C8250	DEVICE_GET_INFO_NAME( c8250 )
#define SFD1001	DEVICE_GET_INFO_NAME( sfd1001 )

#define MDRV_C8050_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C8050, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c8050_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c8050_config, address, _address)

#define MDRV_C8250_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C8250, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c8050_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c8050_config, address, _address)

#define MDRV_SFD1001_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, SFD1001, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c8050_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c8050_config, address, _address)

#define C8050_IEEE488(_tag) \
	_tag, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c8050_ieee488_ifc_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c8050_ieee488_atn_w), DEVCB_NULL

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c8050_config c8050_config;
struct _c8050_config
{
	const char *bus_tag;		/* bus device */
	int address;				/* bus address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( c8050 );
DEVICE_GET_INFO( c8250 );
DEVICE_GET_INFO( sfd1001 );

/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( c8050_ieee488_atn_w );
WRITE_LINE_DEVICE_HANDLER( c8050_ieee488_ifc_w );

#endif
