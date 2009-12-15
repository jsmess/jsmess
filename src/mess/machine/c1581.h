/**********************************************************************

    Commodore 1581 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1581__
#define __C1581__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define C1581 DEVICE_GET_INFO_NAME( c1581 )

#define MDRV_C1581_ADD(_tag, _serial_bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1581, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1581_config, serial_bus_tag, _serial_bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1581_config, address, _address)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1581_config c1581_config;
struct _c1581_config
{
	const char *serial_bus_tag;	/* serial bus device */
	int address;				/* serial address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( c1581 );

#endif
