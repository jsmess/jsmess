/*****************************************************************************
 *
 * includes/ti85.h
 *
 ****************************************************************************/

#ifndef TI85_H_
#define TI85_H_

#include "imagedev/snapquik.h"


class ti85_state : public driver_device
{
public:
	ti85_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_nvram(*this, "nvram") { }

	optional_shared_ptr<UINT8>	m_nvram;
	UINT8 m_LCD_memory_base;
	UINT8 m_LCD_contrast;
	UINT8 m_LCD_status;
	UINT8 m_timer_interrupt_mask;
	UINT8 m_ti82_video_buffer[0x300];
	UINT8 m_ti_calculator_model;
	UINT8 m_timer_interrupt_status;
	UINT8 m_ON_interrupt_mask;
	UINT8 m_ON_interrupt_status;
	UINT8 m_ON_pressed;
	UINT8 m_ti8x_memory_page_1;
	UINT8 m_ti8x_memory_page_2;
	UINT8 m_LCD_mask;
	UINT8 m_power_mode;
	UINT8 m_keypad_mask;
	UINT8 m_video_buffer_width;
	UINT8 m_interrupt_speed;
	UINT8 m_port4_bit0;
	UINT8 m_ti81_port_7_data;
	UINT8 *m_ti8x_ram;
	UINT8 m_PCR;
	UINT8 m_red_out;
	UINT8 m_white_out;
	UINT8 m_ti82_video_mode;
	UINT8 m_ti82_video_x;
	UINT8 m_ti82_video_y;
	UINT8 m_ti82_video_dir;
	UINT8 m_ti82_video_scroll;
	UINT8 m_ti82_video_bit;
	UINT8 m_ti82_video_col;
	UINT8 m_ti8x_port2;
	UINT8 m_ti83p_port4;
	int m_ti_video_memory_size;
	int m_ti_screen_x_size;
	int m_ti_screen_y_size;
	int m_ti_number_of_frames;
	UINT8 * m_frames;
};


/*----------- defined in machine/ti85.c -----------*/

MACHINE_START( ti81 );
MACHINE_START( ti82 );
MACHINE_START( ti85 );
MACHINE_START( ti83p );
MACHINE_START( ti86 );
MACHINE_RESET( ti85 );

NVRAM_HANDLER( ti83p );
NVRAM_HANDLER( ti86 );

SNAPSHOT_LOAD( ti8x );

WRITE8_HANDLER( ti81_port_0007_w );
 READ8_HANDLER( ti85_port_0000_r );
 READ8_HANDLER( ti8x_keypad_r );
 READ8_HANDLER( ti85_port_0002_r );
 READ8_HANDLER( ti85_port_0003_r );
 READ8_HANDLER( ti85_port_0004_r );
 READ8_HANDLER( ti85_port_0005_r );
 READ8_HANDLER( ti85_port_0006_r );
 READ8_HANDLER( ti85_port_0007_r );
 READ8_HANDLER( ti86_port_0005_r );
 READ8_HANDLER( ti86_port_0006_r );
 READ8_HANDLER( ti82_port_0000_r );
 READ8_HANDLER( ti82_port_0002_r );
 READ8_HANDLER( ti82_port_0010_r );
 READ8_HANDLER( ti82_port_0011_r );
 READ8_HANDLER( ti83_port_0000_r );
 READ8_HANDLER( ti83_port_0002_r );
 READ8_HANDLER( ti83_port_0003_r );
 READ8_HANDLER( ti73_port_0000_r );
 READ8_HANDLER( ti83p_port_0000_r );
 READ8_HANDLER( ti83p_port_0002_r );
WRITE8_HANDLER( ti85_port_0000_w );
WRITE8_HANDLER( ti8x_keypad_w );
WRITE8_HANDLER( ti85_port_0002_w );
WRITE8_HANDLER( ti85_port_0003_w );
WRITE8_HANDLER( ti85_port_0004_w );
WRITE8_HANDLER( ti85_port_0005_w );
WRITE8_HANDLER( ti85_port_0006_w );
WRITE8_HANDLER( ti85_port_0007_w );
WRITE8_HANDLER( ti86_port_0005_w );
WRITE8_HANDLER( ti86_port_0006_w );
WRITE8_HANDLER( ti82_port_0000_w );
WRITE8_HANDLER( ti82_port_0002_w );
WRITE8_HANDLER( ti82_port_0010_w );
WRITE8_HANDLER( ti82_port_0011_w );
WRITE8_HANDLER( ti83_port_0000_w );
WRITE8_HANDLER( ti83_port_0002_w );
WRITE8_HANDLER( ti83_port_0003_w );
WRITE8_HANDLER( ti73_port_0000_w );
WRITE8_HANDLER( ti83p_port_0000_w );
WRITE8_HANDLER( ti83p_port_0002_w );
WRITE8_HANDLER( ti83p_port_0003_w );
WRITE8_HANDLER( ti83p_port_0004_w );
WRITE8_HANDLER( ti83p_port_0006_w );
WRITE8_HANDLER( ti83p_port_0007_w );
WRITE8_HANDLER( ti83p_port_0010_w );

/*----------- defined in video/ti85.c -----------*/

VIDEO_START( ti85 );
SCREEN_UPDATE( ti85 );
SCREEN_UPDATE( ti82 );
PALETTE_INIT( ti85 );


#endif /* TI85_H_ */
