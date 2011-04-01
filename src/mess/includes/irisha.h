/*****************************************************************************
 *
 * includes/irisha.h
 *
 ****************************************************************************/

#ifndef IRISHA_H_
#define IRISHA_H_

#include "machine/i8255a.h"

class irisha_state : public driver_device
{
public:
	irisha_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int m_keyboard_mask;
	UINT8 m_keypressed;
	UINT8 m_keyboard_cnt;
};


/*----------- defined in machine/irisha.c -----------*/

extern DRIVER_INIT( irisha );
extern MACHINE_START( irisha );
extern MACHINE_RESET( irisha );
extern const i8255a_interface irisha_ppi8255_interface;
extern const struct pit8253_config irisha_pit8253_intf;
extern const struct pic8259_interface irisha_pic8259_config;

extern READ8_HANDLER (irisha_keyboard_r);


/*----------- defined in video/irisha.c -----------*/

extern VIDEO_START( irisha );
extern SCREEN_UPDATE( irisha );

#endif /* UT88_H_ */
