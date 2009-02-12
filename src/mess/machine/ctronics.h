/***************************************************************************

    Centronics printer interface

***************************************************************************/

#ifndef __CTRONICS_H__
#define __CTRONICS_H__

#include "devcb.h"
#include "devices/printer.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _centronics_interface centronics_interface;
struct _centronics_interface
{
	int is_ibmpc;

	devcb_write_line out_ack_func;
	devcb_write_line out_busy_func;
	devcb_write_line out_not_busy_func;
};


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define CENTRONICS DEVICE_GET_INFO_NAME(centronics)

#define MDRV_CENTRONICS_ADD(_tag, _intf) \
	MDRV_PRINTER_ADD(_tag"_p") \
	MDRV_DEVICE_ADD(_tag, CENTRONICS, 0) \
	MDRV_DEVICE_CONFIG(_intf)

#define MDRV_CENTRONICS_REMOVE(_tag) \
	MDRV_PRINTER_REMOVE(_tag"_p") \
	MDRV_DEVICE_REMOVE(_tag, CENTRONICS)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( centronics );

WRITE8_DEVICE_HANDLER( centronics_data_w );

WRITE_LINE_DEVICE_HANDLER( centronics_strobe_w );
WRITE_LINE_DEVICE_HANDLER( centronics_prime_w );
WRITE_LINE_DEVICE_HANDLER( centronics_init_w );
WRITE_LINE_DEVICE_HANDLER( centronics_autofeed_w );

READ_LINE_DEVICE_HANDLER( centronics_ack_r );
READ_LINE_DEVICE_HANDLER( centronics_busy_r );
READ_LINE_DEVICE_HANDLER( centronics_pe_r );
READ_LINE_DEVICE_HANDLER( centronics_not_busy_r );
READ_LINE_DEVICE_HANDLER( centronics_vcc_r );
READ_LINE_DEVICE_HANDLER( centronics_fault_r );


/***************************************************************************
    DEFAULT INTERFACES
***************************************************************************/

extern const centronics_interface standard_centronics;


#endif /* __CTRONICS_H__ */
