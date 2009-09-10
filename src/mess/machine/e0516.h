#ifndef __E0516__
#define __E0516__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define E0516 DEVICE_GET_INFO_NAME(e0516)

#define MDRV_E0516_ADD(_tag, _clock) \
	MDRV_DEVICE_ADD(_tag, E0516, _clock)

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( e0516 );

/* serial data input/output */
READ_LINE_DEVICE_HANDLER( e0516_dio_r );
WRITE_LINE_DEVICE_HANDLER( e0516_dio_w );

/* clock */
WRITE_LINE_DEVICE_HANDLER( e0516_clk_w );

/* chip select */
WRITE_LINE_DEVICE_HANDLER( e0516_cs_w );

#endif
