/*****************************************************************************
 *
 * includes/poly88.h
 *
 ****************************************************************************/

#ifndef POLY88_H_
#define POLY88_H_
#include "machine/msm8251.h"
#include "devices/snapquik.h"

class poly88_state : public driver_device
{
public:
	poly88_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *video_ram;
	UINT8 intr;
	UINT8 last_code;
	UINT8 int_vector;
	emu_timer * cassette_timer;
	emu_timer * usart_timer;
	int previous_level;
	int clk_level;
	int clk_level_tape;
};


/*----------- defined in machine/poly88.c -----------*/

DRIVER_INIT(poly88);
MACHINE_RESET(poly88);
INTERRUPT_GEN( poly88_interrupt );
READ8_HANDLER(poly88_keyboard_r);
WRITE8_HANDLER(poly88_intr_w);
WRITE8_HANDLER(poly88_baud_rate_w);

extern const msm8251_interface poly88_usart_interface;

extern SNAPSHOT_LOAD( poly88 );


/*----------- defined in video/poly88.c -----------*/

extern VIDEO_START( poly88 );
extern VIDEO_UPDATE( poly88 );

#endif /* POLY88_H_ */

