#ifndef __CONKORT__
#define __CONKORT__

#include "driver.h"

#define CONKORT_TAG	"55-21046"

#define LUXOR_CONKORT DEVICE_GET_INFO_NAME( conkort )

/* device interface */
DEVICE_GET_INFO( conkort );

/* configuration dips */
INPUT_PORTS_EXTERN( conkort );

/* ABCBUS interface */
READ8_HANDLER( conkort_abcbus_data_r );
WRITE8_HANDLER( conkort_abcbus_data_w );
READ8_HANDLER( conkort_abcbus_status_r );
WRITE8_HANDLER( conkort_abcbus_channel_w );
WRITE8_HANDLER( conkort_abcbus_command_w );
READ8_HANDLER( conkort_abcbus_reset_r );

#endif
