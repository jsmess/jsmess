/**********************************************************************

    Commodore VIC-1112 IEEE-488 cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __VIC1112__
#define __VIC1112__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define VIC1112_TAG	"vic1112"

#define VIC1112	DEVICE_GET_INFO_NAME( vic1112 )

#define MDRV_VIC1112_ADD(_ieee_bus_tag) \
	MDRV_DEVICE_ADD(VIC1112_TAG, VIC1112, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(vic1112_config, ieee_bus_tag, _ieee_bus_tag) \

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vic1112_config vic1112_config;
struct _vic1112_config
{
	const char *ieee_bus_tag;	/* IEEE bus device */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( vic1112 );

#endif
