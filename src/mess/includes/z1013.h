/*****************************************************************************
 *
 * includes/z1013.h
 *
 ****************************************************************************/

#ifndef Z1013_H_
#define Z1013_H_

#include "machine/z80pio.h"
#include "devices/snapquik.h"


/*----------- defined in machine/z1013.c -----------*/

extern const z80pio_interface z1013_z80pio_intf;
extern const z80pio_interface z1013k7659_z80pio_intf;

DRIVER_INIT( z1013 );
MACHINE_RESET( z1013 );

extern WRITE8_HANDLER(z1013_keyboard_w);

extern SNAPSHOT_LOAD( z1013 );

/*----------- defined in video/z1013.c -----------*/
extern UINT8 *z1013_video_ram;

VIDEO_START( z1013 );
VIDEO_UPDATE( z1013 );


#endif /* Z1013_H_ */
