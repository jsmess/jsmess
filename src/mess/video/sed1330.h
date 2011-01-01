/**********************************************************************

    Seiko-Epson SED1330 LCD Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __SED1330__
#define __SED1330__

#include "emu.h"
#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(SED1330, sed1330);

#define MCFG_SED1330_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, SED1330, _clock) \
	MCFG_DEVICE_CONFIG(_config)

#define SED1330_INTERFACE(name) \
	const sed1330_interface (name) =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _sed1330_interface sed1330_interface;
struct _sed1330_interface
{
	const char *screen_tag;		/* screen we are acting on */

	devcb_read8				in_vd_func;
	devcb_write8			out_vd_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* register access */
READ8_DEVICE_HANDLER( sed1330_status_r );
WRITE8_DEVICE_HANDLER( sed1330_command_w );

/* memory access */
READ8_DEVICE_HANDLER( sed1330_data_r );
WRITE8_DEVICE_HANDLER( sed1330_data_w );

/* screen update */
void sed1330_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
