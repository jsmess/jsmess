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

	UINT8 *videoram;
	int laser_latch;
	char laser_frame_message[64+1];
	int laser_frame_time;
	UINT8 *mem;
	int laser_bank_mask;
	int laser_bank[4];
	int laser_video_bank;
	UINT8 laser_track_x2[2];
	UINT8 laser_fdc_status;
	UINT8 laser_fdc_data[TRKSIZE_FM];
	int laser_data;
	int laser_fdc_edge;
	int laser_fdc_bits;
	int laser_drive;
	int laser_fdc_start;
	int laser_fdc_write;
	int laser_fdc_offs;
	int laser_fdc_latch;
	int level_old;
	int cassette_bit;
	int row_a;
	int row_b;
	int row_c;
	int row_d;
	int laser_bg_mode;
	int laser_two_color;
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
extern VIDEO_UPDATE( laser );
extern WRITE8_HANDLER ( laser_bg_mode_w );
extern WRITE8_HANDLER ( laser_two_color_w );


#endif /* VTECH2_H_ */
