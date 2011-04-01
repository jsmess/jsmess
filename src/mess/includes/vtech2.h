/*****************************************************************************
 *
 * includes/vtech2.h
 *
 ****************************************************************************/

#ifndef VTECH2_H_
#define VTECH2_H_


#define TRKSIZE_FM	3172	/* size of a standard FM mode track */

class vtech2_state : public driver_device
{
public:
	vtech2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	int m_laser_latch;
	char m_laser_frame_message[64+1];
	int m_laser_frame_time;
	UINT8 *m_mem;
	int m_laser_bank_mask;
	int m_laser_bank[4];
	int m_laser_video_bank;
	UINT8 m_laser_track_x2[2];
	UINT8 m_laser_fdc_status;
	UINT8 m_laser_fdc_data[TRKSIZE_FM];
	int m_laser_data;
	int m_laser_fdc_edge;
	int m_laser_fdc_bits;
	int m_laser_drive;
	int m_laser_fdc_start;
	int m_laser_fdc_write;
	int m_laser_fdc_offs;
	int m_laser_fdc_latch;
	int m_level_old;
	int m_cassette_bit;
	int m_row_a;
	int m_row_b;
	int m_row_c;
	int m_row_d;
	int m_laser_bg_mode;
	int m_laser_two_color;
};


/*----------- defined in machine/vtech2.c -----------*/

DRIVER_INIT(laser);
MACHINE_RESET( laser350 );
MACHINE_RESET( laser500 );
MACHINE_RESET( laser700 );

DEVICE_IMAGE_LOAD( laser_cart );
DEVICE_IMAGE_UNLOAD( laser_cart );

extern  READ8_HANDLER ( laser_fdc_r );
extern WRITE8_HANDLER ( laser_fdc_w );
extern WRITE8_HANDLER ( laser_bank_select_w );


/*----------- defined in video/vtech2.c -----------*/

extern VIDEO_START( laser );
extern SCREEN_UPDATE( laser );
extern WRITE8_HANDLER ( laser_bg_mode_w );
extern WRITE8_HANDLER ( laser_two_color_w );


#endif /* VTECH2_H_ */
