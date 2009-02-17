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
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( centronics );

WRITE8_DEVICE_HANDLER( centronics_data_w );
READ8_DEVICE_HANDLER( centronics_data_r );

/* access to the individual bits */
WRITE_LINE_DEVICE_HANDLER( centronics_d0_w );
WRITE_LINE_DEVICE_HANDLER( centronics_d1_w );
WRITE_LINE_DEVICE_HANDLER( centronics_d2_w );
WRITE_LINE_DEVICE_HANDLER( centronics_d3_w );
WRITE_LINE_DEVICE_HANDLER( centronics_d4_w );
WRITE_LINE_DEVICE_HANDLER( centronics_d5_w );
WRITE_LINE_DEVICE_HANDLER( centronics_d6_w );
WRITE_LINE_DEVICE_HANDLER( centronics_d7_w );

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

/* callback for the printer device */
void centronics_printer_online(const device_config *device, int state);


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define CENTRONICS DEVICE_GET_INFO_NAME(centronics)

#define MDRV_CENTRONICS_ADD(_tag, _intf) \
	MDRV_PRINTER_ADD(_tag) \
	MDRV_PRINTER_ONLINE(centronics_printer_online) \
	MDRV_DEVICE_ADD(_tag, CENTRONICS, 0) \
	MDRV_DEVICE_CONFIG(_intf)

#define MDRV_CENTRONICS_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag, PRINTER) \
	MDRV_DEVICE_REMOVE(_tag, CENTRONICS)


/***************************************************************************
    DEFAULT INTERFACES
***************************************************************************/

extern const centronics_interface standard_centronics;


#endif /* __CTRONICS_H__ */
