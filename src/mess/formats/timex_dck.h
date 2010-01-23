/*****************************************************************************
 *
 * formats/timex_dck.h
 *
 ****************************************************************************/

#ifndef __TIMEX_DCK_H__
#define __TIMEX_DCK_H__

#include "devices/snapquik.h"
#include "devices/cartslot.h"

typedef enum
{
    TIMEX_CART_NONE,
    TIMEX_CART_DOCK,
    TIMEX_CART_EXROM,
    TIMEX_CART_HOME
} TIMEX_CART_TYPE;

extern TIMEX_CART_TYPE timex_cart_type;
extern UINT8 timex_cart_chunks;
extern UINT8 *timex_cart_data;

DEVICE_IMAGE_LOAD( timex_cart );
DEVICE_IMAGE_UNLOAD( timex_cart );

#endif  /* __TIMEX_DCK_H__ */