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

MACHINE_CONFIG_EXTERN( genesis_cartslot );
MACHINE_CONFIG_EXTERN( _32x_cartslot );
MACHINE_CONFIG_EXTERN( pico_cartslot );

WRITE16_HANDLER( jcart_ctrl_w );
READ16_HANDLER( jcart_ctrl_r );

#endif /* GENESIS_H_ */
