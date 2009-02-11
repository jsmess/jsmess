#ifndef __E0516__
#define __E0516__

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
int e0516_dio_r(const device_config *device);
void e0516_dio_w(const device_config *device, int level);

/* clock */
void e0516_clk_w(const device_config *device, int level);

/* chip select */
void e0516_cs_w(const device_config *device, int level);

#endif
