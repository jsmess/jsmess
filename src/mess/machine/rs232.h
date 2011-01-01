/**********************************************************************

    EIA/TIA RS-232 interface emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __RS232__
#define __RS232__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(RS232, rs232);

#define MCFG_RS232_ADD(_tag, _intf) \
	MCFG_DEVICE_ADD(_tag, RS232, 0) \
	MCFG_DEVICE_CONFIG(_intf)

#define RS232_INTERFACE(_name) \
	const rs232_interface (_name)[] =

enum _rs232_equipment_type
{
	RS232_DTE,	/* data terminal equipment (computer) */
	RS232_DCE	/* data communications equipment (modem) */
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef enum _rs232_equipment_type rs232_equipment_type;

typedef struct _rs232_interface rs232_interface;
struct _rs232_interface
{
	const char *tag;	/* device tag */

	devcb_write_line		out_rd_func;
	devcb_write_line		out_dcd_func;
	devcb_write_line		out_dtr_func;
	devcb_write_line		out_dsr_func;
	devcb_write_line		out_rts_func;
	devcb_write_line		out_cts_func;
	devcb_write_line		out_ri_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* receive/transmit data */
READ_LINE_DEVICE_HANDLER( rs232_rd_r );
void rs232_td_w(device_t *rs232, device_t *device, int state);

/* data carrier detect */
READ_LINE_DEVICE_HANDLER( rs232_dcd_r );
WRITE_LINE_DEVICE_HANDLER( rs232_dcd_w );

/* data terminal ready */
READ_LINE_DEVICE_HANDLER( rs232_dtr_r );
WRITE_LINE_DEVICE_HANDLER( rs232_dtr_w );

/* data set ready */
READ_LINE_DEVICE_HANDLER( rs232_dsr_r );
WRITE_LINE_DEVICE_HANDLER( rs232_dsr_w );

/* request to send */
READ_LINE_DEVICE_HANDLER( rs232_rts_r );
WRITE_LINE_DEVICE_HANDLER( rs232_rts_w );

/* clear to send */
READ_LINE_DEVICE_HANDLER( rs232_cts_r );
WRITE_LINE_DEVICE_HANDLER( rs232_cts_w );

/* ring indicator */
READ_LINE_DEVICE_HANDLER( rs232_ri_r );
WRITE_LINE_DEVICE_HANDLER( rs232_ri_w );

#endif
