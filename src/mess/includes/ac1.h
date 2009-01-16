/*****************************************************************************
 *
 * includes/ac1.h
 *
 ****************************************************************************/

#ifndef AC1_H_
#define AC1_H_

#include "machine/z80pio.h"

/*----------- defined in machine/ac1.c -----------*/

DRIVER_INIT( ac1 );
MACHINE_RESET( ac1 );

extern const z80pio_interface ac1_z80pio_intf;

/*----------- defined in video/ac1.c -----------*/

extern const gfx_layout ac1_charlayout;

VIDEO_START( ac1 );
VIDEO_UPDATE( ac1 );
VIDEO_UPDATE( ac1_32 );


#endif /* AC1_h_ */
