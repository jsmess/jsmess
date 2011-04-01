/*****************************************************************************
 *
 * includes/z1013.h
 *
 ****************************************************************************/

#ifndef Z1013_H_
#define Z1013_H_

#include "machine/z80pio.h"
#include "imagedev/snapquik.h"


class z1013_state : public driver_device
{
public:
	z1013_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_video_ram;
	UINT8 m_keyboard_line;
	UINT8 m_keyboard_part;
};


/*----------- defined in machine/z1013.c -----------*/

extern const z80pio_interface z1013_z80pio_intf;
extern const z80pio_interface z1013k7659_z80pio_intf;

DRIVER_INIT( z1013 );
MACHINE_RESET( z1013 );

extern WRITE8_HANDLER(z1013_keyboard_w);

extern SNAPSHOT_LOAD( z1013 );

/*----------- defined in video/z1013.c -----------*/

VIDEO_START( z1013 );
SCREEN_UPDATE( z1013 );


#endif /* Z1013_H_ */
