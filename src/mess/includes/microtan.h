/******************************************************************************
 *  Microtan 65
 *
 *  variables and function prototypes
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http://www.geo255.redhotant.com
 *  and to Fabrice Frances <frances@ensica.fr>
 *  for his site http://www.ifrance.com/oric/microtan.html
 *
 ******************************************************************************/

#ifndef MICROTAN_H_
#define MICROTAN_H_

#include "devices/snapquik.h"


/*----------- defined in machine/microtan.c -----------*/

DRIVER_INIT( microtan );
MACHINE_RESET( microtan );

SNAPSHOT_LOAD( microtan );
QUICKLOAD_LOAD( microtan_hexfile );

INTERRUPT_GEN( microtan_interrupt );

 READ8_HANDLER ( microtan_via_0_r );
 READ8_HANDLER ( microtan_via_1_r );
 READ8_HANDLER ( microtan_bffx_r );
 READ8_HANDLER ( microtan_sound_r );
 READ8_HANDLER ( microtan_sio_r );

WRITE8_HANDLER ( microtan_via_0_w );
WRITE8_HANDLER ( microtan_via_1_w );
WRITE8_HANDLER ( microtan_bffx_w );
WRITE8_HANDLER ( microtan_sound_w );
WRITE8_HANDLER ( microtan_sio_w );


/*----------- defined in video/microtan.c -----------*/

extern UINT8 microtan_chunky_graphics;
extern UINT8 *microtan_chunky_buffer;

extern WRITE8_HANDLER ( microtan_videoram_w );

extern VIDEO_START( microtan );
extern VIDEO_UPDATE( microtan );


#endif /* MICROTAN_H_ */
