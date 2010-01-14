/**********************************************************************

    Commodore VIC-1112 IEEE-488 Interface Cartridge emulation

	SYS 45065 to start

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

#define MDRV_VIC1112_ADD(_bus_tag) \
	MDRV_DEVICE_ADD(VIC1112_TAG, VIC1112, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(vic1112_config, bus_tag, _bus_tag) \

#define VIC1112_IEEE488 \
	VIC1112_TAG, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(VIC1112_TAG, vic1112_ieee488_srq_w), DEVCB_NULL, DEVCB_NULL

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vic1112_config vic1112_config;
struct _vic1112_config
{
	const char *bus_tag;	/* bus device */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( vic1112 );

/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( vic1112_ieee488_srq_w );

#endif
