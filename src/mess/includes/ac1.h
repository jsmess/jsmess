/*****************************************************************************
 *
 * includes/ac1.h
 *
 ****************************************************************************/

#ifndef AC1_H_
#define AC1_H_

#include "machine/z80pio.h"

class ac1_state : public driver_device
{
public:
	ac1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	device_t *m_cassette;
};


/*----------- defined in machine/ac1.c -----------*/

DRIVER_INIT( ac1 );
MACHINE_RESET( ac1 );

extern const z80pio_interface ac1_z80pio_intf;

/*----------- defined in video/ac1.c -----------*/

extern const gfx_layout ac1_charlayout;

VIDEO_START( ac1 );
SCREEN_UPDATE( ac1 );
SCREEN_UPDATE( ac1_32 );


#endif /* AC1_h_ */
