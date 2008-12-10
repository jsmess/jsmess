#ifndef __CONKORT__
#define __CONKORT__

#include "driver.h"

#define CONKORT_Z80_TAG		"5a"
#define CONKORT_Z80PIO_TAG	"3a"
#define CONKORT_Z80DMA_TAG	"z80dma"

#define CONKORT_TAG	"conkort"

#define LUXOR_55_10828 DEVICE_GET_INFO_NAME( luxor_55_10828 )
#define LUXOR_55_21046 DEVICE_GET_INFO_NAME( luxor_55_21046 )

/* device interface */
DEVICE_GET_INFO( luxor_55_10828 );
DEVICE_GET_INFO( luxor_55_21046 );

/* configuration dips */
INPUT_PORTS_EXTERN( luxor_55_21046 );

#endif
