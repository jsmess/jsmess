/***************************************************************************

    Epson TF-20

    Dual floppy drive with HX-20 factory option

***************************************************************************/

#ifndef __TF20_H__
#define __TF20_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

#if 0
typedef struct _tf20_interface tf20_interface;
struct _tf20_interface
{
};
#endif


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( tf20 );

/* serial interface in (to the host computer) */
WRITE_LINE_DEVICE_HANDLER( tf20_txs_w );
READ_LINE_DEVICE_HANDLER( tf20_rxs_r );
WRITE_LINE_DEVICE_HANDLER( tf20_pouts_w );
READ_LINE_DEVICE_HANDLER( tf20_pins_r );

#ifdef UNUSED_FUNCTION
/* serial interface out (to another terminal) */
WRITE_LINE_DEVICE_HANDLER( tf20_txc_r );
READ_LINE_DEVICE_HANDLER( tf20_rxc_w );
WRITE_LINE_DEVICE_HANDLER( tf20_poutc_r );
READ_LINE_DEVICE_HANDLER( tf20_pinc_w );
#endif


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define TF20 DEVICE_GET_INFO_NAME(tf20)

#define MDRV_TF20_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TF20, 0) \


#endif /* __TF20_H__ */
