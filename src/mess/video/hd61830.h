/**********************************************************************

    HD61830 LCD Timing Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __HD61830__
#define __HD61830__

#include "emu.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define HD61830 DEVICE_GET_INFO_NAME( hd61830 )

#define MDRV_HD61830_ADD(_tag, _clock, _screen) \
	MDRV_DEVICE_ADD(_tag, HD61830, _clock) \
	MDRV_DEVICE_CONFIG_DATAPTR(hd61830_config, screen_tag, _screen)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _hd61830_config hd61830_config;
struct _hd61830_config
{
	const char *screen_tag;		/* screen we are acting on */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( hd61830 );

/* register access */
READ8_DEVICE_HANDLER( hd61830_r );
WRITE8_DEVICE_HANDLER( hd61830_w );

READ8_DEVICE_HANDLER( hd61830_status_r );
WRITE8_DEVICE_HANDLER( hd61830_control_w );

READ8_DEVICE_HANDLER( hd61830_data_r );
WRITE8_DEVICE_HANDLER( hd61830_data_w );

/* screen update */
void hd61830_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
