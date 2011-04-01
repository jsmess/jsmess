/*****************************************************************************
 *
 * includes/ut88.h
 *
 ****************************************************************************/

#ifndef UT88_H_
#define UT88_H_

#include "machine/i8255a.h"

class ut88_state : public driver_device
{
public:
	ut88_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_video_ram;
	int m_keyboard_mask;
	int m_lcd_digit[6];
};


/*----------- defined in machine/ut88.c -----------*/

extern const i8255a_interface ut88_ppi8255_interface;

extern DRIVER_INIT( ut88 );
extern MACHINE_RESET( ut88 );
extern READ8_DEVICE_HANDLER( ut88_keyboard_r );
extern WRITE8_DEVICE_HANDLER( ut88_keyboard_w );
extern WRITE8_HANDLER( ut88_sound_w );
extern READ8_HANDLER( ut88_tape_r );

extern DRIVER_INIT( ut88mini );
extern MACHINE_START( ut88mini );
extern MACHINE_RESET( ut88mini );
extern READ8_HANDLER( ut88mini_keyboard_r );
extern WRITE8_HANDLER( ut88mini_write_led );

/*----------- defined in video/ut88.c -----------*/

extern const gfx_layout ut88_charlayout;

extern VIDEO_START( ut88 );
extern SCREEN_UPDATE( ut88 );


#endif /* UT88_H_ */
