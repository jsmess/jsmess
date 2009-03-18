#ifndef __UPD7220__
#define __UPD7220__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define UPD7220		DEVICE_GET_INFO_NAME(upd7220)

#define MDRV_UPD7220_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, UPD7220, _clock) \
	MDRV_DEVICE_CONFIG(_config)

#define UPD7220_INTERFACE(name) const upd7220_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd7220_interface upd7220_interface;
struct _upd7220_interface
{
	const char *screen_tag;		/* screen we are acting on */

	/* this gets called for every change of the DRQ pin (pin 7) */
	devcb_write_line		out_drq_func;

	/* this gets called for every change of the HSYNC pin (pin 3) */
	devcb_write_line		out_hsync_func;

	/* this gets called for every change of the VSYNC pin (pin 4) */
	devcb_write_line		out_vsync_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( upd7220 );

/* register read */
READ8_DEVICE_HANDLER( upd7220_r );

/* register write */
WRITE8_DEVICE_HANDLER( upd7220_w );

/* dma acknowledge with write */
WRITE8_DEVICE_HANDLER( upd7220_dack_w );

/* screen update */
void upd7220_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
