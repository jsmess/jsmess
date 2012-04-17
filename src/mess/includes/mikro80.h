/*****************************************************************************
 *
 * includes/mikro80.h
 *
 ****************************************************************************/

#ifndef MIKRO80_H_
#define MIKRO80_H_

#include "machine/i8255.h"

class mikro80_state : public driver_device
{
public:
	mikro80_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_cursor_ram(*this, "cursor_ram"),
		m_video_ram(*this, "video_ram"){ }

	required_shared_ptr<UINT8> m_cursor_ram;
	required_shared_ptr<UINT8> m_video_ram;
	int m_keyboard_mask;
	int m_key_mask;
};


/*----------- defined in machine/mikro80.c -----------*/

extern const i8255_interface mikro80_ppi8255_interface;

extern DRIVER_INIT( mikro80 );
extern DRIVER_INIT( radio99 );
extern MACHINE_RESET( mikro80 );
extern READ8_HANDLER( mikro80_keyboard_r );
extern WRITE8_HANDLER( mikro80_keyboard_w );
extern READ8_HANDLER( mikro80_tape_r );
extern WRITE8_HANDLER( mikro80_tape_w );

extern READ8_HANDLER (mikro80_8255_portb_r );
extern READ8_HANDLER (mikro80_8255_portc_r );
extern WRITE8_HANDLER (mikro80_8255_porta_w );
extern WRITE8_HANDLER (mikro80_8255_portc_w );
/*----------- defined in video/mikro80.c -----------*/

extern VIDEO_START( mikro80 );
extern SCREEN_UPDATE_IND16( mikro80 );

#endif /* UT88_H_ */
