/*****************************************************************************
 *
 * includes/vector06.h
 *
 ****************************************************************************/

#ifndef VECTOR06_H_
#define VECTOR06_H_

#include "machine/i8255a.h"

class vector06_state : public driver_device
{
public:
	vector06_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_keyboard_mask;
	UINT8 m_color_index;
	UINT8 m_video_mode;
	UINT8 m_romdisk_msb;
	UINT8 m_romdisk_lsb;
	UINT8 m_vblank_state;
};


/*----------- defined in machine/vector06.c -----------*/

extern const i8255a_interface vector06_ppi8255_interface;
extern const i8255a_interface vector06_ppi8255_2_interface;

extern MACHINE_START( vector06 );
extern MACHINE_RESET( vector06 );

extern INTERRUPT_GEN( vector06_interrupt );

extern READ8_HANDLER(vector06_8255_1_r);
extern WRITE8_HANDLER(vector06_8255_1_w);
extern READ8_HANDLER(vector06_8255_2_r);
extern WRITE8_HANDLER(vector06_8255_2_w);

extern WRITE8_HANDLER(vector06_color_set);
extern WRITE8_HANDLER(vector06_disc_w);


/*----------- defined in video/vector06.c -----------*/

extern PALETTE_INIT( vector06 );
extern VIDEO_START( vector06 );
extern SCREEN_UPDATE( vector06 );

#endif /* VECTOR06_H_ */
