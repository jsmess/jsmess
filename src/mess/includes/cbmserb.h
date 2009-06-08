/*****************************************************************************
 *
 * includes/cbmserb.h
 *
 ****************************************************************************/

#ifndef CBMSERB_H_
#define CBMSERB_H_

/*----------- defined in machine/cbmserb.c -----------*/

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CBM_SERBUS		DEVICE_GET_INFO_NAME(cbm_serial_bus)

#define MDRV_CBM_SERBUS_ADD(_tag, _config) \
	MDRV_DEVICE_ADD(_tag, CBM_SERBUS, 0) \
	MDRV_DEVICE_CONFIG(_config)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cbm_serial_bus_interface cbm_serial_bus_interface;
struct _cbm_serial_bus_interface
{
	devcb_read8 read_atn_func;
	devcb_read8 read_clock_func;
	devcb_read8 read_data_func;
	devcb_read8 read_request_func;
	devcb_write8 write_atn_func;
	devcb_write8 write_clock_func;
	devcb_write8 write_data_func;
	devcb_write8 write_request_func;

	devcb_write8 write_reset_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( cbm_serial_bus );

READ8_DEVICE_HANDLER( cbm_serial_request_read );
READ8_DEVICE_HANDLER( cbm_serial_atn_read );
READ8_DEVICE_HANDLER( cbm_serial_data_read );
READ8_DEVICE_HANDLER( cbm_serial_clock_read );

WRITE8_DEVICE_HANDLER( cbm_serial_request_write );
WRITE8_DEVICE_HANDLER( cbm_serial_atn_write );
WRITE8_DEVICE_HANDLER( cbm_serial_data_write );
WRITE8_DEVICE_HANDLER( cbm_serial_clock_write );


#endif /* CBMSERB_H_ */
