/*****************************************************************************
 *
 * includes/pcw16.h
 *
 ****************************************************************************/

#ifndef PCW16_H_
#define PCW16_H_


#define PCW16_BORDER_HEIGHT 8
#define PCW16_BORDER_WIDTH 8
#define PCW16_NUM_COLOURS 32
#define PCW16_DISPLAY_WIDTH 640
#define PCW16_DISPLAY_HEIGHT 480

#define PCW16_SCREEN_WIDTH	(PCW16_DISPLAY_WIDTH + (PCW16_BORDER_WIDTH<<1))
#define PCW16_SCREEN_HEIGHT	(PCW16_DISPLAY_HEIGHT  + (PCW16_BORDER_HEIGHT<<1))


class pcw16_state : public driver_device
{
public:
	pcw16_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	unsigned long interrupt_counter;
	int banks[4];
	int _4_bit_port;
	int fdc_int_code;
	int system_status;
	char *mem_ptr[4];
	unsigned char keyboard_data_shift;
	int keyboard_parity_table[256];
	int keyboard_bits;
	int keyboard_bits_output;
	int keyboard_state;
	int keyboard_previous_state;
	unsigned char rtc_seconds;
	unsigned char rtc_minutes;
	unsigned char rtc_hours;
	unsigned char rtc_days_max;
	unsigned char rtc_days;
	unsigned char rtc_months;
	unsigned char rtc_years;
	unsigned char rtc_control;
	unsigned char rtc_256ths_seconds;
	int previous_fdc_int_state;
	int colour_palette[16];
	int video_control;
};


/*----------- defined in video/pcw16.c -----------*/

extern PALETTE_INIT( pcw16 );
extern VIDEO_START( pcw16 );
extern VIDEO_UPDATE( pcw16 );


#endif /* PCW16_H_ */
