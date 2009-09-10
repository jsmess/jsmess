/**********************************************************************

	RCA VP550 - VIP Super Sound System emulation

	Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __VP550__
#define __VP550__

#include "devcb.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define VP550_TAG	"vp550"

#define VP550 DEVICE_GET_INFO_NAME(vp550)

#define MDRV_VP550_ADD(_clock) \
	MDRV_DEVICE_ADD(VP550_TAG, VP550, _clock)

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( vp550 );

/* Q line */
WRITE_LINE_DEVICE_HANDLER( vp550_q_w );

/* install write handlers */
void vp550_install_write_handlers(const device_config *device, const address_space *program, int enabled);

#endif /* __VP550__ */
