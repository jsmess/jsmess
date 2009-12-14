/**********************************************************************

    Commodore 1570/1571/1571CR Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1571__
#define __C1571__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define C1570 DEVICE_GET_INFO_NAME( c1570 )
#define C1571 DEVICE_GET_INFO_NAME( c1571 )
#define C1571CR DEVICE_GET_INFO_NAME( c1571cr )

#define MDRV_C1570_ADD(_tag, _serial_bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1570, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1571_config, serial_bus_tag, _serial_bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1571_config, address, _address)

#define MDRV_C1571_ADD(_tag, _serial_bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1571, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1571_config, serial_bus_tag, _serial_bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1571_config, address, _address)

#define MDRV_C1571CR_ADD(_tag, _serial_bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1571CR, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1571_config, serial_bus_tag, _serial_bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1571_config, address, _address)

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

/* device interface */
DEVICE_GET_INFO( c1570 );
DEVICE_GET_INFO( c1571 );
DEVICE_GET_INFO( c1571cr );

#endif
