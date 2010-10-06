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

#define HD61830_TAG		"hd61830"

DECLARE_LEGACY_MEMORY_DEVICE(HD61830, hd61830);

#define MDRV_HD61830_ADD(_tag, _clock, _screen) \
	MDRV_DEVICE_ADD(_tag, HD61830, _clock) \
	MDRV_DEVICE_CONFIG_DATAPTR(hd61830_config, screen_tag, _screen)

#define MDRV_HD61830_CGROM_ADD(_tag, _clock, _screen, _cgrom_map) \
	MDRV_DEVICE_ADD(_tag, HD61830, _clock) \
	MDRV_DEVICE_CONFIG_DATAPTR(hd61830_config, screen_tag, _screen) \
	MDRV_DEVICE_ADDRESS_MAP(1, _cgrom_map)

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

/* register access */
READ8_DEVICE_HANDLER( hd61830_r );
WRITE8_DEVICE_HANDLER( hd61830_w );

READ8_DEVICE_HANDLER( hd61830_status_r );
WRITE8_DEVICE_HANDLER( hd61830_control_w );

READ8_DEVICE_HANDLER( hd61830_data_r );
WRITE8_DEVICE_HANDLER( hd61830_data_w );

/* screen update */
void hd61830_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
