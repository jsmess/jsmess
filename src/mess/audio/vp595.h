/**********************************************************************

	RCA VP595 - VIP Simple Sound System emulation

	Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __VP595__
#define __VP595__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define VP595_TAG	"vp595"

#define VP595 DEVICE_GET_INFO_NAME(vp595)

#define MDRV_VP595_ADD \
	MDRV_DEVICE_ADD(VP595_TAG, VP595, 0)

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( vp595 );

/* Q line */
WRITE_LINE_DEVICE_HANDLER( vp595_q_w ) ATTR_NONNULL(1);

/* install write handlers */
void vp595_install_write_handlers(const device_config *device, const address_space *io, int enabled) ATTR_NONNULL(1) ATTR_NONNULL(2);

#endif /* __VP595__ */
