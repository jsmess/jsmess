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
#include "machine/6522via.h"


class microtan_state : public driver_device
{
public:
	microtan_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in machine/microtan.c -----------*/

extern const via6522_interface microtan_via6522_0;
extern const via6522_interface microtan_via6522_1;

DRIVER_INIT( microtan );
MACHINE_RESET( microtan );

SNAPSHOT_LOAD( microtan );
QUICKLOAD_LOAD( microtan_hexfile );

INTERRUPT_GEN( microtan_interrupt );

READ8_HANDLER ( microtan_bffx_r );
READ8_HANDLER ( microtan_sound_r );

WRITE8_HANDLER ( microtan_bffx_w );
WRITE8_HANDLER ( microtan_sound_w );


/*----------- defined in video/microtan.c -----------*/

extern UINT8 microtan_chunky_graphics;
extern UINT8 *microtan_chunky_buffer;

extern WRITE8_HANDLER ( microtan_videoram_w );

extern VIDEO_START( microtan );
extern VIDEO_UPDATE( microtan );


#endif /* MICROTAN_H_ */
