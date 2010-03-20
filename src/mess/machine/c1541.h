/**********************************************************************

    Commodore 1540/1541/1541C/1541-II/2031 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1541__
#define __C1541__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define C1540	DEVICE_GET_INFO_NAME( c1540 )
#define C1541	DEVICE_GET_INFO_NAME( c1541 )
#define C1541C	DEVICE_GET_INFO_NAME( c1541c )
#define C1541II DEVICE_GET_INFO_NAME( c1541ii )
#define SX1541	DEVICE_GET_INFO_NAME( sx1541 )
#define C2031	DEVICE_GET_INFO_NAME( c2031 )
#define OC118	DEVICE_GET_INFO_NAME( oc118 )

#define MDRV_C1540_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1540, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MDRV_C1541_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1541, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MDRV_C1541C_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1541C, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MDRV_C1541II_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C1541II, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MDRV_SX1541_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, SX1541, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MDRV_C2031_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, C2031, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define MDRV_OC118_ADD(_tag, _bus_tag, _address) \
	MDRV_DEVICE_ADD(_tag, OC118, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(c1541_config, bus_tag, _bus_tag) \
	MDRV_DEVICE_CONFIG_DATA32(c1541_config, address, _address)

#define C1541_IEC(_tag) \
	_tag, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c1541_iec_atn_w), DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c1541_iec_reset_w)

#define C2031_IEEE488(_tag) \
	_tag, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c2031_ieee488_ifc_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c2031_ieee488_atn_w), DEVCB_NULL

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1541_config c1541_config;
struct _c1541_config
{
	const char *bus_tag;		/* bus device */
	int address;				/* bus address */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( c1540 );
DEVICE_GET_INFO( c1541 );
DEVICE_GET_INFO( c1541c );
DEVICE_GET_INFO( c1541ii );
DEVICE_GET_INFO( sx1541 );
DEVICE_GET_INFO( c2031 );
DEVICE_GET_INFO( oc118 );

/* IEC interface */
WRITE_LINE_DEVICE_HANDLER( c1541_iec_atn_w );
WRITE_LINE_DEVICE_HANDLER( c1541_iec_reset_w );

/* IEEE-488 interface */
WRITE_LINE_DEVICE_HANDLER( c2031_ieee488_atn_w );
WRITE_LINE_DEVICE_HANDLER( c2031_ieee488_ifc_w );

#endif
