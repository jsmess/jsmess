/*****************************************************************************
 *
 * includes/mz80.h
 *
 ****************************************************************************/

#ifndef MZ80_H_
#define MZ80_H_

class mz80_state : public driver_device
{
public:
	mz80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_mz80k_vertical;
	UINT8 m_mz80k_cursor_cnt;
	UINT8 m_mz80k_tempo_strobe;
	UINT8 m_mz80k_8255_portc;
	UINT8 m_mz80k_keyboard_line;
	UINT8 m_speaker_level;
	UINT8 m_prev_state;
};


/*----------- defined in machine/mz80.c -----------*/

extern DRIVER_INIT( mz80k );
extern MACHINE_RESET( mz80k );
extern READ8_HANDLER( mz80k_strobe_r );
extern WRITE8_HANDLER( mz80k_strobe_w );

extern const i8255a_interface mz80k_8255_int;
extern const struct pit8253_config mz80k_pit8253_config;


/*----------- defined in video/mz80.c -----------*/

extern const gfx_layout mz80k_charlayout;
extern const gfx_layout mz80kj_charlayout;

extern VIDEO_START( mz80k );
extern SCREEN_UPDATE( mz80k );

#endif /* MZ80_H_ */
