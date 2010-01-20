/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __C1551__
#define __C1551__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define C1551 DEVICE_GET_INFO_NAME( c1551 )

#define MDRV_C1551_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, C1551, 0)

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( c1551 );

#endif
