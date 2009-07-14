/*****************************************************************************
 *
 * includes/irisha.h
 *
 ****************************************************************************/

#ifndef IRISHA_H_
#define IRISHA_H_

#include "machine/i8255a.h"

/*----------- defined in machine/irisha.c -----------*/

extern DRIVER_INIT( irisha );
extern MACHINE_RESET( irisha );
extern const i8255a_interface irisha_ppi8255_interface;
extern const struct pit8253_config irisha_pit8253_intf;
extern const struct pic8259_interface irisha_pic8259_config;

extern READ8_HANDLER (irisha_keyboard_r);


/*----------- defined in video/irisha.c -----------*/

extern VIDEO_START( irisha );
extern VIDEO_UPDATE( irisha );

#endif /* UT88_H_ */
