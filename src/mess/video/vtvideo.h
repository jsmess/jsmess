/**********************************************************************

    DEC VT Terminal video emulation
    [ DC012 and DC011 emulation ]

    01/05/2009 Initial implementation [Miodrag Milanovic]
    
    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __VT_VIDEO__
#define __VT_VIDEO__

#include "driver.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define VT100_VIDEO DEVICE_GET_INFO_NAME( vt100_video )

#define MDRV_VT100_VIDEO_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, VT100_VIDEO, 0) \
	MDRV_DEVICE_CONFIG(_intrf)
/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vt_video_interface vt_video_interface;
struct _vt_video_interface
{
	const char *screen_tag;		/* screen we are acting on */
	const char *char_rom_region_tag; /* character rom region */

	/* this gets called for every memory read */
	devcb_read8			in_ram_func;
	devcb_write8		clear_video_interrupt;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( vt100_video );

/* register access */
READ8_DEVICE_HANDLER  ( lba7_r );
WRITE8_DEVICE_HANDLER ( dc012_w );
WRITE8_DEVICE_HANDLER ( dc011_w );
WRITE8_DEVICE_HANDLER ( vt_video_brightness_w );


/* screen update */
void vt_video_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
