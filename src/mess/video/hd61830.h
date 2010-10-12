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

#define MDRV_HD61830_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, HD61830, _clock) \
	MDRV_DEVICE_CONFIG(_config)

#define HD61830_INTERFACE(name) \
	const hd61830_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef UINT8 (*hd61830_rd_read)(running_device *device, UINT16 ma, UINT8 md);
#define HD61830_RD_READ(name)	UINT8 name(running_device *device, UINT16 ma, UINT8 md)

typedef struct _hd61830_interface hd61830_interface;
struct _hd61830_interface
{
	const char *screen_tag;		/* screen we are acting on */

	hd61830_rd_read	rd_r;
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
