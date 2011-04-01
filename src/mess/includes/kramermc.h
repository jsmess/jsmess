/*****************************************************************************
 *
 * includes/kramermc.h
 *
 ****************************************************************************/

#ifndef KRAMERMC_H_
#define KRAMERMC_H_

#include "machine/z80pio.h"

class kramermc_state : public driver_device
{
public:
	kramermc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_key_row;
};


/*----------- defined in machine/kramermc.c -----------*/

DRIVER_INIT( kramermc );
MACHINE_RESET( kramermc );

extern const z80pio_interface kramermc_z80pio_intf;

/*----------- defined in video/kramermc.c -----------*/

extern const gfx_layout kramermc_charlayout;

VIDEO_START( kramermc );
SCREEN_UPDATE( kramermc );


#endif /* KRAMERMC_h_ */
