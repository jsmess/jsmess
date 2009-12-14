/**********************************************************************

    Commodore 1540/1541 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1541__
#define __C1541__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define C1540 DEVICE_GET_INFO_NAME( c1540 )
#define C1541 DEVICE_GET_INFO_NAME( c1541 )

#define MDRV_C1540_ADD(_tag, _serial_bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1540, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, serial_bus_tag, _serial_bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MDRV_C1541_ADD(_tag, _serial_bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1541, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, serial_bus_tag, _serial_bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1541_config c1541_config;
struct _c1541_config
{
	const char *serial_bus_tag;	/* serial bus device */
	int address;				/* serial address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( c1540 );
DEVICE_GET_INFO( c1541 );

#endif
