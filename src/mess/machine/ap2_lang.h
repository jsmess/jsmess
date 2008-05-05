/*********************************************************************

	ap2_lang.h

	Implementation of the Apple II Language Card

*********************************************************************/

#ifndef __AP2_LANG__
#define __AP2_LANG__

#include "driver.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define APPLE2_LANGCARD		DEVICE_GET_INFO_NAME(apple2_langcard)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device get info function */
DEVICE_GET_INFO(apple2_langcard);	

/* slot read function */
READ8_DEVICE_HANDLER(apple2_langcard_r);

/* slot write function */
WRITE8_DEVICE_HANDLER(apple2_langcard_w);

#endif /* __AP2_LANG__ */
