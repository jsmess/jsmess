/**********************************************************************

    HD44102 Dot Matrix Liquid Crystal Graphic Display Column Driver emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __HD44102__
#define __HD44102__

#include "driver.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define HD44102 DEVICE_GET_INFO_NAME( hd44102 )

#define MDRV_HD44102_ADD(_tag, _screen, _sx, _sy) \
	MDRV_DEVICE_ADD((_tag), HD44102, 0) \
	MDRV_DEVICE_CONFIG_DATAPTR(hd44102_config, screen_tag, _screen) \
	MDRV_DEVICE_CONFIG_DATA32(hd44102_config, sx, _sx) \
	MDRV_DEVICE_CONFIG_DATA32(hd44102_config, sy, _sy)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _hd44102_config hd44102_config;
struct _hd44102_config
{
	const char *screen_tag;		/* screen we are acting on */

	int sx;						/* X origin on screen */
	int sy;						/* Y origin on screen */
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( hd44102 );

/* chip select */
WRITE_LINE_DEVICE_HANDLER( hd44102_cs2_w );

/* register access */
READ8_DEVICE_HANDLER( hd44102_r );
WRITE8_DEVICE_HANDLER( hd44102_w );

/* screen update */
void hd44102_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
