/**********************************************************************

    Commodore Serial Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __CBMSERIAL__
#define __CBMSERIAL__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CBMSERIAL DEVICE_GET_INFO_NAME( cbmserial )

#define MDRV_CBMSERIAL_ADD(_tag, _daisy_chain) \
	MDRV_DEVICE_ADD(_tag, CBMSERIAL, 0) \
	MDRV_DEVICE_CONFIG(_daisy_chain)

#define CBMSERIAL_DAISY(_name) \
	const cbmserial_daisy_chain (_name)[] =

enum
{
	DEVINFO_FCT_CBM_SERIAL_SRQ = DEVINFO_FCT_DEVICE_SPECIFIC,
	DEVINFO_FCT_CBM_SERIAL_ATN,
	DEVINFO_FCT_CBM_SERIAL_CLK,
	DEVINFO_FCT_CBM_SERIAL_DATA,
	DEVINFO_FCT_CBM_SERIAL_RESET
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cbmserial_daisy_chain cbmserial_daisy_chain;
struct _cbmserial_daisy_chain
{
	const char *tag;	/* device tag */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( cbmserial );

/* serial service request */
WRITE_LINE_DEVICE_HANDLER( cbmserial_srq_w );
READ_LINE_DEVICE_HANDLER( cbmserial_srq_r );

/* attention */
WRITE_LINE_DEVICE_HANDLER( cbmserial_atn_w );
READ_LINE_DEVICE_HANDLER( cbmserial_atn_r );

/* clock */
WRITE_LINE_DEVICE_HANDLER( cbmserial_clk_w );
READ_LINE_DEVICE_HANDLER( cbmserial_clk_r );

/* data */
WRITE_LINE_DEVICE_HANDLER( cbmserial_data_w );
READ_LINE_DEVICE_HANDLER( cbmserial_data_r );

/* reset */
WRITE_LINE_DEVICE_HANDLER( cbmserial_reset_w );
READ_LINE_DEVICE_HANDLER( cbmserial_reset_r );

#endif
