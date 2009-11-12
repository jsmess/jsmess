/*****************************************************************************
 *
 *  includes/genesis.h
 *
 ****************************************************************************/

#ifndef GENESIS_H_
#define GENESIS_H_

#include "devices/cartslot.h"

/*----------- defined in machine/genesis.c -----------*/

MACHINE_RESET( md_mappers );

MACHINE_DRIVER_EXTERN( genesis_cartslot );
MACHINE_DRIVER_EXTERN( _32x_cartslot );
MACHINE_DRIVER_EXTERN( pico_cartslot );

#endif /* GENESIS_H_ */
