/**********************************************************************

    Commodore 1581/1563 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1581__
#define __C1581__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(C1581, c1581);
DECLARE_LEGACY_DEVICE(C1563, c1563);

#define MCFG_C1581_ADD(_tag, _serial_bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1581, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1581_config, serial_bus_tag, _serial_bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1581_config, address, _address)

#define MCFG_C1563_ADD(_tag, _serial_bus_tag, _address) \
	MCFG_DEVICE_ADD(_tag, C1563, 0) \
	MCFG_DEVICE_CONFIG_DATAPTR(c1581_config, serial_bus_tag, _serial_bus_tag) \
	MCFG_DEVICE_CONFIG_DATA32(c1581_config, address, _address)

#define C1581_IEC(_tag) \
	_tag, DEVCB_DEVICE_LINE(_tag, c1581_iec_srq_w), DEVCB_DEVICE_LINE(_tag, c1581_iec_atn_w), DEVCB_NULL, DEVCB_DEVICE_LINE(_tag, c1581_iec_data_w), DEVCB_DEVICE_LINE(_tag, c1581_iec_reset_w)

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
/* IEC interface */
WRITE_LINE_DEVICE_HANDLER( c1581_iec_srq_w );
WRITE_LINE_DEVICE_HANDLER( c1581_iec_atn_w );
WRITE_LINE_DEVICE_HANDLER( c1581_iec_data_w );
WRITE_LINE_DEVICE_HANDLER( c1581_iec_reset_w );

#endif
