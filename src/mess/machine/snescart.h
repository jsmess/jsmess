/*
    snescart.h

*/

#ifndef _SNESCART_H
#define _SNESCART_H

#include "devices/cartslot.h"

MACHINE_START( snes_mess );
MACHINE_START( snesst );

DRIVER_INIT( snes_mess );
DRIVER_INIT( snesst );

MACHINE_DRIVER_EXTERN( snes_cartslot );
MACHINE_DRIVER_EXTERN( sufami_cartslot );
MACHINE_DRIVER_EXTERN( bsx_cartslot );

#endif /* _SNESCART_H */

