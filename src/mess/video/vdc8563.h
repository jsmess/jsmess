/*****************************************************************************
 *
 * video/vdc8563.h
 *
 * CBM Video Device Chip 8563
 *
 * peter.trauner@jk.uni-linz.ac.at, 2000
 *
 ****************************************************************************/

#ifndef __VDC8563_H__
#define __VDC8563_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vdc8563_interface vdc8563_interface;
struct _vdc8563_interface
{
	const char         *screen;
	int                ram16konly;
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( vdc8563 );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define VDC8563 DEVICE_GET_INFO_NAME( vdc8563 )

#define MDRV_VDC8563_ADD(_tag, _interface) \
	MDRV_DEVICE_ADD(_tag, VDC8563, 0) \
	MDRV_DEVICE_CONFIG(_interface)


/*----------- defined in video/vdc8563.c -----------*/

void vdc8563_set_rastering(running_device *device, int on);
UINT32 vdc8563_video_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect);

WRITE8_DEVICE_HANDLER( vdc8563_port_w );
READ8_DEVICE_HANDLER( vdc8563_port_r );


#endif /* __VDC8563_H__ */
