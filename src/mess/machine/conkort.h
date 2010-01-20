#ifndef __CONKORT__
#define __CONKORT__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CONKORT_TAG	"conkort"

#define LUXOR_55_10828 DEVICE_GET_INFO_NAME( luxor_55_10828 )
#define LUXOR_55_21046 DEVICE_GET_INFO_NAME( luxor_55_21046 )

#define MDRV_LUXOR_55_10828_ADD \
	MDRV_DEVICE_ADD(CONKORT_TAG, LUXOR_55_10828, 0)

#define MDRV_LUXOR_55_21046_ADD \
	MDRV_DEVICE_ADD(CONKORT_TAG, LUXOR_55_21046, 0)

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( luxor_55_10828 );
DEVICE_GET_INFO( luxor_55_21046 );

/* configuration dips */
INPUT_PORTS_EXTERN( luxor_55_21046 );

#endif
