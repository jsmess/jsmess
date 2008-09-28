/*****************************************************************************
 *
 * includes/kramermc.h
 *
 ****************************************************************************/

#ifndef KRAMERMC_H_
#define KRAMERMC_H_

#include "machine/z80pio.h"

/*----------- defined in machine/kramermc.c -----------*/

DRIVER_INIT( kramermc );
MACHINE_RESET( kramermc );

const z80pio_interface kramermc_z80pio_intf;

/*----------- defined in video/kramermc.c -----------*/

extern const gfx_layout kramermc_charlayout;

VIDEO_START( kramermc );
VIDEO_UPDATE( kramermc );


#endif /* KRAMERMC_h_ */
