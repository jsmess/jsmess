/*****************************************************************************
 *
 *	includes/genesis.h
 *
 ****************************************************************************/

#ifndef GENESIS_H_
#define GENESIS_H_

#include "devices/cartslot.h"

/*----------- defined in machine/genesis.c -----------*/

MACHINE_RESET( md_mappers );
DRIVER_INIT( gencommon );

extern const cartslot_interface genesis_cartslot;
extern const cartslot_interface pico_cartslot;

#endif /* GENESIS_H_ */
