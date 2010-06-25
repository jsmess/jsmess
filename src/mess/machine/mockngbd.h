/*********************************************************************

    mockngbd.h

    Implementation of the Apple II Mockingboard

*********************************************************************/

#ifndef __MOCKNGBD__
#define __MOCKNGBD__

#include "emu.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(MOCKINGBOARD, mockingboard);

#define MDRV_MOCKINGBOARD_ADD(_tag)	\
	MDRV_DEVICE_ADD((_tag), MOCKINGBOARD, 0)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* slot read function */
READ8_DEVICE_HANDLER(mockingboard_r);

/* slot write function */
WRITE8_DEVICE_HANDLER(mockingboard_w);


#endif /* __MOCKNGBD__ */
