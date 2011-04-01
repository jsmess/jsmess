/*****************************************************************************
 *
 * includes/poly88.h
 *
 ****************************************************************************/

#ifndef POLY88_H_
#define POLY88_H_

#include "machine/serial.h"
#include "machine/msm8251.h"
#include "imagedev/snapquik.h"

class poly88_state : public driver_device
{
public:
	poly88_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_video_ram;
	UINT8 *m_FNT;
	UINT8 m_intr;
	UINT8 m_last_code;
	UINT8 m_int_vector;
	emu_timer * m_cassette_timer;
	emu_timer * m_usart_timer;
	int m_previous_level;
	int m_clk_level;
	int m_clk_level_tape;
	serial_connection m_cassette_serial_connection;
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
extern SCREEN_UPDATE( poly88 );

#endif /* POLY88_H_ */

