/*********************************************************************

    mockngbd.h

    Implementation of the Apple II Mockingboard

*********************************************************************/

#ifndef __MOCKNGBD__
#define __MOCKNGBD__

#include "driver.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MOCKINGBOARD		DEVICE_GET_INFO_NAME(mockingboard)

#define MDRV_MOCKINGBOARD_ADD(_tag)	\
	MDRV_DEVICE_ADD((_tag), MOCKINGBOARD, 0)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device get info function */
DEVICE_GET_INFO(mockingboard);

/* slot read function */
READ8_DEVICE_HANDLER(mockingboard_r);

/* slot write function */
WRITE8_DEVICE_HANDLER(mockingboard_w);


#endif /* __MOCKNGBD__ */
